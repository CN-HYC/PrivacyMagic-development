#ifndef CUCKOO_HASH_HPP
#define CUCKOO_HASH_HPP

#include "HashCommon.hpp"
#include <iostream>
#include <vector>
#include <optional>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <memory>
#include <limits>
#include <cassert>

namespace HashTools
{

    // ------------------------------- CuckooHash ---------------------------------
    template <class Key, class T>
    class CuckooHash
    {
    public:
        /**
         * @brief 构造函数
         * @param family 哈希函数族
         * @param initial_capacity 初始容量
         * @param max_displacements 最大位移次数
         */
        explicit CuckooHash(std::shared_ptr<const HashFamily<Key>> family,
                            std::size_t initial_capacity = 16,
                            std::size_t max_displacements = 500)
            : family_(std::move(family)),
              max_displacements_(max_displacements)
        {
            if (!family_ || family_->k() < 2)
                throw std::invalid_argument("HashFamily must be non-null and k>=2 for cuckoo hashing");
            capacity_ = std::max<std::size_t>(initial_capacity, 2);
            table_.resize(capacity_);
        }

        /**
         * @brief 插入键值对
         * @param key 键
         * @param value 值
         * @return 是否插入成功（true表示新插入，false表示更新）
         */
        bool insert(const Key &key, const T &value)
        {
            if (load_factor() > 0.5)
                resize(capacity_ * 2);
            return do_insert(key, value);
        }

        /**
         * @brief 删除键值对
         * @param key 要删除的键
         * @return 是否删除成功
         */
        bool erase(const Key &key)
        {
            for (std::size_t i = 0; i < family_->k(); ++i)
            {
                std::size_t idx = position(i, key);
                if (table_[idx] && table_[idx]->key == key)
                {
                    table_[idx].reset();
                    --sz_;
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief 查找键对应的值
         * @param key 要查找的键
         * @return 值的指针，如果不存在返回nullptr
         */
        T *find(const Key &key)
        {
            for (std::size_t i = 0; i < family_->k(); ++i)
            {
                std::size_t idx = position(i, key);
                if (table_[idx] && table_[idx]->key == key)
                    return &table_[idx]->value;
            }
            return nullptr;
        }

        /**
         * @brief 查找键对应的值（常量版本）
         * @param key 要查找的键
         * @return 值的常量指针，如果不存在返回nullptr
         */
        const T *find(const Key &key) const
        {
            return const_cast<CuckooHash *>(this)->find(key);
        }

        /**
         * @brief 检查是否包含键
         * @param key 要检查的键
         * @return 是否包含
         */
        bool contains(const Key &key) const
        {
            return find(key) != nullptr;
        }

        /**
         * @brief 获取元素数量
         * @return 元素数量
         */
        std::size_t size() const noexcept
        {
            return sz_;
        }

        /**
         * @brief 获取容量
         * @return 容量大小
         */
        std::size_t capacity() const noexcept
        {
            return capacity_;
        }

        /**
         * @brief 计算负载因子
         * @return 负载因子值
         */
        double load_factor() const noexcept
        {
            return capacity_ ? double(sz_) / double(capacity_) : 0.0;
        }

        /**
         * @brief 清空哈希表
         */
        void clear()
        {
            for (auto &cell : table_)
                cell.reset();
            sz_ = 0;
        }

        /**
         * @brief 调整哈希表大小
         * @param new_capacity 新的容量
         */
        void resize(std::size_t new_capacity)
        {
            new_capacity = std::max<std::size_t>(2, new_capacity);
            std::vector<std::optional<Entry>> old;
            old.swap(table_);
            std::size_t old_sz = sz_;
            capacity_ = new_capacity;
            table_.assign(capacity_, std::nullopt);
            sz_ = 0;
            for (auto &cell : old)
                if (cell)
                    do_insert(cell->key, cell->value);
            assert(sz_ == old_sz);
        }

        /**
         * @brief 打印哈希表结构
         * @param os 输出流
         * @param detailed 是否打印详细信息
         */
        void print(std::ostream &os = std::cout, bool detailed = true) const
        {
            os << "CuckooHash Structure:" << std::endl;
            os << "---------------------" << std::endl;
            os << "Capacity: " << capacity_ << std::endl;
            os << "Element count: " << sz_ << std::endl;
            os << "Load factor: " << load_factor() << std::endl;
            os << "Number of hash functions: " << family_->k() << std::endl;
            os << "Max displacements: " << max_displacements_ << std::endl;

            if (detailed)
            {
                os << "Table entries:" << std::endl;
                for (std::size_t i = 0; i < capacity_; ++i)
                {
                    os << "  Index " << i << ": ";
                    if (table_[i])
                    {
                        os << "{" << table_[i]->key << ": " << table_[i]->value << "}";

                        // 显示该键的所有可能位置
                        os << " (possible positions: ";
                        for (std::size_t h = 0; h < family_->k(); ++h)
                        {
                            if (h > 0)
                                os << ", ";
                            os << position(h, table_[i]->key);
                        }
                        os << ")";
                    }
                    else
                    {
                        os << "empty";
                    }
                    os << std::endl;
                }
            }
            os << "---------------------" << std::endl;
        }

    private:
        struct Entry
        {
            Key key;
            T value;
        };

        /**
         * @brief 计算键在指定哈希函数下的位置
         * @param hash_idx 哈希函数索引
         * @param key 键
         * @return 位置索引
         */
        std::size_t position(std::size_t hash_idx, const Key &key) const
        {
            return family_->hash(hash_idx, key) % capacity_;
        }

        /**
         * @brief 实际插入操作
         * @param key 键
         * @param value 值
         * @return 是否插入成功
         */
        bool do_insert(const Key &key, const T &value)
        {
            // 检查键是否已存在
            for (std::size_t i = 0; i < family_->k(); ++i)
            {
                std::size_t idx = position(i, key);
                if (table_[idx] && table_[idx]->key == key)
                {
                    table_[idx]->value = value;
                    return false;
                }
            }

            Entry cur{key, value};
            std::size_t idx = npos;

            // 尝试直接插入到空位置
            for (std::size_t i = 0; i < family_->k(); ++i)
            {
                idx = position(i, cur.key);
                if (!table_[idx])
                {
                    table_[idx] = cur;
                    ++sz_;
                    return true;
                }
            }

            // 使用踢出策略
            std::size_t which = 0;
            for (std::size_t disp = 0; disp < max_displacements_; ++disp)
            {
                which = (which + 1) % family_->k();
                idx = position(which, cur.key);
                std::swap(cur, *table_[idx]);

                // 检查被踢出的元素是否有空位
                for (std::size_t i = 0; i < family_->k(); ++i)
                {
                    std::size_t alt = position(i, cur.key);
                    if (!table_[alt])
                    {
                        table_[alt] = cur;
                        ++sz_;
                        return true;
                    }
                }
            }

            // 超过最大位移次数，扩容后重新插入
            resize(capacity_ * 2);
            return do_insert(cur.key, cur.value);
        }

        static constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

        std::shared_ptr<const HashFamily<Key>> family_;
        std::size_t capacity_ = 0;
        std::vector<std::optional<Entry>> table_;
        std::size_t sz_ = 0;
        std::size_t max_displacements_;
    };

} // namespace hashing

#endif // CUCKOO_HASH_HPP
