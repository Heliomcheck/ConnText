#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

void hash_pass(const char *password, char *output_hex) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)password, strlen(password), hash);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(output_hex + (i * 2), "%02x", hash[i]);

    output_hex[64] = 0;
}
