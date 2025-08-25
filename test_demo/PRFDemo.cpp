#include "PRFDemo.h"

// 打印十六进制字符串
void print_hex(const std::string &data, const std::string &label = "")
{
    if (!label.empty())
    {
        std::cout << label << ": ";
    }
    for (unsigned char c : data)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    std::cout << std::dec << std::endl;
}

int PRFdemo()
{
    try
    {
        // 128位密钥(16字节)
        std::string key = "xQ72kP9aF3sR5dZ8"; // 注意：实际应用中应使用真正随机的密钥

        // 创建PRF实例
        PRF_AES prf(key);

        // 测试相同输入产生相同输出
        std::string input1 = "test input 1";
        std::string output1 = prf.evaluate(input1);
        std::string output1_again = prf.evaluate(input1);

        print_hex(input1, "输入1 (ASCII)");
        print_hex(output1, "输出1");
        print_hex(output1_again, "再次输出1");

        if (output1 == output1_again)
        {
            std::cout << "验证：相同输入产生相同输出 ✅" << std::endl;
        }
        else
        {
            std::cout << "错误：相同输入产生不同输出 ❌" << std::endl;
        }

        // 测试不同输入产生不同输出
        std::string input2 = "test input 2";
        std::string output2 = prf.evaluate(input2);

        print_hex(input2, "\n输入2 (ASCII)");
        print_hex(output2, "输出2");

        if (output1 != output2)
        {
            std::cout << "验证：不同输入产生不同输出 ✅" << std::endl;
        }
        else
        {
            std::cout << "警告：不同输入产生相同输出 ❌" << std::endl;
        }

        // 测试不同密钥下相同输入产生不同输出
        std::string key2 = "xQ72kP9aF3sR5dZ5";
        PRF_AES prf2(key2);
        std::string output1_key2 = prf2.evaluate(input1);

        print_hex(output1_key2, "\n使用不同密钥的输出1");

        if (output1 != output1_key2)
        {
            std::cout << "验证：不同密钥下相同输入产生不同输出 ✅" << std::endl;
        }
        else
        {
            std::cout << "警告：不同密钥下相同输入产生相同输出 ❌" << std::endl;
        }

        // 测试长输入
        std::string long_input(1024, 'A'); // 1024字节的输入
        std::string long_output = prf.evaluate(long_input);
        print_hex(long_output, "\n长输入的输出");
    }
    catch (const std::exception &e)
    {
        std::cerr << "错误：" << e.what() << std::endl;
        return 1;
    }

    return 0;
}
