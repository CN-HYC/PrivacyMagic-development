#ifndef PRP_AES_HPP
#define PRP_AES_HPP

#include <iostream>
#include <vector>
#include <cstring>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace CryptoTools
{
    class PRP_AES
    {
    private:
        EVP_CIPHER_CTX *enc_ctx;          // 加密上下文
        EVP_CIPHER_CTX *dec_ctx;          // 解密上下文
        int key_len;                      // 密钥长度（位）
        static const int BLOCK_SIZE = 16; // AES固定块大小（16字节）

    public:
        // 构造函数：初始化密钥和上下文（核心修复：禁用填充）
        PRP_AES(const unsigned char *key, int key_length) : key_len(key_length)
        {
            if (key_length != 128 && key_length != 192 && key_length != 256)
            {
                throw std::invalid_argument("AES密钥长度必须是128、192或256位");
            }

            // 创建加密上下文
            enc_ctx = EVP_CIPHER_CTX_new();
            if (!enc_ctx)
            {
                throw std::runtime_error("无法创建加密上下文");
            }

            // 创建解密上下文
            dec_ctx = EVP_CIPHER_CTX_new();
            if (!dec_ctx)
            {
                EVP_CIPHER_CTX_free(enc_ctx);
                throw std::runtime_error("无法创建解密上下文");
            }

            // 选择AES算法（ECB模式）
            const EVP_CIPHER *cipher = nullptr;
            switch (key_length)
            {
            case 128:
                cipher = EVP_aes_128_ecb();
                break;
            case 192:
                cipher = EVP_aes_192_ecb();
                break;
            case 256:
                cipher = EVP_aes_256_ecb();
                break;
            default:
                EVP_CIPHER_CTX_free(enc_ctx);
                EVP_CIPHER_CTX_free(dec_ctx);
                throw std::invalid_argument("不支持的AES密钥长度");
            }

            // 初始化加密上下文 + 禁用填充（关键修复）
            if (EVP_EncryptInit_ex(enc_ctx, cipher, nullptr, key, nullptr) != 1)
            {
                EVP_CIPHER_CTX_free(enc_ctx);
                EVP_CIPHER_CTX_free(dec_ctx);
                throw std::runtime_error("无法初始化加密上下文");
            }
            EVP_CIPHER_CTX_set_padding(enc_ctx, 0); // 禁用加密填充

            // 初始化解密上下文 + 禁用填充（关键修复）
            if (EVP_DecryptInit_ex(dec_ctx, cipher, nullptr, key, nullptr) != 1)
            {
                EVP_CIPHER_CTX_free(enc_ctx);
                EVP_CIPHER_CTX_free(dec_ctx);
                throw std::runtime_error("无法初始化解密上下文");
            }
            EVP_CIPHER_CTX_set_padding(dec_ctx, 0); // 禁用解密填充
        }

        // 析构函数：释放上下文资源
        ~PRP_AES()
        {
            EVP_CIPHER_CTX_free(enc_ctx);
            EVP_CIPHER_CTX_free(dec_ctx);
        }

        // 禁用拷贝构造和赋值（避免上下文浅拷贝）
        PRP_AES(const PRP_AES &) = delete;
        PRP_AES &operator=(const PRP_AES &) = delete;

        // 生成随机密钥
        static std::vector<unsigned char> generate_random_key(int key_length)
        {
            if (key_length != 128 && key_length != 192 && key_length != 256)
            {
                throw std::invalid_argument("AES密钥长度必须是128、192或256位");
            }
            int key_bytes = key_length / 8;
            std::vector<unsigned char> key(key_bytes);
            if (RAND_bytes(key.data(), key_bytes) != 1)
            {
                throw std::runtime_error("生成随机密钥失败");
            }
            return key;
        }

        // 置换操作（加密）：仅处理16字节完整块
        void permute(const unsigned char *input, unsigned char *output) const
        {
            int out_len = 0;
            // 禁用填充后，无需EVP_EncryptFinal_ex（仅需EVP_EncryptUpdate）
            if (EVP_EncryptUpdate(enc_ctx, output, &out_len, input, BLOCK_SIZE) != 1)
            {
                throw std::runtime_error("置换操作失败");
            }
            // 验证输出长度是否为16字节（确保无异常）
            if (out_len != BLOCK_SIZE)
            {
                throw std::runtime_error("置换输出长度异常");
            }
        }

        // 逆置换操作（解密）：仅处理16字节完整块
        void inverse_permute(const unsigned char *input, unsigned char *output) const
        {
            int out_len = 0;
            // 禁用填充后，无需EVP_DecryptFinal_ex（仅需EVP_DecryptUpdate）
            if (EVP_DecryptUpdate(dec_ctx, output, &out_len, input, BLOCK_SIZE) != 1)
            {
                throw std::runtime_error("逆置换操作失败");
            }
            // 验证输出长度是否为16字节（确保无异常）
            if (out_len != BLOCK_SIZE)
            {
                throw std::runtime_error("逆置换输出长度异常");
            }
        }

        // 处理任意长度数据的置换（统一零填充）
        std::vector<unsigned char> permute_data(const unsigned char *data, size_t length) const
        {
            std::vector<unsigned char> result;
            size_t full_blocks = length / BLOCK_SIZE;
            size_t remaining = length % BLOCK_SIZE;

            // 处理完整块
            for (size_t i = 0; i < full_blocks; ++i)
            {
                unsigned char block[BLOCK_SIZE];
                permute(&data[i * BLOCK_SIZE], block);
                result.insert(result.end(), block, block + BLOCK_SIZE);
            }

            // 处理剩余字节（零填充到16字节后置换，保留完整块）
            if (remaining > 0)
            {
                unsigned char block[BLOCK_SIZE] = {0}; // 零填充
                memcpy(block, &data[full_blocks * BLOCK_SIZE], remaining);
                permute(block, block);
                result.insert(result.end(), block, block + BLOCK_SIZE);
            }

            return result;
        }

        // 处理任意长度数据的逆置换（对应零填充逻辑）
        std::vector<unsigned char> inverse_permute_data(const unsigned char *data, size_t length) const
        {
            // 校验输入长度是否为16字节的整数倍（置换后数据必为完整块）
            if (length % BLOCK_SIZE != 0)
            {
                throw std::invalid_argument("逆置换输入长度必须是16字节的整数倍");
            }

            std::vector<unsigned char> result;
            size_t full_blocks = length / BLOCK_SIZE;

            // 处理所有完整块
            for (size_t i = 0; i < full_blocks; ++i)
            {
                unsigned char block[BLOCK_SIZE];
                inverse_permute(&data[i * BLOCK_SIZE], block);
                result.insert(result.end(), block, block + BLOCK_SIZE);
            }

            return result;
        }

        // 获取密钥长度（位）
        int get_key_length() const { return key_len; }

        // 获取AES块大小（字节）
        static int get_block_size() { return BLOCK_SIZE; }
    };

    // 辅助函数：打印字节数组
    void print_bytes(const unsigned char *data, size_t length, const std::string &label = "");

} // namespace CryptoTools

#endif // PRP_AES_HPP