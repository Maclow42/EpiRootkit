#ifndef CRYPTO_H
#define CRYPTO_H

// AES encryption parameters
// Key must be 16 bytes (128 bits) for AES-128
// IV is used to initialize the cipher and must be 16 bytes as well
#define AES_BLOCK_SIZE 16

#define SHA256_DIGEST_SIZE 32

int encrypt_buffer(const char *in, size_t in_len, 			// Encrypt a buffer using AES
	char **out, size_t *out_len);
int decrypt_buffer(const char *in, size_t in_len, 			// Decrypt a buffer using AES
	char **out, size_t *out_len);

int hash_string(const char *input, u8 *digest);				// Hash a string using SHA-256
bool are_hash_equals(const u8 *h1, const u8 *h2);			// Compare two hashes
void hash_to_str(const u8 *digest, char *output);			// Convert a hash to a string representation


#endif /* CRYPTO_H */