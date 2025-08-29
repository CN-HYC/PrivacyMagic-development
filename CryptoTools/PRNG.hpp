// PRNG.hpp
#ifndef PRNG_HPP
#define PRNG_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <openssl/rand.h>

namespace CryptoTools
{

    /**
     * 生成指定字节数的加密安全随机数，并返回十六进制字符串
     * @param num_bytes 需要生成的随机字节数
     * @return 指向十六进制字符串的指针，使用后需用free()释放；失败返回NULL
     */
    char *PRNG(int num_bytes)
    {
        // 校验输入（至少1字节，避免无效调用）
        if (num_bytes <= 0)
        {
            fprintf(stderr, "错误：随机数长度必须大于0\n");
            return NULL;
        }

        // 计算所需内存大小（每个字节对应2个十六进制字符 + 终止符）
        const size_t str_length = static_cast<size_t>(num_bytes) * 2 + 1;
        char *result = static_cast<char *>(malloc(str_length));
        if (!result)
        {
            fprintf(stderr, "错误：内存分配失败\n");
            return NULL;
        }

        // 存储原始随机字节
        unsigned char *random_bytes = static_cast<unsigned char *>(malloc(static_cast<size_t>(num_bytes)));
        if (!random_bytes)
        {
            fprintf(stderr, "错误：内存分配失败\n");
            free(result);
            return NULL;
        }

        // 检查随机数生成器状态并生成随机字节
        if (RAND_status() != 1 || RAND_bytes(random_bytes, static_cast<size_t>(num_bytes)) != 1)
        {
            fprintf(stderr, "错误：随机数生成失败（熵不足或初始化失败）\n");
            free(result);
            free(random_bytes);
            return NULL;
        }

        // 转换为十六进制字符串
        for (int i = 0; i < num_bytes; ++i)
        {
            snprintf(result + i * 2, 3, "%02x", random_bytes[i]);
        }
        result[str_length - 1] = '\0'; // 显式添加字符串终止符

        free(random_bytes);
        return result;
    }

} // namespace cryptoTools

#endif // PRNG_HPP

