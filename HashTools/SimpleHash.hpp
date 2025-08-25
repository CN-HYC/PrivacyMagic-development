#ifndef SIMPLE_HASH_HPP
#define SIMPLE_HASH_HPP

#include "HashCommon.hpp"
#include <iostream>
#include <vector>
#include <list>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <memory>

namespace Hashing
{

    template <class Key, class T>
    class SimpleHash
    {
    public:
        using value_type = std::pair<Key, T>;

        // 构造函数：确保哈希函数数量>=3（适配需求）
        explicit SimpleHash(std::shared_ptr<const HashFamily<Key>> family,
                            std::size_t initial_buckets = 16)
            : family_(std::move(family))
        {
            // 新增：要求哈希函数数量至少为3（满足“三个位置”的需求）
            if (!family_ || family_->k() < 3)
                throw std::invalid_argument("HashFamily must be non-null and k>=3");
            buckets_.resize(std::max<std::size_t>(initial_buckets, 1));
        }

        /**
         * 插入逻辑修改：遍历3个哈希函数，在每个对应位置插入/更新键值对
         * 仅当所有位置都没有该键时，才视为“新插入”（sz_+1）
         */
        bool insert(const Key &key, const T &value)
        {
            if (load_factor() > 0.75)
                rehash(buckets_.size() * 2);

            bool is_new_insert = false;
            // 遍历所有3个哈希函数，计算每个对应的桶位置
            for (std::size_t h_idx = 0; h_idx < family_->k(); ++h_idx)
            {
                // 计算当前哈希函数对应的桶索引
                std::size_t bucket_idx = family_->hash(h_idx, key) % buckets_.size();
                auto &chain = buckets_[bucket_idx];

                // 检查该桶中是否已有该键：有则更新值，无则插入
                bool found = false;
                for (auto &kv : chain)
                {
                    if (kv.first == key)
                    {
                        kv.second = value;
                        found = true;
                        break;
                    }
                }
                // 若当前桶无该键，插入新键值对
                if (!found)
                {
                    chain.emplace_back(key, value);
                    // 仅首次插入时计数（避免3个位置都插导致sz_+3）
                    if (!is_new_insert)
                    {
                        is_new_insert = true;
                        ++sz_;
                    }
                }
            }
            // 返回是否为新插入（而非更新）
            return is_new_insert;
        }

        /**
         * 删除逻辑修改：遍历3个哈希函数，删除所有位置的该键副本
         * 仅当至少一个位置有该键时，视为“删除成功”（sz_-1）
         */
        bool erase(const Key &key)
        {
            bool is_erased = false;
            // 遍历所有3个哈希函数，删除每个位置的键
            for (std::size_t h_idx = 0; h_idx < family_->k(); ++h_idx)
            {
                std::size_t bucket_idx = family_->hash(h_idx, key) % buckets_.size();
                auto &chain = buckets_[bucket_idx];

                for (auto it = chain.begin(); it != chain.end(); ++it)
                {
                    if (it->first == key)
                    {
                        chain.erase(it);
                        // 仅首次删除时计数（避免3个位置都删导致sz_-3）
                        if (!is_erased)
                        {
                            is_erased = true;
                            --sz_;
                        }
                        break; // 一个桶中只会有一个该键，删除后退出循环
                    }
                }
            }
            return is_erased;
        }

        /**
         * 查找逻辑优化：遍历3个哈希函数，任意位置找到即返回（提高效率）
         */
        T *find(const Key &key)
        {
            for (std::size_t h_idx = 0; h_idx < family_->k(); ++h_idx)
            {
                std::size_t bucket_idx = family_->hash(h_idx, key) % buckets_.size();
                auto &chain = buckets_[bucket_idx];
                for (auto &kv : chain)
                {
                    if (kv.first == key)
                        return &kv.second; // 任意位置命中即返回
                }
            }
            return nullptr;
        }

        const T *find(const Key &key) const
        {
            return const_cast<SimpleHash *>(this)->find(key);
        }

        bool contains(const Key &key) const
        {
            return find(key) != nullptr;
        }

