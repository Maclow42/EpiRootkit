\page password Password Management

\tableofcontents

## 1. 🗂️ Management {#password-management}

Once the rootkit is connected to the attack server, authentication is required by entering a password to execute commands. The password implementation uses SHA-256 hashing to avoid storing the password in clear text or hardcoding it in the source code. The password hash is compared to a reference value, and only a correct password allows access to sensitive functionalities. This approach enhances security by avoiding direct exposure of the password in code or on disk.

- The password hash is stored in hexadecimal form in a persistent configuration file, named `passwd.cfg`. If this file doesn't exist, the default password is used, specifically `evannounet`.

- During verification, the provided password is hashed with SHA-256 then compared to the stored hash.## Default Password

- Password modification updates the hash in the configuration file, always without ever storing the password in clear text.

The default password is `evannounet`.

## 2. 🛠️ Example

## Password Storage

```c

// SHA-256 hash of default password (example)Passwords are hashed using a cryptographic hash function before storage.

u8 access_code_hash[PASSWD_HASH_SIZE] = {

  0x5e, 0x7e, 0x56, 0x44, 0xa5, 0xeb, 0xfd, 0x8e, 0x3f, 0xd4, 0x2a,## Changing Password

  0x26, 0xf1, 0x5b, 0xe3, 0xe7, 0x16, 0x6a, 0xc0, 0x22, 0x53, 0xb5,

  0xb4, 0x2a, 0x99, 0x43, 0x11, 0xed, 0x09, 0x54, 0x99, 0x9dThe password can be changed remotely using the `passwd` command.

};
```

The password hash is stored in memory and can thus be directly used for verification. This is the SHA-256 hash of the default password `evannounet`.

```c
int passwd_verify(const char *password) {
  u8 digest[PASSWD_HASH_SIZE];
  int err;

  err = hash_string(password, digest);
  if (err < 0)
    return err;

  return are_hash_equals(digest, access_code_hash) ? 1 : 0;
}
```

When a password is provided, it's hashed then compared to the reference hash. Access is granted only if both values match.

```c
int passwd_set(const char *new_password) {
  u8 digest[PASSWD_HASH_SIZE];
  char hexout[PASSWD_HASH_SIZE * 2 + 2];
  int err, len;
  char cfgpath[256];

  err = hash_string(new_password, digest);
  if (err < 0)
    return err;

  // Update hash in memory
  memcpy(access_code_hash, digest, PASSWD_HASH_SIZE);

  // Convert hash to hexadecimal string and save to configuration file
  hash_to_str(digest, hexout);
  hexout[PASSWD_HASH_SIZE * 2] = '\n';
  hexout[PASSWD_HASH_SIZE * 2 + 1] = '\0';
  len = PASSWD_HASH_SIZE * 2 + 1;

  build_cfg_path(PASSWD_CFG_FILE, cfgpath, sizeof(cfgpath));

  // Write hash to configuration file
  return _write_file(cfgpath, hexout, len);
}
```

This function allows changing the password: the new password is hashed, stored in memory and saved to the configuration file in hexadecimal form. For more information on the authentication procedure and password use during connection, see the [Connection](#connection) section. This section details the process of accessing the web interface, entering the password (`evannounet` by default), as well as the necessary steps to access the main dashboard after authentication.

> For more details on password change, see the [Command List](#command-list) section and particularly the `passwd` command.

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
