#ifndef DH_RECEIVER_H
#define DH_RECEIVER_H

#include "common.h"
#include "../../SocketTools/Client_Sender.hpp"
#include "../../SocketTools/Server_Receiver.hpp"
#include "../PRF_AES.h"
#include "../../HashTools/SHA_Family/SHA256.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <stdexcept>

namespace OPRFTools
{
    class DH_Receiver
    {
    private:
        /**
         * 验证给定的数是否为有效的质数
         *
         * @param p 待验证的质数
         * @return 如果p是有效且安全的质数则返回true，否则返回false
         */
        bool isValidPrime(long long p) const
        {
            // 检查质数大小是否满足安全要求
            if (p <= 1000)
            { // 确保质数足够大（实际应用需更大，这里仅为示例）
                std::cerr << "[验证失败] 质数p太小，安全性不足\n";
                return false;
            }
            // 验证p是否为真正的质数
            if (!is_prime(p))
            {
                std::cerr << "[验证失败] p不是有效的质数\n";
                return false;
            }
            return true;
        }

        /**
         * 验证给定的数是否为模p的原根
         * @param g 待验证的原根候选值
         * @param p 模数，必须为素数
         * @return 如果g是模p的原根则返回true，否则返回false
         */
        bool isValidPrimitiveRoot(long long g, long long p) const
        {
            // 检查原根候选值g的范围是否合法
            if (g <= 1 || g >= p)
            {
                std::cerr << "[验证失败] 原根g必须在(1, p)范围内\n";
                return false;
            }
            // 调用底层函数验证g是否为模p的原根
            if (!is_primitive_root(g, p))
            {
                std::cerr << "[验证失败] g不是p的有效原根\n";
                return false;
            }
            return true;
        }

        /**
         * 生成指定范围内的安全随机数
         * @param min 随机数的最小值（包含）
         * @param max 随机数的最大值（包含）
         * @return 返回[min, max]范围内的随机数
         */
        long long generateSecureRandom(long long min, long long max) const
        {
            // 实际应用中应使用密码学安全的随机数生成器
            static std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<long long> dist(min, max);
            return dist(rng);
        }
        /**
         * 打印单个OPRF运算结果并返回格式化后的输出字符串
         * @param input OPRF运算的输入字符串
         * @param output OPRF运算的输出字节序列
         * @param index OPRF运算结果的索引号，用于标识第几个运算结果
         * @return 格式化后的OPRF输出字符串（十六进制表示）
         */
        std::string printSingleOprfResult(const std::string &input, const std::string &output, size_t index) const
        {
            std::cout << "OPRF输入 " << index << ": " << input << std::endl;
            std::cout << "OPRF输出 " << index << ": ";
            std::string str = "";
            // 将输出字节序列转换为十六进制字符串表示
            for (unsigned char byte : output)
            {
                str += byteToHexString(byte);
            }
            return str;
        }

    public:
        /**
         * @brief DH_Receiver构造函数
         * @details 初始化DH_Receiver对象，设置随机数生成器的种子
         * @note 使用高精度时钟作为随机数生成器的种子，确保每次运行时获得不同的随机序列
         */
        DH_Receiver()
        {
            // 初始化随机数生成器（使用高精度时钟作为种子）
            std::srand(std::chrono::steady_clock::now().time_since_epoch().count());
        }

