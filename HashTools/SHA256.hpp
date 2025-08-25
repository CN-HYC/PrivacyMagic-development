#ifndef SHA256_HPP
#define SHA256_HPP

#include <string>
#include <array>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstddef>

/**
 * @brief SHA-256 哈希算法完整实现类
 *
 * 此类整合了哈希算法的声明与实现，支持字节数组和字符串输入，
 * 输出 32 字节（256 位）哈希数组或 64 字符十六进制字符串，
 * 完全遵循 SHA-256 标准规范。
 */
class SHA256
{
public:
    /**
     * @brief 构造函数
     * 初始化哈希状态、数据缓冲区和计数器，委托 reset() 消除代码重复
     */
    SHA256()
    {
        reset();
    }

    /**
     * @brief 重置哈希状态
     * 恢复初始状态（清空缓冲区、重置计数器、初始化哈希寄存器），支持重复计算
     */
    void reset()
    {
        std::memset(m_data, 0, sizeof(m_data));
        m_blocklen = 0;
        m_bitlen = 0;

        // 初始状态：前8个质数的平方根小数部分前32位
        static constexpr std::array<uint32_t, 8> INITIAL_STATE = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
        std::copy(INITIAL_STATE.begin(), INITIAL_STATE.end(), m_state);
    }

    /**
     * @brief 处理输入数据（字节数组形式）
     * @param data 输入数据的字节数组指针
     * @param length 输入数据长度（字节数）
     * 注：每次调用会先重置状态，确保单次计算的独立性
     */
    void input(const uint8_t *data, size_t length)
    {
        reset();
        for (size_t i = 0; i < length; i++)
        {
            m_data[m_blocklen++] = data[i];
            // 缓冲区满64字节（512位）时，触发块处理
            if (m_blocklen == 64)
            {
                transform();
                m_bitlen += 512;
                m_blocklen = 0;
            }
        }
    }

    /**
     * @brief 处理输入数据（字符串形式，重载）
     * @param data 输入的字符串
     * 自动转换字符串为字节数组，调用字节数组版本的 input
     */
    void input(const std::string &data)
    {
        input(reinterpret_cast<const uint8_t *>(data.c_str()), data.size());
    }

    /**
     * @brief 完成哈希计算并返回哈希字节数组
     * @return std::array<uint8_t, 32> 32字节大端序哈希结果
     */
    std::array<uint8_t, 32> digest()
    {
        std::array<uint8_t, 32> hash;
        pad();        // 填充剩余数据
        revert(hash); // 状态转字节数组
        return hash;
    }

    /**
     * @brief 完成哈希计算并返回十六进制字符串
     * @return std::string 64字符小写十六进制哈希结果
     */
    std::string output(uint8_t format = 64)
    {
        std::array<uint8_t, 32> hash;
        pad();
        revert(hash);
        if (format == 32)
        {
            return toString_32(hash); // 返回32字符十六进制字符串
        }
        else if (format == 64)
        {
            return toString_64(hash); // 返回64字符十六进制字符串
        }
        else
        {
            throw std::invalid_argument("Unsupported format, use 32 or 64");
        }
    }

    /**
     * @brief 静态方法：哈希字节数组转十六进制字符串
     * @param digest 32字节哈希数组（通常由 digest() 获取）
     * @return std::string 64字符小写十六进制字符串
     */
    static std::string toString_64(const std::array<uint8_t, 32> &digest)
    {
        std::stringstream s;
        s << std::setfill('0') << std::hex;
        for (uint8_t byte : digest)
        {
            s << std::setw(2) << static_cast<unsigned int>(byte);
        }
        return s.str();
    }
    /**
     * @brief 静态方法：哈希字节数组转十六进制字符串
     * @param digest 32字节哈希数组（通常由 digest() 获取）
     * @return std::string 32字符小写十六进制字符串
     * 原理：每2个连续字节（共16位）合并，转换为1个4位十六进制字符（0-9, a-f）
     */
    static std::string toString_32(const std::array<uint8_t, 32> &digest)
    {
        std::string result;
        result.reserve(32);                         // 预分配32字符空间，提升效率
        const char *hex_chars = "0123456789abcdef"; // 小写十六进制字符表

        // 每2个字节合并为1个十六进制字符（32字节 → 16组 → 16字符？不，原需求32字符需调整逻辑）
        // 修正逻辑：原“1字节→2字符”是64字符，要32字符需“1字节→1字符”（取字节高4位或低4位，此处取高4位示例）
        for (uint8_t byte : digest)
        {
            // 取每个字节的「高4位」作为十六进制字符（1字节→1字符，32字节→32字符）
            uint8_t hex_idx = (byte >> 4) & 0x0F; // 右移4位，保留高4位
            result += hex_chars[hex_idx];
        }

        return result;
    }

private:
    // ------------------------------ 私有成员变量 ------------------------------
    uint8_t m_data[64] = {0};  // 数据缓冲区（64字节 = SHA-256 块大小）
    uint32_t m_blocklen = 0;   // 缓冲区当前长度（0~63字节）
    uint64_t m_bitlen = 0;     // 已处理数据总长度（比特数）
    uint32_t m_state[8] = {0}; // 哈希状态寄存器（A~H 8个32位值）

    /**
     * @brief SHA-256 标准64个常量K
     * 来自前64个质数的立方根小数部分前32位，用于压缩循环
     */
    static constexpr std::array<uint32_t, 64> K = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

