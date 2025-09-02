#include "PRPDemo.h"
#include <iostream>
#include <cstring>
#include <openssl/evp.h> // OpenSSL初始化/清理函数

// 辅助函数：打印字节数组（实现）
void print_bytes(const unsigned char *data, size_t length, const std::string &label)
{
    if (!label.empty())
    {
        std::cout << label << ": ";
    }
    for (size_t i = 0; i < length; ++i)
    {
        printf("%02x ", data[i]);
    }
    std::cout << std::endl;
}

// 演示函数：PRP_AES功能测试（实现）
int PRPDemo()
{
    try
    {
        // 初始化OpenSSL算法库
        OpenSSL_add_all_algorithms();

        // 测试单个完整块的置换与逆置换
        int key_length = 128;
        std::cout << "=== 测试" << key_length << "位密钥的PRP_AES（单个块） ===" << std::endl;

        // 生成随机密钥
        auto key = CryptoTools::PRP_AES::generate_random_key(key_length); // 注意：PRP_AES仍在CryptoTools命名空间中
        print_bytes(key.data(), key.size(), "生成的随机密钥");

        // 创建PRP_AES实例
        CryptoTools::PRP_AES prp(key.data(), key_length);
        const int block_size = CryptoTools::PRP_AES::get_block_size();

        // 初始化16字节测试数据
        std::vector<unsigned char> plaintext_block(block_size, 0);
        const char *init_str = "TestAESBlock123";
        memcpy(plaintext_block.data(), init_str, strlen(init_str));

        std::vector<unsigned char> ciphertext_block(block_size);
        std::vector<unsigned char> decrypted_block(block_size);

        // 执行置换与逆置换
        print_bytes(plaintext_block.data(), block_size, "原始数据块");
        prp.permute(plaintext_block.data(), ciphertext_block.data());
        print_bytes(ciphertext_block.data(), block_size, "置换后的数据块");
        prp.inverse_permute(ciphertext_block.data(), decrypted_block.data());
        print_bytes(decrypted_block.data(), block_size, "逆置换后的数据块");

        // 验证单个块
        if (memcmp(plaintext_block.data(), decrypted_block.data(), block_size) == 0)
        {
            std::cout << "✅ 单个块测试成功：逆置换正确恢复原始数据" << std::endl;
        }
        else
        {
            std::cout << "❌ 单个块测试失败：逆置换未能恢复原始数据" << std::endl;
            return 1;
        }

        // 测试长数据处理
        std::cout << "\n=== 测试长数据处理（含零填充） ===" << std::endl;
        const char *long_data = "这是一段超过16字节的测试数据，用于验证PRP_AES！";
        size_t origin_len = strlen(long_data);

        std::cout << "原始数据: " << long_data << std::endl;
        std::cout << "原始数据长度: " << origin_len << " 字节" << std::endl;
        print_bytes(reinterpret_cast<const unsigned char *>(long_data), origin_len, "原始数据（十六进制）");

        // 置换长数据
        auto encrypted_data = prp.permute_data(reinterpret_cast<const unsigned char *>(long_data), origin_len);
        std::cout << "置换后数据长度: " << encrypted_data.size() << " 字节（16的整数倍）" << std::endl;
        print_bytes(encrypted_data.data(), encrypted_data.size(), "置换后的数据");

        // 逆置换长数据
        auto decrypted_data = prp.inverse_permute_data(encrypted_data.data(), encrypted_data.size());
        print_bytes(decrypted_data.data(), decrypted_data.size(), "逆置换后的数据（含填充）");

        // 验证长数据
        std::string decrypted_str(reinterpret_cast<const char *>(decrypted_data.data()), origin_len);
        std::cout << "解密后数据: " << decrypted_str << std::endl;

        if (memcmp(long_data, decrypted_data.data(), origin_len) == 0)
        {
            std::cout << "✅ 长数据测试成功：逆置换正确恢复原始数据（忽略填充）" << std::endl;
        }
        else
        {
            std::cout << "❌ 长数据测试失败：逆置换未能恢复原始数据" << std::endl;
            return 1;
        }

        // 清理OpenSSL资源
        EVP_cleanup();
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ 错误: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n=== 所有PRP_AES测试通过 ===" << std::endl;
    return 0;
}