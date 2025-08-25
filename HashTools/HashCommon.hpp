#ifndef HASH_COMMON_HPP
#define HASH_COMMON_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <random>
#include <functional>
#include <cassert>

namespace Hashing
{

    // -------------------------- 哈希混合工具函数 ---------------------------
    /**
     * @brief SplitMix64哈希算法，用于生成高质量的伪随机数和哈希值
     * @param x 输入的64位无符号整数
     * @return 经过混合运算后的64位无符号整数
     */
    static inline std::uint64_t splitmix64(std::uint64_t x)
    {
        x += 0x9e3779b97f4a7c15ull;// 64位黄金比例常数
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;// 混合步骤1
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;// 混合步骤2
        return x ^ (x >> 31);
    }

    /**
     * @brief 将两个64位无符号整数混合，并转换为size_t类型
     * @param a 第一个输入的64位无符号整数
     * @param b 第二个输入的64位无符号整数
     * @return 混合后转换得到的size_t类型值（适配32/64位系统）
     */
    static inline std::size_t mix_to_size_t(std::uint64_t a, std::uint64_t b)
    {
        std::uint64_t x = splitmix64(a ^ (b + 0x9e3779b97f4a7c15ull + (a << 6) + (a >> 2)));
        // 根据当前系统size_t的字节数（4或8字节）进行适配转换
        if constexpr (sizeof(std::size_t) == 8)
            return static_cast<std::size_t>(x);
        else
            return static_cast<std::size_t>(x ^ (x >> 32));
    }

    // ------------------------------- 哈希函数族 ---------------------------
    /**
     * @brief 哈希函数族类，用于生成多个独立的哈希函数
     * @tparam Key 哈希函数处理的键类型
     */
    template <class Key>
    class HashFamily
    {
    public:
        /**
         * @brief 构造函数，初始化哈希函数族
         * @param k 哈希函数族中包含的哈希函数数量（默认值为3）
         * @param master_seed 主种子，用于生成所有哈希函数的独立种子（默认使用随机设备生成）
         */
        explicit HashFamily(std::size_t k = 3, std::uint64_t master_seed = std::random_device{}())
            : k_(k), seeds_(k)
        {
            std::uint64_t x = master_seed;
            // 为每个哈希函数生成独立的种子（基于主种子和函数索引）
            for (std::size_t i = 0; i < k_; ++i)
            {
                x = splitmix64(x + i);
                seeds_[i] = x;
            }
        }

        /**
         * @brief 获取哈希函数族中哈希函数的数量
         * @return 哈希函数数量
         * noexcept: 表示该函数不会抛出异常
         */
        std::size_t k() const noexcept { return k_; }

        /**
         * @brief 使用指定索引的哈希函数计算键的哈希值
         * @param i 哈希函数在族中的索引（需小于k()）
         * @param key 待计算哈希值的键
         * @return 键的哈希值（size_t类型）
         */
        std::size_t hash(std::size_t i, const Key &key) const
        {
            // 断言索引合法（防止越界访问）
            assert(i < k_);
            // 先通过标准哈希函数获取键的基础哈希值（转换为64位）
            std::uint64_t base = static_cast<std::uint64_t>(std::hash<Key>{}(key));
            // 结合当前哈希函数的种子进行混合，得到最终哈希值
            return mix_to_size_t(base, seeds_[i]);
        }

    private:
        std::size_t k_;                    // 哈希函数族中哈希函数的数量
        std::vector<std::uint64_t> seeds_; // 每个哈希函数对应的独立种子
    };

} // namespace Hashing

#endif // HASH_COMMON_HPP