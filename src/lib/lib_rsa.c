#include "head.h"

int rsa_encrypt(const unsigned char *plaintext, unsigned char *ciphertext, RSA *rsa, int rsa_type) {
    int ret = 0;

    // 加密
    int rsa_len = RSA_size(rsa);
    bzero(ciphertext, rsa_len);
    if (rsa_type) {
        ret = RSA_public_encrypt(rsa_len - 11, plaintext, ciphertext, rsa, RSA_PKCS1_PADDING);
    } else {
        ret = RSA_private_encrypt(rsa_len - 11, plaintext, ciphertext, rsa, RSA_PKCS1_PADDING);
    }
    // PKCS#1 建议的 padding 占用 11 字节
    RET_CHECK_BLACKLIST(-1, ret, "RSA_encrypt");
    rsa_len = RSA_size(rsa);

    return rsa_len; // 返回密文长度 (2048 位密钥情况下为 256 字节)
}

int rsa_decrypt(unsigned char *plaintext, const unsigned char *ciphertext, RSA *rsa, int rsa_type) {
    int ret = 0;

    // 解密
    int rsa_len = RSA_size(rsa);
    bzero(plaintext, rsa_len);
    if (rsa_type) {
        ret = RSA_public_decrypt(rsa_len, ciphertext, plaintext, rsa, RSA_PKCS1_PADDING);
    } else {
        ret = RSA_private_decrypt(rsa_len, ciphertext, plaintext, rsa, RSA_PKCS1_PADDING);
    }

    return ret; // 返回读取到的明文长度
}