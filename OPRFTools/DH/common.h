#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <iomanip> // 用于setw、setfill等格式化操作

// 与接收方统一的端口常量（确保两端端口一致）
const int PARAM_PORT = 8080;
const int PUBLIC_KEY_PORT = 8081;
const int MIN_PRIME = 10000;
const int MAX_PRIME = 50000;
// 大整数模幂运算: (base^exponent) % mod
// 使用快速幂算法提高效率
inline long long mod_pow(long long base, long long exponent, long long mod)
{
    long long result = 1;
    base = base % mod; // 确保base小于mod

    while (exponent > 0)
    {
        // 如果指数是奇数，将当前base乘到结果中
        if (exponent % 2 == 1)
        {
            result = (result * base) % mod;
        }

        // 指数变为偶数，base平方，指数减半
        exponent = exponent >> 1; // 等价于exponent /= 2
        base = (base * base) % mod;
    }

    return result;
}

// 检查一个数是否为质数
inline bool is_prime(long long n)
{
    if (n <= 1)
        return false;
    if (n <= 3)
        return true;

    // 检查是否能被2或3整除
    if (n % 2 == 0 || n % 3 == 0)
        return false;

    // 检查到sqrt(n)
    for (long long i = 5; i * i <= n; i += 6)
    {
        if (n % i == 0 || n % (i + 2) == 0)
            return false;
    }
    return true;
}

// 生成一个指定范围内的随机质数
inline long long generate_prime(long long min, long long max)
{
    long long num;
    do
    {
        // 生成范围内的随机数
        num = min + (rand() % (max - min + 1));
        // 确保是奇数
        if (num % 2 == 0)
            num++;
    } while (!is_prime(num));

    return num;
}

// 检查g是否为p的原根
inline bool is_primitive_root(long long g, long long p)
{
    if (g >= p)
        return false;

    // 质因数分解p-1的结果，这里简化处理
    long long phi = p - 1;
    for (long long i = 2; i * i <= phi; i++)
    {
        if (phi % i == 0)
        {
            if (mod_pow(g, phi / i, p) == 1)
                return false;
            while (phi % i == 0)
                phi /= i;
        }
    }

    if (phi > 1)
    {
        if (mod_pow(g, phi, p) == 1)
            return false;
    }

    return true;
}

// 生成一个原根
inline long long generate_primitive_root(long long p)
{
    if (p == 2)
        return 1;

    for (long long g = 2; g < p; g++)
    {
        if (is_primitive_root(g, p))
        {
            return g;
        }
    }
    return -1; // 未找到原根
}

// 生成私有密钥
inline long long generate_private_key(long long p)
{
    // 私有密钥应该是1到p-2之间的随机数
    return 2 + (rand() % (p - 3));
}

// 计算公开密钥
inline long long generate_public_key(long long private_key, long long g, long long p)
{
    return mod_pow(g, private_key, p);
}

// 计算共享密钥
inline long long compute_shared_secret(long long received_public, long long private_key, long long p)
{
    return mod_pow(received_public, private_key, p);
}
// 将单个字节转换为两位十六进制字符串（例如：0x0a -> "0a"，0xab -> "ab"）
std::string byteToHexString(unsigned char byte)
{
    std::stringstream ss;
    // 设置十六进制格式，宽度为2，不足补0，然后写入字节的整数形式
    ss << std::hex
       << std::setw(2)
       << std::setfill('0')
       << static_cast<int>(byte);
    return ss.str(); // 返回格式化后的字符串
}
#endif // COMMON_H
