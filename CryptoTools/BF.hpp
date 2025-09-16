#ifndef BLOOM_FILTER_HPP
#define BLOOM_FILTER_HPP

#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <stdexcept>
namespace CryptoTools
{

    // 布隆过滤器类模板，支持任意类型元素
    template <typename T>
    class BloomFilter
    {
    private:
        std::vector<bool> bit_array; // 位数组，存储元素存在标记
        size_t bit_size;             // 位数组大小（位）
        size_t hash_count;           // 哈希函数数量
        size_t item_count;           // 已插入元素数量

        // 哈希函数1：基于DJB2算法
        uint64_t hash1(const T &item) const
        {
            std::string str = std::to_string(item); // 对于非字符串类型转换为字符串
            uint64_t hash = 5381;
            for (char c : str)
            {
                hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
            }
            return hash % bit_size;
        }

        // 哈希函数2：基于SDBM算法
        uint64_t hash2(const T &item) const
        {
            std::string str = std::to_string(item);
            uint64_t hash = 0;
            for (char c : str)
            {
                hash = static_cast<uint64_t>(c) + (hash << 6) + (hash << 16) - hash;
            }
            return hash % bit_size;
        }

        // 哈希函数3：基于FNV-1a算法
        uint64_t hash3(const T &item) const
        {
            std::string str = std::to_string(item);
            uint64_t hash = 14695981039346656037ULL;
            for (char c : str)
            {
                hash ^= static_cast<uint64_t>(c);
                hash *= 1099511628211ULL;
            }
            return hash % bit_size;
        }

        // 计算第i个哈希值（组合基础哈希函数）
        uint64_t get_hash(const T &item, size_t i) const
        {
            return (hash1(item) + i * hash2(item) + i * i * hash3(item)) % bit_size;
        }

    public:
        /**
         * @brief 构造布隆过滤器
         * @param expected_items 预期插入的元素数量
         * @param false_positive_rate 可接受的误判率
         */
        BloomFilter(size_t expected_items, double false_positive_rate, size_t hash_count): hash_count(hash_count)
        {
            if (expected_items == 0)
            {
                throw std::invalid_argument("预期元素数量不能为0");
            }
            if (false_positive_rate <= 0 || false_positive_rate >= 1)
            {
                throw std::invalid_argument("误判率必须在(0, 1)范围内");
            }

            // 计算最佳位数组大小：m = -n * ln(p) / (ln(2)^2)
            bit_size = static_cast<size_t>(-(expected_items * std::log(false_positive_rate)) / (std::log(2) * std::log(2)));
            if (bit_size == 0)
                bit_size = 1; // 确保至少有1位

            // 计算最佳哈希函数数量：k = m/n * ln(2)
            //hash_count = static_cast<size_t>(std::round((bit_size / expected_items) * std::log(2)));
            if (hash_count == 0)
                hash_count = 1; // 确保至少有1个哈希函数
            std::cout <<"hash_count: "<<hash_count<<std::endl;
            // 初始化位数组
            bit_array.resize(bit_size, false);
            item_count = 0;
        }

        /**
         * @brief 插入元素到布隆过滤器
         * @param item 要插入的元素
         */
        void insert(const T &item)
        {
            for (size_t i = 0; i < hash_count; ++i)
            {
                size_t position = get_hash(item, i);
                bit_array[position] = true;
            }
            item_count++;
        }

        /**
         * @brief 检查元素是否可能存在于布隆过滤器中
         * @param item 要检查的元素
         * @return true：可能存在（有一定误判率）；false：一定不存在
         */
        bool contains(const T &item) const
        {
            for (size_t i = 0; i < hash_count; ++i)
            {
                size_t position = get_hash(item, i);
                if (!bit_array[position])
                {
                    return false; // 只要有一位为0，肯定不存在
                }
            }
            return true; // 所有位都为1，可能存在
        }

        /**
         * @brief 获取位数组大小（位）
         */
        size_t get_bit_size() const
        {
            return bit_size;
        }

        /**
         * @brief 获取哈希函数数量
         */
        size_t get_hash_count() const
        {
            return hash_count;
        }

        /**
         * @brief 获取已插入元素数量
         */
        size_t get_item_count() const
        {
            return item_count;
        }

        /**
         * @brief 清空布隆过滤器
         */
        void clear()
        {
            std::fill(bit_array.begin(), bit_array.end(), false);
            item_count = 0;
        }
    };

    // 针对字符串类型的特化，优化哈希计算
    template <>
    inline uint64_t BloomFilter<std::string>::hash1(const std::string &item) const
    {
        uint64_t hash = 5381;
        for (char c : item)
        {
            hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
        }
        return hash % bit_size;
    }

    template <>
    inline uint64_t BloomFilter<std::string>::hash2(const std::string &item) const
    {
        uint64_t hash = 0;
        for (char c : item)
        {
            hash = static_cast<uint64_t>(c) + (hash << 6) + (hash << 16) - hash;
        }
        return hash % bit_size;
    }

    template <>
    inline uint64_t BloomFilter<std::string>::hash3(const std::string &item) const
    {
        uint64_t hash = 14695981039346656037ULL;
        for (char c : item)
        {
            hash ^= static_cast<uint64_t>(c);
            hash *= 1099511628211ULL;
        }
        return hash % bit_size;
    }
}
#endif // BLOOM_FILTER_HPP
