#ifndef PRF_AES_H
#define PRF_AES_H

#include <string>
#include <cstdint>
#include <array>

// 基于AES的伪随机函数实现
class PRF_AES
{
private:
    // AES密钥 (支持128位、192位或256位)
    std::string key_;
    // 密钥长度(字节)
    size_t key_len_;

    // AES内部状态
    struct AESContext
    {
        uint32_t round_keys[60]; // 轮密钥
        int rounds;              // 轮数，取决于密钥长度
    };
    AESContext ctx_;

    // 初始化AES上下文
    void aes_init(const uint8_t *key, size_t key_len);
    // AES加密单块数据
    void aes_encrypt(const uint8_t *input, uint8_t *output);

public:
    // 构造函数，接受密钥
    PRF_AES(const std::string &key);

    // 禁止复制构造和赋值，确保密钥安全
    PRF_AES(const PRF_AES &) = delete;
    PRF_AES &operator=(const PRF_AES &) = delete;

    // 移动构造和移动赋值
    PRF_AES(PRF_AES &&) = default;
    PRF_AES &operator=(PRF_AES &&) = default;

    // 析构函数，清除敏感数据
    ~PRF_AES();

    // 核心函数：计算PRF(key, input)
    // 输入：任意长度的输入
    // 输出：16字节(128位)的伪随机输出
    std::string evaluate(const std::string &input);

    // 重载版本，接受字节数组
    std::string evaluate(const uint8_t *input, size_t input_len);
};

#endif // PRF_AES_H
