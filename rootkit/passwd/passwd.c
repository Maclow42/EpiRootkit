#include "passwd.h"
#include "io.h"
#include "config.h"

u8 passwd_hash[PASSWD_HASH_SIZE] = {
    0x5e, 0x7e, 0x56, 0x44, 0xa5, 0xeb, 0xfd,
    0x8e, 0x3f, 0xd4, 0x2a, 0x26, 0xf1, 0x5b,
    0xe3, 0xe7, 0x16, 0x6a, 0xc0, 0x22, 0x53,
    0xb5, 0xb4, 0x2a, 0x99, 0x43, 0x11, 0xed,
    0x09, 0x54, 0x99, 0x9d
};

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

    // Parse hex string into passwd_hash[]
    ret = hex2bin(passwd_hash, buf, PASSWD_HASH_SIZE);
    if (ret < 0) {
        kfree(buf);
        return -EINVAL;
    }

    kfree(buf);

    return SUCCESS;
}

int passwd_verify(const char *password) {
    u8 digest[PASSWD_HASH_SIZE];
    int err;

    err = hash_string(password, digest);
    if (err < 0)
        return err;

    return are_hash_equals(digest, passwd_hash) ? 1 : 0;
}

int passwd_set(const char *new_password) {
    u8 digest[PASSWD_HASH_SIZE];
    char hexout[PASSWD_HASH_SIZE * 2 + 2];
    int err, len;
    char cfgpath[256];

    err = hash_string(new_password, digest);
    if (err < 0)
        return err;

    // Update in-memory hash
    memcpy(passwd_hash, digest, PASSWD_HASH_SIZE);

    // Build hex string and newline
    hash_to_str(digest, hexout);
    hexout[PASSWD_HASH_SIZE * 2] = '\n';
    hexout[PASSWD_HASH_SIZE * 2 + 1] = '\0';
    len = PASSWD_HASH_SIZE * 2 + 1;

    build_cfg_path(PASSWD_CFG_FILE, cfgpath, sizeof(cfgpath));

    // Write it out
    return _write_file(cfgpath, hexout, len);
}