        /**
         * @brief 执行接收方的OPRF协议流程，包括密钥交换、共享密钥计算和OPRF值计算。
         *
         * 该函数模拟了接收方在OPRF协议中的完整操作流程：
         * 1. 接收发送方的公开参数（p, g, 发送方公钥）并验证其有效性；
         * 2. 生成接收方自身的密钥对；
         * 3. 将接收方的公钥发送给发送方；
         * 4. 基于双方密钥计算共享密钥；
         * 5. 使用共享密钥派生PRF密钥，并对输入数据集逐个计算OPRF输出。
         *
         * @param datasets 输入的数据集，每个元素为一个字符串，用于计算对应的OPRF值。
         * @return std::vector<std::string> 返回每个输入数据对应的OPRF输出结果字符串；
         *         若过程中发生错误，则返回空向量。
         */
        std::vector<std::string> run(const std::vector<std::string> &datasets)
        {
            try
            {
                // 验证输入数据集有效性
                if (datasets.empty())
                {
                    throw std::invalid_argument("[输入错误] 数据集不能为空");
                }

                // 步骤1: 接收来自发送方的公开参数 (p, g, 发送方公开密钥)
                std::cout << "\n===== 步骤1/5: 接收公开参数 =====" << std::endl;
                std::cout << "接收方在端口 " << PARAM_PORT << " 等待发送方的公开参数...\n";

                SocketTools::Receiver<long long> param_server(PARAM_PORT);
                std::vector<long long> public_params;

                if (param_server.run(public_params) != 0)
                {
                    throw std::runtime_error("接收公开参数时网络操作失败");
                }
                if (public_params.size() != 3)
                {
                    throw std::runtime_error("公开参数格式错误，期望3个参数，实际接收" + std::to_string(public_params.size()) + "个");
                }

                long long p = public_params[0];
                long long g = public_params[1];
                long long public_key_sender = public_params[2];

                // 验证公开参数有效性
                if (!isValidPrime(p))
                {
                    throw std::runtime_error("公开参数p无效");
                }
                if (!isValidPrimitiveRoot(g, p))
                {
                    throw std::runtime_error("公开参数g无效");
                }
                if (public_key_sender <= 1 || public_key_sender >= p)
                {
                    throw std::runtime_error("发送方公开密钥不在有效范围(1, p)内");
                }

                std::cout << "成功接收并验证公开参数:" << std::endl;
                std::cout << "p (质数) = " << p << std::endl;
                std::cout << "g (原根) = " << g << std::endl;
                std::cout << "发送方公开密钥 = " << public_key_sender << std::endl;

                // 步骤2: 生成接收方私有密钥和公开密钥
                std::cout << "\n===== 步骤2/5: 生成密钥对 =====" << std::endl;
                long long private_key_receiver = generateSecureRandom(2, p - 2); // 安全随机生成私有密钥
                long long public_key_receiver = generate_public_key(private_key_receiver, g, p);

                std::cout << "接收方私有密钥 (仅本地存储) = " << private_key_receiver << std::endl;
                std::cout << "接收方公开密钥 (待发送) = " << public_key_receiver << std::endl;

                // 步骤3: 发送接收方公开密钥给发送方
                std::cout << "\n===== 步骤3/5: 发送公开密钥 =====" << std::endl;
                std::cout << "向发送方端口 " << PUBLIC_KEY_PORT << " 发送接收方公开密钥...\n";

                SocketTools::Sender<long long> key_client("127.0.0.1", PUBLIC_KEY_PORT);
                std::vector<long long> receiver_public_key = {public_key_receiver};

                if (key_client.send_array(receiver_public_key) != 0)
                {
                    throw std::runtime_error("发送接收方公开密钥时网络操作失败");
                }
                std::cout << "接收方公开密钥发送成功" << std::endl;

                // 步骤4: 计算共享密钥
                std::cout << "\n===== 步骤4/5: 计算共享密钥 =====" << std::endl;
                long long shared_secret = compute_shared_secret(public_key_sender, private_key_receiver, p);

                if (shared_secret <= 1)
                {
                    throw std::runtime_error("计算得到的共享密钥无效（值过小）");
                }
                std::cout << "接收方计算的共享密钥 = " << shared_secret << std::endl;

                // 步骤5: 基于共享密钥计算OPRF值
                std::cout << "\n===== 步骤5/5: 计算OPRF结果 =====" << std::endl;
                // 将共享密钥通过SHA256转换为32字节AES-256密钥
                SHA256 sha256;
                sha256.input(std::to_string(shared_secret));
                std::string prf_key = sha256.output(32); // 获取32字节密钥

                if (prf_key.size() != 32)
                {
                    throw std::runtime_error("PRF密钥生成失败，长度不是32字节");
                }

                PRF_AES prf(prf_key);
                std::vector<std::string> oprf_outputs;

                // 遍历数据集计算OPRF
                for (size_t i = 0; i < datasets.size(); ++i)
                {
                    const std::string &input = datasets[i];
                    auto output = prf.evaluate(input);

                    if (output.empty())
                    {
                        throw std::runtime_error("第" + std::to_string(i + 1) + "个数据的OPRF计算失败");
                    }

                    std::string res = printSingleOprfResult(input, output, i + 1);
                    std::cout << res << std::endl;
                    oprf_outputs.push_back(res);
                }

                std::cout << "===== 密钥交换与OPRF计算完成 =====" << std::endl;
                return oprf_outputs;
            }
            catch (const std::exception &e)
            {
                std::cerr << "[接收方错误] " << e.what() << std::endl;
                return {};
            }
        }
    };
}
#endif
