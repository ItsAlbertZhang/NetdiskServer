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

int rsa_rsa2str(char *str, RSA *rsa, int rsa_type) {
    int ret = 0;

    BIO *bio = BIO_new(BIO_s_mem());
    if (rsa_type) {
        ret = PEM_write_bio_RSAPublicKey(bio, rsa);
        RET_CHECK_BLACKLIST(0, ret, "PEM_write_bio_RSAPublicKey")
    } else {
        ret = PEM_write_bio_RSAPrivateKey(bio, rsa, NULL, NULL, 0, NULL, NULL);
        RET_CHECK_BLACKLIST(0, ret, "PEM_write_bio_RSAPrivateKey")
    }
    int keylen = BIO_pending(bio);
    bzero(str, keylen + 1);
    ret = BIO_read(bio, str, keylen);
    if (ret < 0 || ret == 0) {
        return -1;
    }

    BIO_free_all(bio);
    return keylen + 1;
}

int rsa_str2rsa(const char *str, RSA **rsa, int rsa_type) {
    int ret = 0;
    RSA *rsa_ret = NULL;

    BIO *bio = BIO_new(BIO_s_mem());
    ret = BIO_write(bio, str, strlen(str));
    if (ret < 0 || ret == 0) {
        return -1;
    }
    if (rsa_type) {
        rsa_ret = PEM_read_bio_RSAPublicKey(bio, rsa, NULL, NULL);
        RET_CHECK_BLACKLIST(NULL, rsa_ret, "PEM_read_bio_RSAPublicKey");
    } else {
        rsa_ret = PEM_read_bio_RSAPrivateKey(bio, rsa, NULL, NULL);
        RET_CHECK_BLACKLIST(NULL, rsa_ret, "PEM_read_bio_RSAPrivateKey");
    }

    BIO_free_all(bio);
    return 0;
}