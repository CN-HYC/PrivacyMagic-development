#include <iostream>
#include "logo/PrintLogo.h"
#include "test_demo/HashDemo.h"
#include "test_demo/PRFDemo.h"
#include "SocketTools/Server_Receiver.hpp"
#include "SocketTools/Client_Sender.hpp"
#include "OPRFTools/DH/DH_Sender.hpp"
#include "OPRFTools/DH/DH_Receiver.hpp"
#include <string>
#include <chrono>
#include <ctime>
#include <vector>
//#include "CryptoTools/PRNG.hpp"

using namespace std;
/**
 * @brief 主函数，程序入口点。根据命令行参数执行不同的功能模块。
 *
 * @param argc 命令行参数的数量
 * @param argv 指向命令行参数字符串数组的指针
 * @return int 程序退出状态码：
 *             - 0 表示正常退出
 *             - 非0 表示异常退出
 */
int main(int argc, char const *argv[])
{

    if (argc > 1 && string(argv[1]) == "--logo")
    {
        printLargeMPC(); // Print the logo if --logo argument is provided
    }
    else if (argc > 1 && string(argv[1]) == "--hash")
    {
        hashdemo(); // Run the hash demo if --hashdemo argument is provided
    }
    else if (argc > 1 && string(argv[1]) == "--socket")
    {
        // 如果输入0，运行server端
        if (argc > 2 && string(argv[2]) == "0")
        {
            cout << "Running Server..." << endl;
            SocketTools::Receiver<double> server(8080);
            std::vector<double> recv_data;
            // 阻塞等待客户端连接、接收数据
            server.run(recv_data);
            return 0;
        }
        // 如果输入1，则运行client端
        else if (argc > 2 && string(argv[2]) == "1")
        {
            // 1. 创建客户端实例（连接本地服务器127.0.0.1，端口8080，发送double类型）
            SocketTools::Sender<double> client("127.0.0.1", 8080);

            // 2. 准备待发送的double数组
            std::vector<double> send_data = {3.14, 2.718, 3, 0.0, 100.99, 5.555, 7, 10};

            // 3. 发送数组（调用客户端接口）
            const int result = client.send_array(send_data);

            // 4. 处理发送结果
            if (result == 0)
            {
                std::cout << "[主程序] 客户端发送成功\n";
            }
            else
            {
                std::cerr << "[主程序] 客户端发送失败，错误码：" << result << "\n";
                return 1; // 非0表示异常退出
            }

            return 0; // 0表示正常退出
        }
    }
    else if (argc > 1 && string(argv[1]) == "--prf")
    {
        // Run the PRF demo
        return PRFdemo();
    }
    else if (argc > 1 && string(argv[1]) == "--oprf")
    {

        // 运行OPRF工具的发送方或接收方
        if (argc > 2 && string(argv[2]) == "0")
        {
            std::cout << "Running OPRF Receiver..." << std::endl;
            // 运行接收方
            OPRFTools::DH_Receiver dh_receiver;
            std::vector<std::string> datasets = {"1", "2", "3"};                   // 可以传入数据集参数
            std::vector<std::string> receiver_outputs = dh_receiver.run(datasets); // 运行发送方
            // for (const auto &output : receiver_outputs)
            // {
            //     std::cout << output << std::endl;
            // }
            return 0;
        }
        else if (argc > 2 && string(argv[2]) == "1")
        {
            std::cout << "Running OPRF Sender..." << std::endl;
            // 运行发送方
            OPRFTools::DH_Sender dh_sender;
            std::vector<std::string> datasets = {"1", "2", "4"}; // 可以传入数据集参数
            std::vector<std::string> sender_outputs = dh_sender.run(datasets);
            // for (const auto &output : sender_outputs)
            // {
            //     std::cout << output << std::endl;
            // }
            return 0;
        }
        else
        {
            cout << "Usage: " << argv[0] << " --oprf [0|1]" << endl;
            cout << "  0: Run Receiver" << endl;
            cout << "  1: Run Sender" << endl;
            return 1; // Exit with error code if no valid argument is provided
        }
    }
    else
    {
        cout << "Usage: " << argv[0] << " [--parm]" << endl;
        cout << "parm: " << endl;
        cout << "  --logo         Print the logo" << endl;
        cout << "  --hash         Run the hash demo" << endl;
        cout << "  --prf          Run the PRF demo" << endl
             << endl;
        cout << "   Two terminals need to be opened: " << endl;
        cout << "  --socket [0|1] Run the socket demo (0 for Server, 1 for Client)" << endl;
        cout << "  --oprf   [0|1] Run OPRF tools (0 for Receiver, 1 for Sender)" << endl;
        return 1; // Exit with error code if no valid argument is provided
    }

    return 0;
}
