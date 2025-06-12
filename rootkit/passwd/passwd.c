#include "passwd.h"

#include "config.h"
#include "io.h"

// Default
u8 access_code_hash[PASSWD_HASH_SIZE] = {
    0x5e, 0x7e, 0x56, 0x44, 0xa5, 0xeb, 0xfd, 0x8e, 0x3f, 0xd4, 0x2a,
    0x26, 0xf1, 0x5b, 0xe3, 0xe7, 0x16, 0x6a, 0xc0, 0x22, 0x53, 0xb5,
    0xb4, 0x2a, 0x99, 0x43, 0x11, 0xed, 0x09, 0x54, 0x99, 0x9d
};

/*
 * @brief Load the access code hash from the configuration file.
 *
 * Reads the hash from PASSWD_CFG_FILE, which should contain a single line
 * with the hash in hexadecimal format. The hash is expected to be 64 hex
 * characters long (representing 32 bytes).
 * @return 0 on success, negative error code on failure.
 *
 */
int passwd_load(void) {
    char *buf;
    int ret;
    char cfgpath[256];

    build_cfg_path(PASSWD_CFG_FILE, cfgpath, sizeof(cfgpath));

    ret = _read_file(cfgpath, &buf);
    if (ret < 0)
        return ret;

    // Read up to first newline
    size_t linelen = strcspn(buf, "\r\n");
    if (linelen != PASSWD_HASH_SIZE * 2) {
        kfree(buf);
        return -EINVAL;
    }

    // Parse hex string into access_code_hash[]
    ret = hex2bin(access_code_hash, buf, PASSWD_HASH_SIZE);
    if (ret < 0) {
        kfree(buf);
        return -EINVAL;
    }

    kfree(buf);

    return SUCCESS;
}

/**
 * @brief Verify the provided password against the stored hash.
 * @param password The password to verify.
 * @return 1 if the password is correct, 0 if incorrect, or negative error code on failure.
 */
int passwd_verify(const char *password) {
    u8 digest[PASSWD_HASH_SIZE];
    int err;

    err = hash_string(password, digest);
    if (err < 0)
        return err;

    return are_hash_equals(digest, access_code_hash) ? 1 : 0;
}

/**
 * @brief Set a new password by updating the stored hash.
 * @param new_password The new password to set.
 * @return 0 on success, negative error code on failure.
 */
int passwd_set(const char *new_password) {
    u8 digest[PASSWD_HASH_SIZE];
    char hexout[PASSWD_HASH_SIZE * 2 + 2];
    int err, len;
    char cfgpath[256];

    err = hash_string(new_password, digest);
    if (err < 0)
        return err;

    // Update in-memory hash
    memcpy(access_code_hash, digest, PASSWD_HASH_SIZE);

    // Build hex string and newline
    hash_to_str(digest, hexout);
    hexout[PASSWD_HASH_SIZE * 2] = '\n';
    hexout[PASSWD_HASH_SIZE * 2 + 1] = '\0';
    len = PASSWD_HASH_SIZE * 2 + 1;

    build_cfg_path(PASSWD_CFG_FILE, cfgpath, sizeof(cfgpath));

    // Write it out
    return _write_file(cfgpath, hexout, len);
}