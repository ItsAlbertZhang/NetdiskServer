#include "head.h"
#include "log.h"
#include "mycrypt.h"
#include "mylibrary.h"
#include "program_init.h"

static int mysql_try_connect_by_pwdfile(MYSQL **mysql_connect, const char *config_dir, RSA *rsa_private, const char config[][MAX_CONFIG_LENGTH]);
static int mysql_get_pwd(MYSQL **mysql_connect, const char *config_dir, RSA *rsa_public, const char config[][MAX_CONFIG_LENGTH]);
static int mysql_save_pwd(const char *mysql_pwd_plaintext, const char *config_dir, RSA *rsa_public);

int init_mysql(MYSQL **mysql_connect, const char *config_dir, RSA *rsa_private, RSA *rsa_public, char config[][MAX_CONFIG_LENGTH]) {
    int ret = 0;
    *mysql_connect = mysql_init(NULL);

    // 获取 MySQL 连接地址, 用户名, 数据库名
    ret = getconfig(config_dir, "mysql.config", config);
    RET_CHECK_BLACKLIST(-1, ret, "getconfig");

    // 尝试打开密码文件
    if (file_exist(config_dir, "mysql.pwd")) {
        // 密码文件存在
        ret = mysql_try_connect_by_pwdfile(mysql_connect, config_dir, rsa_private, config);
        if (-1 == ret) {
            // 使用配置文件登录失败
            log_print("密码文件或配置文件错误, 将重设密码.");
            printf("如果你确认密码文件无误, 可退出程序修改配置文件后重试.\n");
            ret = mysql_get_pwd(mysql_connect, config_dir, rsa_public, config);
            RET_CHECK_BLACKLIST(-1, ret, "mysql_get_pwd");
        }
    } else {
        // 密码文件不存在
        ret = mysql_get_pwd(mysql_connect, config_dir, rsa_public, config);
        RET_CHECK_BLACKLIST(-1, ret, "mysql_get_pwd");
    }

    return 0;
}

static int mysql_try_connect_by_pwdfile(MYSQL **mysql_connect, const char *config_dir, RSA *rsa_private, const char config[][MAX_CONFIG_LENGTH]) {
    int ret = 0;
    char mysql_pwd_plaintext[512] = {0};
    char mysql_pwd_ciphertext[512] = {0};
    ret = read_string_from_file(mysql_pwd_ciphertext, sizeof(mysql_pwd_ciphertext), config_dir, "mysql.pwd"); // 读取密码密文
    RET_CHECK_BLACKLIST(-1, ret, "read_string_from_file");
    ret = rsa_decrypt((unsigned char *)mysql_pwd_plaintext, (unsigned char *)mysql_pwd_ciphertext, rsa_private, PRIKEY); // 将密文解密为明文
    RET_CHECK_BLACKLIST(-1, ret, "rsa_decrypt");
    MYSQL *mysql_connect_ret = mysql_real_connect(*mysql_connect, config[0], config[1], mysql_pwd_plaintext, config[2], 0, NULL, 0); // 尝试连接
    if (NULL == mysql_connect_ret) {
        ret = -1; // 连接失败
    } else {
        ret = 0; // 连接成功
    }
    bzero(mysql_pwd_plaintext, sizeof(mysql_pwd_plaintext)); // 清空密码明文, 确保安全
    return ret;
}

static int mysql_get_pwd(MYSQL **mysql_connect, const char *config_dir, RSA *rsa_public, const char config[][MAX_CONFIG_LENGTH]) {
    int ret = 0;

    MYSQL *mysql_connect_ret = NULL;
    char *mysql_pwd_plaintext;
    int cnt = 0;
    while (NULL == mysql_connect_ret) {
        if (cnt) {
            printf("密码或配置文件错误! 请重新输入密码. 如果你确认密码无误, 可输入 exit 以退出并修改配置文件后重试.\n");
            printf("%s\n", mysql_error(*mysql_connect));
        }
        mysql_pwd_plaintext = getpass("请输入 MySQL 密码(无回显):\n");
        if (!strcmp(mysql_pwd_plaintext, "exit")) {
            kill(0, SIGINT); // 向自己发送 SIGINT 信号
        }
        mysql_connect_ret = mysql_real_connect(*mysql_connect, config[0], config[1], mysql_pwd_plaintext, config[2], 0, NULL, 0); // 尝试连接
        cnt = 1;
    }

    printf("是否要保存密码并在下一次自动登录? 密码将会以加密的方式保存在 ./config/mysql.pwd\n请注意: mysql.pwd 与 private.pem 同时泄露会造成极大的安全隐患, 任何获得这两份文件的人均可获得你的密码明文.\n请输入(y/n):");
    fflush(stdout);
    char savepwd_input;
    int savepwd = -1;
    while (-1 == savepwd) {
        if (savepwd_input == '\n') {
            printf("你输入的数据不合法! 请输入\"y\"或\"n\":");
            fflush(stdout);
        }
        savepwd_input = getchar();
        if (savepwd_input == 'y') {
            savepwd = 1;
        }
        if (savepwd_input == 'n') {
            savepwd = 0;
        }
    }
    RET_CHECK_BLACKLIST(-1, savepwd, "savepwd");

    if (savepwd) {
        ret = mysql_save_pwd(mysql_pwd_plaintext, config_dir, rsa_public);
        RET_CHECK_BLACKLIST(-1, ret, "mysql_save_pwd");
        log_print("成功保存密码.");
    }

    mysql_pwd_plaintext = NULL; // 清空密码明文, 确保安全
    return 0;
}

static int mysql_save_pwd(const char *mysql_pwd_plaintext, const char *config_dir, RSA *rsa_public) {
    int ret = 0;
    char mysql_pwd_ciphertext[512] = {0};
    ret = rsa_encrypt((unsigned char *)mysql_pwd_plaintext, (unsigned char *)mysql_pwd_ciphertext, rsa_public, PUBKEY);
    RET_CHECK_BLACKLIST(-1, ret, "rsa_encrypt");
    ret = write_file_from_string(mysql_pwd_ciphertext, ret, config_dir, "mysql.pwd");
    RET_CHECK_BLACKLIST(-1, ret, "write_file_from_string");

    return 0;
}