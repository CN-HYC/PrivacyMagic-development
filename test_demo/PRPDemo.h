#ifndef PRP_AES_DEMO_H
#define PRP_AES_DEMO_H

// 包含PRP_AES类的头文件（假设路径不变）
#include "../CryptoTools/PRP_AES.hpp"

/**
 * @brief PRP_AES类的演示函数，测试单个块和长数据的置换/逆置换功能
 * @return 0：测试成功；1：测试失败（抛出异常）
 */
int PRPDemo();

/**
 * @brief 辅助函数：以十六进制格式打印字节数组
 * @param data 待打印的字节数组
 * @param length 数组长度
 * @param label 可选标签（用于区分不同打印内容）
 */
void print_bytes(const unsigned char *data, size_t length, const std::string &label = "");

#endif // PRP_AES_DEMO_H