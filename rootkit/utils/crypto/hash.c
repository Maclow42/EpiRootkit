#include <crypto/hash.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "crypto.h"

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
