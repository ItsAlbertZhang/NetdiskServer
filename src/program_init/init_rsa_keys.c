#include "head.h"
#include "mylibrary.h"
#include "program_init.h"

// 生成 rsa 密钥对并保存至密钥文件
static int generate_rsa_keys(const char *private_key_filename, const char *public_key_filename);
// 检查 rsa 密钥是否成对
static int check_rsa_keys(RSA *private_rsa, RSA *public_rsa);
// 从密钥文件获取 rsa 结构体
static int get_rsa_from_file(RSA **rsa, const char *rsa_key_filename, int rsa_type);

int init_rsa_keys(RSA **private_rsa, RSA **public_rsa, const char *config_dir) {
    int ret = 0;

    // 拼接出公私钥的完整路径
    char private_key_filename[1024] = {0};
    sprintf(private_key_filename, "%s%s", config_dir, "private.pem");
    char public_key_filename[1024] = {0};
    sprintf(public_key_filename, "%s%s", config_dir, "public.pem");

    ret = 0;
    if (file_exist(NULL, private_key_filename) && file_exist(NULL, public_key_filename)) {
        // 两个文件均存在
        ret += get_rsa_from_file(private_rsa, private_key_filename, PRIKEY);
        ret += get_rsa_from_file(public_rsa, public_key_filename, PUBKEY);
        if (0 == ret) { // 文件中的内容是有效 rsa 信息
            // 核验文件中的密钥是否成对
            ret += check_rsa_keys(*private_rsa, *public_rsa);
        }
    } else {
        ret = -1;
    }
    if (0 == ret) {
        logging(LOG_INFO, "服务端运行所需密钥验证完毕.");
        return 0;
    } else {
        // 两个文件至少有其一不存在, 或两个密钥不成对, 重新生成密钥
        unlink(private_key_filename);
        unlink(public_key_filename);
        ret = generate_rsa_keys(private_key_filename, public_key_filename);
        RET_CHECK_BLACKLIST(-1, ret, "generate_rsa_keys");
        logging(LOG_WARN, "服务端运行所需密钥不存在或不成对, 已重新生成密钥.");

        ret = get_rsa_from_file(private_rsa, private_key_filename, PRIKEY);
        RET_CHECK_BLACKLIST(-1, ret, "get_rsa_from_file");
        ret = get_rsa_from_file(public_rsa, public_key_filename, PUBKEY);
        RET_CHECK_BLACKLIST(-1, ret, "get_rsa_from_file");
    }

    return 0;
}

static int generate_rsa_keys(const char *private_key_filename, const char *public_key_filename) {
    int ret = 1;
    RSA *r = NULL;
    BIGNUM *bne = NULL;
    BIO *bp_public = NULL, *bp_private = NULL;

    int bits = 2048;
    unsigned long e = RSA_F4;

    // 1. generate rsa key
    if (1 == ret) {
        bne = BN_new();
        ret = BN_set_word(bne, e);
    }
    if (1 == ret) {
        r = RSA_new();
        ret = RSA_generate_key_ex(r, bits, bne, NULL);
    }

    // 2. save public key
    if (1 == ret) {
        bp_public = BIO_new_file(public_key_filename, "w+");
        ret = PEM_write_bio_RSAPublicKey(bp_public, r);
    }

    // 3. save private key
    if (1 == ret) {
        bp_private = BIO_new_file(private_key_filename, "w+");
        ret = PEM_write_bio_RSAPrivateKey(bp_private, r, NULL, NULL, 0, NULL, NULL);
    }

    // 4. free
    BIO_free_all(bp_public);
    BIO_free_all(bp_private);
    RSA_free(r);
    BN_free(bne);

    if (1 == ret) {
        ret = 0;
    } else {
        ret = -1;
    }

    return ret;
}

static int check_rsa_keys(RSA *private_rsa, RSA *public_rsa) {
    int ret = 0;

    unsigned char source[256] = {0};
    unsigned char plaintext[1024] = {0};
    unsigned char ciphertext[256] = {0};

    // 随机生成 128 个字节
    ret = random_gen_str(source, 128, 0);
    // 对读取到的内容加密再解密, 进行比对
    ret = rsa_encrypt(source, ciphertext, public_rsa, PUBKEY);
    RET_CHECK_BLACKLIST(-1, ret, "RSAfile_encrypt");
    ret = rsa_decrypt(plaintext, ciphertext, private_rsa, PRIKEY);
    // 此处不进行返回值检查, 因为当公私钥不匹配时会返回 -1, 这种情况下不希望在此处报错.
    ret = strcmp(source, plaintext);

    return ret;
}

static int get_rsa_from_file(RSA **rsa, const char *rsa_key_filename, int rsa_type) {
    int ret = 0;

    // 打开 RSA 密钥文件
    int fd = open(rsa_key_filename, O_RDWR);
    RET_CHECK_BLACKLIST(-1, fd, "open");

    // 从文件中获取 RSA 信息
    char buf[4096] = {0};
    ret = read(fd, buf, sizeof(buf));
    RET_CHECK_BLACKLIST(-1, ret, "read");
    ret = rsa_str2rsa(buf, rsa, rsa_type);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_str2rsa");

    close(fd);

    return 0;
}