    // ------------------------------ 私有成员函数 ------------------------------
    /**
     * @brief 32位整数循环右移
     * @param x 待移位的32位整数
     * @param n 移位位数
     * @return 移位后的结果
     */
    static uint32_t rotr(uint32_t x, uint32_t n)
    {
        return (x >> n) | (x << (32 - n));
    }

    /**
     * @brief 选择函数 Ch(e, f, g)
     * 根据e的位选择f或g的对应位（e=1选f，e=0选g）
     */
    static uint32_t choose(uint32_t e, uint32_t f, uint32_t g)
    {
        return (e & f) ^ (~e & g);
    }

    /**
     * @brief 多数函数 Maj(a, b, c)
     * 返回a、b、c中多数位的值（至少两位为1则为1）
     */
    static uint32_t majority(uint32_t a, uint32_t b, uint32_t c)
    {
        return (a & (b | c)) | (b & c);
    }

    /**
     * @brief 消息扩展函数 Σ₀(x)
     * 用于消息扩展阶段的混淆操作
     */
    static uint32_t sig0(uint32_t x)
    {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }

    /**
     * @brief 消息扩展函数 Σ₁(x)
     * 用于消息扩展阶段的混淆操作（与sig0移位位数不同）
     */
    static uint32_t sig1(uint32_t x)
    {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }

    /**
     * @brief 核心变换：处理64字节数据块
     * 实现SHA-256压缩函数，包括消息扩展和64轮迭代更新状态
     */
    void transform()
    {
        uint32_t m[64] = {0};    // 消息字数组（16个原始 + 48个扩展）
        uint32_t state[8] = {0}; // 临时状态寄存器

        // 1. 64字节缓冲区转16个32位大端序消息字
        for (uint8_t i = 0, j = 0; i < 16; i++, j += 4)
        {
            m[i] = (static_cast<uint32_t>(m_data[j]) << 24) |
                   (static_cast<uint32_t>(m_data[j + 1]) << 16) |
                   (static_cast<uint32_t>(m_data[j + 2]) << 8) |
                   static_cast<uint32_t>(m_data[j + 3]);
        }

        // 2. 消息扩展：生成剩余48个消息字
        for (uint8_t k = 16; k < 64; k++)
        {
            m[k] = sig1(m[k - 2]) + m[k - 7] + sig0(m[k - 15]) + m[k - 16];
        }

        // 3. 初始化临时状态
        std::copy(m_state, m_state + 8, state);

        // 4. 64轮压缩循环
        for (uint8_t i = 0; i < 64; i++)
        {
            const uint32_t maj = majority(state[0], state[1], state[2]);
            const uint32_t xorA = rotr(state[0], 2) ^ rotr(state[0], 13) ^ rotr(state[0], 22);
            const uint32_t ch = choose(state[4], state[5], state[6]);
            const uint32_t xorE = rotr(state[4], 6) ^ rotr(state[4], 11) ^ rotr(state[4], 25);
            const uint32_t sum = m[i] + K[i] + state[7] + ch + xorE;

            // 状态轮转更新
            state[7] = state[6];
            state[6] = state[5];
            state[5] = state[4];
            state[4] = state[3] + sum;
            state[3] = state[2];
            state[2] = state[1];
            state[1] = state[0];
            state[0] = xorA + maj + sum;
        }

        // 5. 临时状态累加到主状态
        for (uint8_t i = 0; i < 8; i++)
        {
            m_state[i] += state[i];
        }
    }

    /**
     * @brief 数据填充：满足512位块对齐要求
     * 遵循SHA-256填充规则：加0x80 → 填0x00 → 存总比特数
     */
    void pad()
    {
        uint64_t i = m_blocklen;
        uint8_t end = (m_blocklen < 56) ? 56 : 64;

        // 步骤1：添加1个0x80（标记数据结束）
        m_data[i++] = 0x80;
        // 步骤2：填充0x00至剩余8字节
        while (i < end)
        {
            m_data[i++] = 0x00;
        }

        // 若缓冲区超56字节，先处理当前块
        if (m_blocklen >= 56)
        {
            transform();
            std::memset(m_data, 0, 56);
        }

        // 步骤3：存储总比特数（大端序）
        m_bitlen += m_blocklen * 8;
        m_data[56] = static_cast<uint8_t>(m_bitlen >> 56);
        m_data[57] = static_cast<uint8_t>(m_bitlen >> 48);
        m_data[58] = static_cast<uint8_t>(m_bitlen >> 40);
        m_data[59] = static_cast<uint8_t>(m_bitlen >> 32);
        m_data[60] = static_cast<uint8_t>(m_bitlen >> 24);
        m_data[61] = static_cast<uint8_t>(m_bitlen >> 16);
        m_data[62] = static_cast<uint8_t>(m_bitlen >> 8);
        m_data[63] = static_cast<uint8_t>(m_bitlen);

        transform(); // 处理最后一个块
    }

    /**
     * @brief 状态转换：32位寄存器 → 32字节数组
     * @param hash 输出参数，存储大端序哈希结果
     */
    void revert(std::array<uint8_t, 32> &hash)
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            for (uint8_t j = 0; j < 8; j++)
            {
                hash[i + (j * 4)] = static_cast<uint8_t>((m_state[j] >> (24 - i * 8)) & 0xFF);
            }
        }
    }
};

// 静态常量K显式初始化（确保编译期常量生效）
constexpr std::array<uint32_t, 64> SHA256::K;

#endif // SHA256_HPP