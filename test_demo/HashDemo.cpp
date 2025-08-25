#include "HashDemo.h"
#include <chrono>

// 字符串转哈希值（整数）
int stringToHash(const std::string &str)
{
    // 使用标准哈希函数生成size_t，再转换为int（可能有碰撞，但适合哈希表场景）
    std::hash<std::string> hasher;
    return static_cast<int>(hasher(str) % INT_MAX); // 取模避免溢出
}

void hashdemo()
{
    // 创建共享的哈希函数族
    auto family = std::make_shared<Hashing::HashFamily<int>>(3);

    // 测试 SimpleHash
    std::cout << "=== Testing SimpleHash ===\n";
    Hashing::SimpleHash<int, std::string> simpleHash(family,10);
    // std::cout << "Inserting 1,000,000 items into SimpleHash...\n";
    // // 计时开始
    // auto start = std::chrono::high_resolution_clock::now();
    // for (int i = 0; i < 1000000; i++)
    // {
    //     RO.input(std::to_string(i));
    //     simpleHash.insert(i, std::to_string(i));
    // }
    // // 计时结束
    // auto end = std::chrono::high_resolution_clock::now();
    // std::chrono::duration<double> elapsed = end - start;
    // std::cout << "SimpleHash insert time: " << elapsed.count() << " seconds\n";

    simpleHash.insert(stringToHash("Alice"), "Alice");
    simpleHash.insert(stringToHash("Bob"), "Bob");
    simpleHash.insert(stringToHash("ming"), "ming");
    if (auto value = simpleHash.find(2))
    {
        std::cout << "Found in SimpleHash: " << *value << std::endl;
    }

    simpleHash.print(std::cout);

    // 测试 CuckooHash
    std::cout << "\n=== Testing CuckooHash ===\n";
    Hashing::CuckooHash<int, std::string> cuckooHash(family,10);
    // std::cout << "Inserting 1,000,000 items into CuckooHash...\n";
    // // 计时开始
    // start = std::chrono::high_resolution_clock::now();
    // for (int i = 0; i < 1000000; i++)
    // {
    //     RO.input(std::to_string(i));
    //     cuckooHash.insert(i, std::to_string(i));
    // }
    // // 计时结束
    // end = std::chrono::high_resolution_clock::now();
    // elapsed = end - start;  
    // std::cout << "CuckooHash insert time: " << elapsed.count() << " seconds\n";

    cuckooHash.insert(stringToHash("Alice"), "Alice");
    cuckooHash.insert(stringToHash("Bob"), "Bob");
    cuckooHash.insert(stringToHash("Charlie"), "Charlie");

    if (auto value = cuckooHash.find(2))
    {
        std::cout << "Found in CuckooHash: " << *value << std::endl;
    }

    cuckooHash.print(std::cout);
}