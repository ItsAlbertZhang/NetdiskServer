#include "head.h"

int rsa_decrypt(unsigned char *plaintext, const unsigned char *ciphertext, RSA *rsa, int rsa_type) {
    int ret = 0;

    // 加密
    int rsa_len = RSA_size(rsa);
    bzero(plaintext, rsa_len);
    if (rsa_type) {
        ret = RSA_public_decrypt(rsa_len, ciphertext, plaintext, rsa, RSA_PKCS1_PADDING);
    } else {
        ret = RSA_private_decrypt(rsa_len, ciphertext, plaintext, rsa, RSA_PKCS1_PADDING);
    }

    return ret; // 返回读取到的明文长度
}