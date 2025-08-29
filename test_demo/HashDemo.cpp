#include "HashDemo.h"
#include <chrono>

// 字符串转哈希值（整数）
/**
 * 将字符串转换为哈希值
 * @param str 输入的字符串
 * @return 返回字符串对应的哈希值，类型为int
 */
int stringToHash(const std::string &str)
{
    // 使用标准哈希函数生成size_t，再转换为int（可能有碰撞，但适合哈希表场景）
    std::hash<std::string> hasher;
    return static_cast<int>(hasher(str) % INT_MAX); // 取模避免溢出
}

/**
 * @brief 演示哈希表的使用，包括 SimpleHash 和 CuckooHash 的插入、查找和打印操作。
 * 
 * 该函数创建一个共享的哈希函数族，并使用它来初始化 SimpleHash 和 CuckooHash。
 * 然后分别向两个哈希表中插入一些键值对，并尝试查找特定键的值，最后打印哈希表内容。
 * 
 * @note 本函数不接受任何参数，也不返回任何值。
 */
void hashTableDemo()
{
    // 创建共享的哈希函数族，参数3表示哈希函数的数量
    auto family = std::make_shared<HashTools::HashFamily<int>>(3);

    // 测试 SimpleHash
    std::cout << "=== Testing SimpleHash ===\n";
    HashTools::SimpleHash<int, std::string> simpleHash(family,10);
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

    // 向 SimpleHash 插入键值对
    simpleHash.insert(stringToHash("Alice"), "Alice");
    simpleHash.insert(stringToHash("Bob"), "Bob");
    simpleHash.insert(stringToHash("ming"), "ming");

    // 在 SimpleHash 中查找键为2的值
    if (auto value = simpleHash.find(stringToHash("Alice")))
    {
        std::cout << "Found in SimpleHash: " << *value << std::endl;
    }

    // 打印 SimpleHash 的内容
    simpleHash.print(std::cout);

    // 测试 CuckooHash
    std::cout << "\n=== Testing CuckooHash ===\n";
    HashTools::CuckooHash<int, std::string> cuckooHash(family,10);
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

    // 向 CuckooHash 插入键值对
    cuckooHash.insert(stringToHash("Alice"), "Alice");
    cuckooHash.insert(stringToHash("Bob"), "Bob");
    cuckooHash.insert(stringToHash("Charlie"), "Charlie");

    // 在 CuckooHash 中查找键为2的值
    if (auto value = cuckooHash.find(stringToHash("Alice")))
    {
        std::cout << "Found in CuckooHash: " << *value << std::endl;
    }

    // 打印 CuckooHash 的内容
    cuckooHash.print(std::cout);
}

/**
 * @brief 演示SHA256哈希算法的使用
 * 
 * 该函数创建一个SHA256对象，对字符串"Hello, World!"进行哈希计算，
 * 并输出原始输入和对应的SHA256哈希值。
 * 
 * @param 无
 * @return 无
 */
void SHA256Demo()
{
    SHA256 sha256;// 创建一个SHA256对象
    
    // 输入待哈希的字符串
    sha256.input("Hello, World!");
    
    // 输出SHA256哈希结果
    std::cout << "SHA256 hash of \"Hello, World!\": " << sha256.output() << std::endl; 
}
void hashdemo()
{
    std::cout << "=== SHA256 Hash Demo ===\n";
    SHA256Demo();// SHA256测试

    std::cout << "\n=== Cuckoo Hash Table and Simple Hash Table Demo ===\n";
    hashTableDemo();// Cuckoo哈希表和Simple哈希表测试
}