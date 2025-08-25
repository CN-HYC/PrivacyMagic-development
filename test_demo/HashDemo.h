// 防止头文件重复包含的保护宏
#ifndef HASHDEMO_H
#define HASHDEMO_H


// 引入哈希相关的头文件（根据实际文件路径调整，确保编译器能找到）
#include "../HashTools/SHA256.hpp"     // SHA256 哈希算法类
#include "../HashTools/CuckooHash.hpp"
#include "../HashTools/SimpleHash.hpp"
#include <iostream>
#include <string>

using namespace std;

/**
 * @brief 哈希功能演示函数
 *
 * 该函数整合 SHA256 哈希算法、简单哈希表、布谷鸟哈希表的核心功能，
 * 演示哈希值生成、键值对存储、查找、删除等操作，并输出关键信息供调试和验证。
 */
void hashdemo();

// 结束头文件保护宏
#endif // HASHDEMO_H