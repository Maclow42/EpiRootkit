#include <crypto/skcipher.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>

#include "config.h"
#include "crypto.h"

/**
 * @brief Add PKCS#7 padding to a buffer
 *
 * This function adds PKCS#7 padding to the input buffer. In PKCS#7,
 * the value of each added byte is the number of bytes that are added.
 *
 * @param in Input buffer
 * @param in_len Length of input buffer
 * @param out Output buffer (allocated within the function)
 * @param out_len Length of output buffer
 * @return 0 on success, negative error code on failure
 */
static int add_pkcs7_padding(const char *in, size_t in_len, char **out, size_t *out_len) {
    size_t padding_len;
    size_t padded_len;
    char *padded_buf;
    int i;

    // Calculate padding length (1-16 bytes)
    padding_len = AES_BLOCK_SIZE - (in_len % AES_BLOCK_SIZE);
    padded_len = in_len + padding_len;

    // Allocate padded buffer
    padded_buf = vmalloc(padded_len);
    if (!padded_buf)
        return -ENOMEM;

    // Copy original data
    memcpy(padded_buf, in, in_len);

    // Add padding bytes
    for (i = 0; i < padding_len; i++) {
        padded_buf[in_len + i] = (char)padding_len;
    }

    *out = padded_buf;
    *out_len = padded_len;

    return 0;
}

/**
 * @brief Remove PKCS#7 padding from a buffer
 *
 * This function removes PKCS#7 padding from the input buffer.
 *
 * @param in Input buffer with padding
 * @param in_len Length of input buffer
 * @param out Output buffer (allocated within the function)
 * @param out_len Length of output buffer
 * @return 0 on success, negative error code on failure
 */
static int remove_pkcs7_padding(const char *in, size_t in_len, char **out, size_t *out_len) {
    unsigned char padding_len;
    size_t data_len;
    char *unpadded_buf;
    int i;

    // Validate input length
    if (in_len == 0 || in_len % AES_BLOCK_SIZE != 0)
        return -EINVAL;

    // Get padding length from the last byte
    padding_len = (unsigned char)in[in_len - 1];

    // Validate padding length
    if (padding_len == 0 || padding_len > AES_BLOCK_SIZE || padding_len > in_len)
        return -EINVAL;

    // Verify all padding bytes are correct
    for (i = 0; i < padding_len; i++) {
        if ((unsigned char)in[in_len - 1 - i] != padding_len)
            return -EINVAL;
    }

    // Calculate data length
    data_len = in_len - padding_len;

    // Allocate unpadded buffer
    unpadded_buf = vmalloc(data_len);
    if (!unpadded_buf)
        return -ENOMEM;

    // Copy data without padding
    memcpy(unpadded_buf, in, data_len);

    *out = unpadded_buf;
    *out_len = data_len;

    return 0;
}

static int _crypt_buffer(bool encrypt, const char *in, size_t in_len, char **out, size_t *out_len) {
    struct crypto_skcipher *tfm;  // Cipher transformation handle
    struct skcipher_request *req; // Cipher request handle
    struct scatterlist sg;        // Scatterlist for input/output data
    char *buf = NULL;             // Buffer to hold input data (padded if necessary)
    char *padded_in = NULL;       // Buffer for padded input (for encryption)
    size_t padded_len = 0;        // Length of padded input
    int ret = 0;                  // Return value
    char iv[AES_BLOCK_SIZE];      // Initialization vector

    if (!in || !out || !out_len)
        return -EINVAL;

    // Initialize output parameters
    *out = NULL;
    *out_len = 0;

    memcpy(iv, AES_IV, AES_BLOCK_SIZE); // Copy IV

    if (encrypt) {
        // Add PKCS#7 padding for encryption
        ret = add_pkcs7_padding(in, in_len, &padded_in, &padded_len);
        if (ret != 0)
            return ret;
    }
    else {
        // For decryption, input must be a multiple of block size
        if (in_len % AES_BLOCK_SIZE != 0) {
            ERR_MSG("Decrypt input length must be a multiple of %d\n", AES_BLOCK_SIZE);
            return -EINVAL;
        }
        padded_in = vmalloc(in_len);
        if (!padded_in)
            return -ENOMEM;
        memcpy(padded_in, in, in_len);
        padded_len = in_len;
    }

    // Allocate buffer for results
    buf = vmalloc(padded_len);
    if (!buf) {
        vfree(padded_in);
        return -ENOMEM;
    }

    // Allocate cipher transformation for AES in CBC mode
    tfm = crypto_alloc_skcipher("cbc(aes)", 0, 0);
    if (IS_ERR(tfm)) {
        ret = PTR_ERR(tfm);
        goto out_free_buffers;
    }

    // Allocate cipher request
    req = skcipher_request_alloc(tfm, GFP_KERNEL);
    if (!req) {
        ret = -ENOMEM;
        goto out_free_tfm;
    }

    // Set the encryption key
    ret = crypto_skcipher_setkey(tfm, AES_KEY, 16);
    if (ret != 0) {
        pr_err("setkey failed: %d\n", ret);
        goto out_free_req;
    }

    // Initialize scatterlist with input and output buffers
    sg_init_one(&sg, padded_in, padded_len);
    skcipher_request_set_crypt(req, &sg, &sg, padded_len, iv);

    // Perform encryption or decryption
    if (encrypt)
        ret = crypto_skcipher_encrypt(req);
    else
        ret = crypto_skcipher_decrypt(req);

    if (ret) {
        ERR_MSG("%s failed: %d\n", encrypt ? "encrypt" : "decrypt", ret);
        goto out_free_req;
    }

    // Copy result
    memcpy(buf, sg_virt(&sg), padded_len);

    if (!encrypt) {
        // Remove padding after decryption
        char *unpadded_buf;
        size_t unpadded_len;

        ret = remove_pkcs7_padding(buf, padded_len, &unpadded_buf, &unpadded_len);
        if (ret != 0) {
            ERR_MSG("Failed to remove padding: %d\n", ret);
            goto out_free_req;
        }

        // Free the original buffer and replace with unpadded one
        vfree(buf);
        buf = unpadded_buf;
        padded_len = unpadded_len;
    }

    *out = buf;
    *out_len = padded_len;
    buf = NULL; // Prevent free on success path

out_free_req:
    skcipher_request_free(req);
out_free_tfm:
    crypto_free_skcipher(tfm);
out_free_buffers:
    vfree(padded_in);
    vfree(buf);

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