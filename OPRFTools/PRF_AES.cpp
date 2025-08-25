#include "PRF_AES.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>

// AES轮常量
static const uint8_t RCON[11] = {0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

// 字节替换
static uint8_t sub_byte(uint8_t byte)
{
    // S盒
    static const uint8_t sbox[256] = {
        0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
        0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
        0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
        0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
        0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
        0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
        0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
        0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
        0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
        0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
        0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
        0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
        0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};
    return sbox[byte];
}

// 行移位
static void shift_rows(uint8_t *state)
{
    // 第二行左移1位
    std::swap(state[1], state[5]);
    std::swap(state[5], state[9]);
    std::swap(state[9], state[13]);

    // 第三行左移2位
    std::swap(state[2], state[10]);
    std::swap(state[6], state[14]);

    // 第四行左移3位
    std::swap(state[15], state[11]);
    std::swap(state[11], state[7]);
    std::swap(state[7], state[3]);
}

// 列混合
static void mix_columns(uint8_t *state)
{
    auto gmul = [](uint8_t a, uint8_t b)
    {
        uint8_t p = 0;
        for (int i = 0; i < 8; ++i)
        {
            if (b & 1)
                p ^= a;
            bool hi = a & 0x80;
            a <<= 1;
            if (hi)
                a ^= 0x1b; // 多项式x^8 + x^4 + x^3 + x + 1
            b >>= 1;
        }
        return p;
    };

    for (int i = 0; i < 4; ++i)
    {
        int col = i * 4;
        uint8_t s0 = state[col];
        uint8_t s1 = state[col + 1];
        uint8_t s2 = state[col + 2];
        uint8_t s3 = state[col + 3];

        state[col] = gmul(s0, 2) ^ gmul(s1, 3) ^ s2 ^ s3;
        state[col + 1] = s0 ^ gmul(s1, 2) ^ gmul(s2, 3) ^ s3;
        state[col + 2] = s0 ^ s1 ^ gmul(s2, 2) ^ gmul(s3, 3);
        state[col + 3] = gmul(s0, 3) ^ s1 ^ s2 ^ gmul(s3, 2);
    }
}

// 密钥扩展
static void key_expansion(const uint8_t *key, size_t key_len, uint32_t *round_keys, int &rounds)
{
    // 根据密钥长度确定轮数
    if (key_len == 16)
        rounds = 10; // 128位密钥
    else if (key_len == 24)
        rounds = 12; // 192位密钥
    else if (key_len == 32)
        rounds = 14; // 256位密钥
    else
        throw std::invalid_argument("无效的AES密钥长度，必须是16、24或32字节");

    // 初始化轮密钥
    int n = key_len / 4; // 密钥字数(4字节为1字)
    int total_words = (rounds + 1) * 4;

    // 复制原始密钥到轮密钥
    for (int i = 0; i < n; ++i)
    {
        round_keys[i] = (key[4 * i] << 24) | (key[4 * i + 1] << 16) | (key[4 * i + 2] << 8) | key[4 * i + 3];
    }

    // 生成剩余轮密钥
    for (int i = n; i < total_words; ++i)
    {
        uint32_t temp = round_keys[i - 1];

        if (i % n == 0)
        {
            // RotWord和SubWord变换
            temp = ((temp << 8) | (temp >> 24)) & 0xFFFFFFFF;
            uint8_t *p = reinterpret_cast<uint8_t *>(&temp);
            p[0] = sub_byte(p[0]);
            p[1] = sub_byte(p[1]);
            p[2] = sub_byte(p[2]);
            p[3] = sub_byte(p[3]);

            // Rcon异或
            temp ^= (RCON[i / n] << 24);
        }
        // 256位密钥额外处理
        else if (n > 6 && i % n == 4)
        {
            uint8_t *p = reinterpret_cast<uint8_t *>(&temp);
            p[0] = sub_byte(p[0]);
            p[1] = sub_byte(p[1]);
            p[2] = sub_byte(p[2]);
            p[3] = sub_byte(p[3]);
        }

        round_keys[i] = round_keys[i - n] ^ temp;
    }
}

// 轮密钥加
static void add_round_key(uint8_t *state, const uint32_t *round_keys, int round)
{
    const uint32_t *rk = &round_keys[round * 4];

    for (int i = 0; i < 4; ++i)
    {
        uint32_t word = rk[i];
        state[i * 4] ^= (word >> 24) & 0xFF;
        state[i * 4 + 1] ^= (word >> 16) & 0xFF;
        state[i * 4 + 2] ^= (word >> 8) & 0xFF;
        state[i * 4 + 3] ^= word & 0xFF;
    }
}

// 初始化AES上下文
void PRF_AES::aes_init(const uint8_t *key, size_t key_len)
{
    key_expansion(key, key_len, ctx_.round_keys, ctx_.rounds);
}

// AES加密单块数据
void PRF_AES::aes_encrypt(const uint8_t *input, uint8_t *output)
{
    uint8_t state[16];
    memcpy(state, input, 16);

    // 初始轮密钥加
    add_round_key(state, ctx_.round_keys, 0);

    // 主加密循环
    for (int round = 1; round < ctx_.rounds; ++round)
    {
        // 字节替换
        for (int i = 0; i < 16; ++i)
        {
            state[i] = sub_byte(state[i]);
        }

        // 行移位
        shift_rows(state);

        // 列混合(最后一轮不执行)
        if (round < ctx_.rounds - 1)
        {
            mix_columns(state);
        }

        // 轮密钥加
        add_round_key(state, ctx_.round_keys, round);
    }

    memcpy(output, state, 16);
}

// 构造函数
PRF_AES::PRF_AES(const std::string &key) : key_(key), key_len_(key.size())
{
    if (key_len_ != 16 && key_len_ != 24 && key_len_ != 32)
    {
        throw std::invalid_argument("AES密钥长度必须是16字节(128位)、24字节(192位)或32字节(256位)");
    }

    aes_init(reinterpret_cast<const uint8_t *>(key.data()), key_len_);
}

// 析构函数，清除敏感数据
PRF_AES::~PRF_AES()
{
    // 清除密钥和轮密钥，防止内存中残留
    std::fill(key_.begin(), key_.end(), 0);
    std::fill(std::begin(ctx_.round_keys), std::end(ctx_.round_keys), 0);
}

// 计算PRF(key, input)，输入为字符串
std::string PRF_AES::evaluate(const std::string &input)
{
    return evaluate(reinterpret_cast<const uint8_t *>(input.data()), input.size());
}

// 计算PRF(key, input)，输入为字节数组
std::string PRF_AES::evaluate(const uint8_t *input, size_t input_len)
{
    // 对于变长输入，我们使用CBC-MAC的方式处理
    // 初始向量设为0
    uint8_t block[16] = {0};
    size_t pos = 0;

    // 处理完整的块
    while (pos + 16 <= input_len)
    {
        // 输入块与当前状态异或
        for (int i = 0; i < 16; ++i)
        {
            block[i] ^= input[pos + i];
        }

        // 加密
        aes_encrypt(block, block);
        pos += 16;
    }

    // 处理剩余数据(填充)
    if (pos < input_len)
    {
        // 输入块与当前状态异或
        for (size_t i = 0; i < input_len - pos; ++i)
        {
            block[i] ^= input[pos + i];
        }

        // 最后一个字节设置为填充长度
        block[input_len - pos] ^= 0x80; // 填充10000000
        aes_encrypt(block, block);
    }

    return std::string(reinterpret_cast<char *>(block), 16);
}
