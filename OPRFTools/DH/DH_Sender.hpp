#ifndef DH_SENDER_H
#define DH_SENDER_H

#include "common.h"
#include "../../SocketTools/Client_Sender.hpp"
#include "../../SocketTools/Server_Receiver.hpp"
#include "../PRF_AES.h"
#include "../../HashTools/SHA256.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <stdexcept>
#include <chrono>
#include <random>
#include <iomanip>

namespace OPRF
{
    class DH_Sender
    {
    private:
        bool isValidPrime(long long p) const
        {
            if (p < MIN_PRIME || p > MAX_PRIME)
            {
                std::cerr << "[质数验证失败] 质数p=" << p << "，需在[" << MIN_PRIME << "," << MAX_PRIME << "]范围内\n";
                return false;
            }
            if (!is_prime(p))
            {
                std::cerr << "[质数验证失败] p=" << p << "不是有效质数\n";
                return false;
            }
            return true;
        }

        bool isValidPrimitiveRoot(long long g, long long p) const
        {
            if (g <= 1 || g >= p)
            {
                std::cerr << "[原根验证失败] 原根g=" << g << "，需在(1, " << p << ")范围内\n";
                return false;
            }
            if (!is_primitive_root(g, p))
            {
                std::cerr << "[原根验证失败] g=" << g << "不是p=" << p << "的有效原根\n";
                return false;
            }
            return true;
        }

        long long generateSecureRandom(long long min, long long max) const
        {
            static std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
            std::uniform_int_distribution<long long> dist(min, max);
            return dist(rng);
        }

        std::string printSingleOprfResult(const std::string &input, const std::string &output, size_t index) const
        {
            std::cout << "OPRF输入 " << index << ": " << input << std::endl;
            std::cout << "OPRF输出 " << index << ": ";
            std::string str = "";
            for (unsigned char byte : output)
            {
                str += byteToHexString(byte);
            }
            return str;
        }

    public:
        DH_Sender()
        {
            std::srand(std::chrono::steady_clock::now().time_since_epoch().count());
        }

        std::vector<std::string> run(const std::vector<std::string> &datasets)
        {
            try
            {
                if (datasets.empty())
                {
                    throw std::invalid_argument("[输入错误] OPRF计算数据集不能为空");
                }

                std::cout << "===== 步骤1/5：生成公开参数 =====" << std::endl;
                long long p = generate_prime(MIN_PRIME, MAX_PRIME);
                long long g = generate_primitive_root(p);

                if (!isValidPrime(p))
                {
                    throw std::runtime_error("生成的质数p无效，无法继续");
                }
                if (!isValidPrimitiveRoot(g, p))
                {
                    throw std::runtime_error("生成的原根g无效，无法继续");
                }

                std::cout << "成功生成公开参数：" << std::endl;
                std::cout << "p (质数) = " << p << std::endl;
                std::cout << "g (原根) = " << g << std::endl
                          << std::endl;

                std::cout << "===== 步骤2/5：生成发送方密钥对 =====" << std::endl;
                long long private_key_sender = generateSecureRandom(2, p - 2);
                long long public_key_sender = generate_public_key(private_key_sender, g, p);

                if (public_key_sender <= 1 || public_key_sender >= p)
                {
                    throw std::runtime_error("生成的发送方公开密钥=" + std::to_string(public_key_sender) + "无效");
                }

                std::cout << "发送方私有密钥 (仅本地存储) = " << private_key_sender << std::endl;
                std::cout << "发送方公开密钥 (待发送给接收方) = " << public_key_sender << std::endl
                          << std::endl;

                std::cout << "===== 步骤3/5：发送公开参数与公开密钥 =====" << std::endl;
                std::cout << "向接收方端口 " << PARAM_PORT << " 发送数据...\n";

                SocketClient::Sender<long long> param_client("127.0.0.1", PARAM_PORT);
                std::vector<long long> public_params = {p, g, public_key_sender};

                if (param_client.send_array(public_params) != 0)
                {
                    throw std::runtime_error("网络发送失败：无法将公开参数传递给接收方");
                }
                std::cout << "公开参数（p, g, 发送方公钥）发送成功" << std::endl
                          << std::endl;

                std::cout << "===== 步骤4/5：接收公钥并计算共享密钥 =====" << std::endl;
                std::cout << "在端口 " << PUBLIC_KEY_PORT << " 等待接收方公开密钥...\n";

                SocketServer::Receiver<long long> key_server(PUBLIC_KEY_PORT);
                std::vector<long long> received_data;

                if (key_server.run(received_data) != 0)
                {
                    throw std::runtime_error("网络接收失败：无法获取接收方公开密钥");
                }
                if (received_data.size() != 1)
                {
                    throw std::runtime_error("接收数据格式错误：期望1个公钥，实际接收" + std::to_string(received_data.size()) + "个数据");
                }

                long long public_key_receiver = received_data[0];
                if (public_key_receiver <= 1 || public_key_receiver >= p)
                {
                    throw std::runtime_error("接收方公开密钥=" + std::to_string(public_key_receiver) + "无效（需在1~" + std::to_string(p - 1) + "范围内）");
                }
                std::cout << "成功接收接收方公开密钥 = " << public_key_receiver << std::endl;

                long long shared_secret = compute_shared_secret(public_key_receiver, private_key_sender, p);
                if (shared_secret <= 1)
                {
                    throw std::runtime_error("共享密钥计算错误，结果=" + std::to_string(shared_secret) + "（无效值）");
                }
                std::cout << "发送方计算的共享密钥 = " << shared_secret << std::endl
                          << std::endl;

                std::cout << "===== 步骤5/5：计算OPRF结果 =====" << std::endl;
                SHA256 sha256;
                sha256.input(std::to_string(shared_secret));
                std::string prf_key = sha256.output(32);

                if (prf_key.size() != 32)
                {
                    throw std::runtime_error("PRF密钥生成失败：长度=" + std::to_string(prf_key.size()) + "字节，需32字节");
                }

                PRF_AES prf(prf_key);
                std::vector<std::string> oprf_outputs;

                for (size_t i = 0; i < datasets.size(); ++i)
                {
                    const std::string &input = datasets[i];
                    std::string output = prf.evaluate(input);

                    if (output.empty())
                    {
                        throw std::runtime_error("第" + std::to_string(i + 1) + "条数据OPRF计算失败，输入=" + input);
                    }

                    std::string res = printSingleOprfResult(input, output, i + 1);
                    std::cout << res << std::endl;
                    oprf_outputs.push_back(res);
                }

                std::cout << "===== 发送方流程全部完成 =====" << std::endl;
                return oprf_outputs;
            }
            catch (const std::exception &e)
            {
                std::cerr << "[发送方错误] " << e.what() << std::endl;
                return {};
            }
        }
    };
}
#endif // DH_SENDER_H