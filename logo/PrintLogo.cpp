#include "PrintLogo.h"

// 打印MPC字符图形，每个字母10行高，增加内部和字符间空隙
void printLargeMPC()
{
    // 顶部边框
    std::cout << std::string(60, '=') << "\n";
    std::cout << "     Welcome to CryptoMagic! This is a MPC framwork!" << "\n\n";
    // 第1行
    std::cout << "  *****     *****           ******              ******  \n";
    // 第2行
    std::cout << " *     *   *     *         *      *          *          \n";
    // 第3行
    std::cout << " *     *   *     *         *      *          *        \n";
    // 第4行
    std::cout << " *     *   *     *         *      *          *        \n";
    // 第5行
    std::cout << " *     *   *     *         *      *          *             \n";
    // 第6行
    std::cout << " *       *       *         ********          *        \n";
    // 第7行
    std::cout << " *       *       *         *                 *        \n";
    // 第8行
    std::cout << " *       *       *         *                 *         \n";
    // 第9行
    std::cout << " *       *       *         *                 *             \n";
    // 第10行
    std::cout << " *       *       *         *                    ******         \n";

    // 底部边框
    std::cout << "\n"
              << std::string(60, '=') << std::endl;
}