        /**
         * 下标操作修改：在3个位置都插入默认值（保持多位置存储一致性）
         */
        T &operator[](const Key &key)
        {
            if (load_factor() > 0.75)
                rehash(buckets_.size() * 2);

            T *found_val = find(key);
            if (found_val)
                return *found_val; // 已有则返回任意位置的值

            // 无该键：在3个位置都插入默认值，返回第一个位置的值
            bool is_new = false;
            T *ret_val = nullptr;
            for (std::size_t h_idx = 0; h_idx < family_->k(); ++h_idx)
            {
                std::size_t bucket_idx = family_->hash(h_idx, key) % buckets_.size();
                auto &chain = buckets_[bucket_idx];
                chain.emplace_back(key, T{});
                // 仅首次插入时计数，并记录返回值地址
                if (!is_new)
                {
                    is_new = true;
                    ++sz_;
                    ret_val = &chain.back().second;
                }
            }
            return *ret_val;
        }

        std::size_t size() const noexcept { return sz_; }
        std::size_t bucket_count() const noexcept { return buckets_.size(); }
        double load_factor() const noexcept { return buckets_.empty() ? 0.0 : double(sz_) / double(buckets_.size()); }

        /**
         * 清空逻辑修改：清空所有桶的所有元素
         */
        void clear()
        {
            for (auto &b : buckets_)
                b.clear();
            sz_ = 0;
        }

        /**
         * 重哈希逻辑修改：遍历所有元素，在新桶的3个位置重新插入
         */
        void rehash(std::size_t new_bucket_count)
        {
            new_bucket_count = std::max<std::size_t>(1, new_bucket_count);
            std::vector<std::list<value_type>> new_buckets(new_bucket_count);
            std::size_t old_sz = sz_;
            sz_ = 0; // 重置计数，后续重新插入时重新计数

            // 遍历原桶的所有元素（注意去重：同一键只处理一次）
            std::vector<value_type> unique_elements;
            for (auto &chain : buckets_)
            {
                for (auto &kv : chain)
                {
                    // 检查是否已收集过该键（避免重复处理）
                    bool exists = false;
                    for (auto &elem : unique_elements)
                    {
                        if (elem.first == kv.first)
                        {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists)
                        unique_elements.emplace_back(std::move(kv));
                }
            }

            // 对每个唯一元素，在新桶的3个位置插入
            for (auto &kv : unique_elements)
            {
                bool is_new = false;
                for (std::size_t h_idx = 0; h_idx < family_->k(); ++h_idx)
                {
                    std::size_t new_bucket_idx = family_->hash(h_idx, kv.first) % new_bucket_count;
                    new_buckets[new_bucket_idx].emplace_back(kv.first, kv.second);
                    if (!is_new)
                    {
                        is_new = true;
                        ++sz_;
                    }
                }
            }

            buckets_.swap(new_buckets);
            assert(sz_ == old_sz); // 确保计数一致
        }

        /**
         * 打印逻辑：显示3个实际存储位置（而非仅“可能位置”）
         */
        void print(std::ostream &os = std::cout, bool detailed = true) const
        {
            os << "SimpleHash Structure (3 hash functions, 3 storage positions):" << std::endl;
            os << "------------------------------------------------------------" << std::endl;
            os << "Bucket count: " << buckets_.size() << std::endl;
            os << "Element count (logical): " << sz_ << std::endl;
            os << "Physical storage count: " << sz_ * family_->k() << " (1 element = " << family_->k() << " copies)" << std::endl;
            os << "Load factor (logical): " << load_factor() << std::endl;

            if (detailed)
            {
                os << "Buckets (each element exists in 3 buckets):" << std::endl;
                for (std::size_t i = 0; i < buckets_.size(); ++i)
                {
                    const auto &chain = buckets_[i];
                    os << "  Bucket " << i << " (" << chain.size() << " elements): ";
                    if (chain.empty())
                    {
                        os << "empty";
                    }
                    else
                    {
                        bool first = true;
                        for (const auto &kv : chain)
                        {
                            if (!first)
                                os << " -> ";
                            os << "{" << kv.first << ": " << kv.second << "}";
                            first = false;
                        }
                    }
                    os << std::endl;
                }
            }
            os << "------------------------------------------------------------" << std::endl;
        }

    private:
        std::shared_ptr<const HashFamily<Key>> family_;
        std::vector<std::list<value_type>> buckets_; // 桶数组（每个桶是链表）
        std::size_t sz_ = 0;                         // 逻辑元素数量（1个键算1个，无论存几份）
    };

} // namespace hashing

#endif // SIMPLE_HASH_HPP