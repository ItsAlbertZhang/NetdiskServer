#ifndef __MYCRYPT_H__
#define __MYCRYPT_H__

#define PRIKEY 0
#define PUBKEY 1

int rsa_encrypt(const unsigned char *plaintext, unsigned char *ciphertext, RSA *rsa, int rsa_type);
int rsa_decrypt(unsigned char *plaintext, const unsigned char *ciphertext, RSA *rsa, int rsa_type);

#endif /* __MYCRYPT_H__*/