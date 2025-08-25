#ifndef CLIENT_SENDER_H
#define CLIENT_SENDER_H

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdint>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

// 命名空间隔离：避免与服务器端或其他库的同名类/函数产生冲突
namespace SocketClient
{

    /**
     * @brief 自定义64位主机字节序到网络字节序的转换函数
     *        用于解决部分系统缺少htonll函数的兼容性问题，与服务器端custom_ntohll完全对应
     * @param host64 主机字节序的64位整数
     * @return 转换后的网络字节序64位整数
     */
    inline uint64_t custom_htonll(uint64_t host64)
    {
        // 检测系统字节序：通过16位整数的存储方式判断（小端/大端）
        const uint16_t endian_test = 0x0001;
        const bool is_little_endian = (*reinterpret_cast<const uint8_t *>(&endian_test) == 0x01);

        if (is_little_endian)
        {
            // 小端系统：需要手动翻转8字节顺序（网络字节序为大端）
            uint64_t net64 = 0;
            const uint8_t *host_bytes = reinterpret_cast<const uint8_t *>(&host64);
            uint8_t *net_bytes = reinterpret_cast<uint8_t *>(&net64);
            for (int i = 0; i < 8; ++i)
            {
                net_bytes[7 - i] = host_bytes[i]; // 反转字节顺序
            }
            return net64;
        }
        else
        {
            // 大端系统：网络字节序与主机字节序一致，无需转换
            return host64;
        }
    }

    /**
     * @brief 通用数组发送客户端模板类
     *        支持向服务器发送任意可序列化类型T的数组，自动处理网络通信和数据统计
     * @tparam T 发送的数据类型（需支持二进制序列化）
     */
    template <typename T>
    class Sender
    {
    private:
        std::string server_ip_;   // 服务器IP地址（字符串形式）
        uint16_t server_port_;    // 服务器端口号
        int client_fd_;           // 客户端套接字文件描述符
        bool is_connected_;       // 连接状态标记（true表示已连接）
        size_t total_sent_bytes_; // 累计发送的总字节数（单次连接统计）

        /**
         * @brief 发送数据并累加统计字节数
         *        封装send系统调用，统一处理发送逻辑和字节统计
         * @param data 待发送数据的指针
         * @param data_len 待发送数据的字节数
         * @return 发送成功返回true，失败返回false
         */
        bool send_and_count(const void *data, size_t data_len)
        {
            // 调用系统send函数发送数据
            ssize_t sent = ::send(client_fd_, data, data_len, 0);

            // 处理发送错误
            if (sent == -1)
            {
                std::cerr << "[客户端错误] 发送数据失败（累计统计异常）：";
                perror("系统错误详情"); // 打印系统级错误信息
                return false;
            }

            // 累加发送的字节数（转换为无符号类型避免负数问题）
            total_sent_bytes_ += static_cast<size_t>(sent);
            return true;
        }

        /**
         * @brief 发送数组数据的核心逻辑
         *        先发送数组长度（网络字节序），再分块发送数组内容
         * @param array 待发送数组的指针
         * @param array_len 数组元素个数
         * @return 发送成功返回true，失败返回false
         */
        bool send_array_data(const T *array, size_t array_len)
        {
            // 步骤1：发送数组长度（固定8字节，uint64_t类型，需转换为网络字节序）
            const uint64_t net_len = custom_htonll(static_cast<uint64_t>(array_len));
            if (!send_and_count(&net_len, sizeof(net_len)))
            {
                std::cerr << "[客户端错误] 发送数组长度失败\n";
                return false;
            }

            // 计算数组总字节数（元素个数 × 单个元素字节数）
            const size_t total_bytes = array_len * sizeof(T);
            const char *data_ptr = reinterpret_cast<const char *>(array); // 转换为字节指针
            size_t remaining = total_bytes;                               // 剩余未发送的字节数

            // 步骤2：循环发送数组数据（处理TCP分包限制）
            while (remaining > 0)
            {
                // 计算本次发送的缓冲区位置和长度
                const char *send_buf = data_ptr + (total_bytes - remaining);
                const size_t send_len = remaining;

                // 发送数据
                ssize_t sent_bytes = ::send(client_fd_, send_buf, send_len, 0);
                if (sent_bytes == -1)
                {
                    std::cerr << "[客户端错误] 发送数组数据失败：";
                    perror("系统错误详情");
                    return false;
                }

                // 累加发送字节数并更新剩余字节数
                total_sent_bytes_ += static_cast<size_t>(sent_bytes);
                remaining -= static_cast<size_t>(sent_bytes);
            }

            std::cout << "[客户端成功] 数组发送完成："
                      << array_len << "个元素，业务数据字节数：" << total_bytes << "\n";
            return true;
        }

        /**
         * @brief 与服务器建立TCP连接
         *        包括创建套接字、配置服务器地址、发起连接请求
         * @return 连接成功返回true，失败返回false
         */
        bool connect_to_server()
        {
            // 1. 创建TCP套接字（IPv4协议，字节流套接字）
            client_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
            if (client_fd_ == -1)
            {
                std::cerr << "[客户端错误] 创建套接字失败：";
                perror("系统错误详情");
                return false;
            }

            // 2. 配置服务器地址结构
            struct sockaddr_in server_addr;
            std::memset(&server_addr, 0, sizeof(server_addr)); // 初始化地址结构为0
            server_addr.sin_family = AF_INET;                  // 使用IPv4协议
            server_addr.sin_port = htons(server_port_);        // 端口转换为网络字节序
            server_addr.sin_addr.s_addr = INADDR_ANY;          // 初始化IP地址（后续会覆盖）

            // 3. 将字符串IP转换为网络字节序
            if (::inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0)
            {
                std::cerr << "[客户端错误] 无效服务器IP地址：" << server_ip_ << "\n";
                ::close(client_fd_); // 清理已创建的套接字
                client_fd_ = -1;
                return false;
            }

            // 4. 发起TCP连接请求
            if (::connect(
                    client_fd_,
                    reinterpret_cast<const struct sockaddr *>(&server_addr),
                    static_cast<socklen_t>(sizeof(server_addr))) == -1)
            {
                std::cerr << "[客户端错误] 连接服务器失败（" << server_ip_ << ":" << server_port_ << "）：";
                perror("系统错误详情");
                ::close(client_fd_); // 清理资源
                client_fd_ = -1;
                return false;
            }

            // 连接成功，更新状态并重置统计
            is_connected_ = true;
            total_sent_bytes_ = 0; // 重置发送统计，确保单次连接单独统计
            std::cout << "[客户端成功] 已连接服务器：" << server_ip_ << ":" << server_port_ << "\n";
            return true;
        }

        /**
         * @brief 发送结束标记
         *        向服务器发送"ARRAY_FINISHED"字符串，标识数组发送完成
         * @return 发送成功返回true，失败返回false
         */
        bool send_finish_flag()
        {
            const char *end_flag = "ARRAY_FINISHED";  // 结束标记字符串
            const size_t flag_len = strlen(end_flag); // 计算标记长度（固定16字节）

            // 发送标记并统计字节数
            if (!send_and_count(end_flag, flag_len))
            {
                std::cerr << "[客户端错误] 发送结束标记失败\n";
                return false;
            }

            std::cout << "[客户端成功] 结束标记发送完成（标记字节数：" << flag_len << "）\n";
            return true;
        }

        /**
         * @brief 安全关闭套接字
         *        确保关闭客户端套接字，避免资源泄漏，重置连接状态
         */
        void close_socket()
        {
            if (client_fd_ != -1)
            {
                ::close(client_fd_); // 关闭套接字
                client_fd_ = -1;
            }
            is_connected_ = false; // 更新连接状态
            std::cout << "[客户端信息] 套接字已关闭\n";
        }

        /**
         * @brief 打印待发送数组内容（调试用）
         *        输出数组元素，方便验证发送数据的正确性
         * @param array 待发送数组的指针
         * @param array_len 数组元素个数
         */
        void print_send_data(const T *array, size_t array_len) const
        {
            std::cout << "[客户端调试] 待发送数组：";
            for (size_t i = 0; i < array_len; ++i)
            {
                std::cout << array[i] << " ";
            }
            std::cout << "\n";
        }

    public:
        /**
         * @brief 构造函数
         * @param server_ip 服务器IP地址（字符串形式，如"127.0.0.1"）
         * @param server_port 服务器端口号（主机字节序）
         */
        Sender(const std::string &server_ip, uint16_t server_port)
            : server_ip_(server_ip), server_port_(server_port), client_fd_(-1),
              is_connected_(false), total_sent_bytes_(0) {}

        /**
         * @brief 析构函数
         *        确保对象销毁时关闭所有打开的套接字，防止资源泄漏
         */
        ~Sender()
        {
            if (is_connected_)
            {
                close_socket();
            }
        }

        /**
         * @brief 禁止拷贝构造和赋值操作
         *        避免套接字文件描述符被重复关闭导致的错误
         */
        Sender(const Sender &) = delete;
        Sender &operator=(const Sender &) = delete;

        /**
         * @brief 获取本次连接发送的总字节数
         * @return 总发送字节数（size_t类型）
         */
        size_t get_total_sent_bytes() const
        {
            return total_sent_bytes_;
        }

        /**
         * @brief 发送数组（C风格数组接口）
         *        完整流程：连接服务器→发送数组长度→发送数组数据→发送结束标记
         * @param array 待发送数组的指针（非空）
         * @param array_len 数组元素个数（>0）
         * @return 0=成功，1=连接失败，2=发送数据失败，3=发送结束标记失败
         */
        int send_array(const T *array, size_t array_len)
        {
            total_sent_bytes_ = 0; // 每次发送前重置统计

            // 1. 检查输入有效性
            if (array == nullptr || array_len == 0)
            {
                std::cerr << "[客户端错误] 待发送数组为空或长度为0\n";
                return 2;
            }

            // 2. 连接服务器
            if (!connect_to_server())
            {
                return 1;
            }

            // 3. 打印待发送数据（调试）
            print_send_data(array, array_len);

            // 4. 发送数组数据
            if (!send_array_data(array, array_len))
            {
                close_socket();
                return 2;
            }

            // 5. 发送结束标记
            if (!send_finish_flag())
            {
                close_socket();
                return 3;
            }

            // 6. 输出发送统计结果（与服务器接收的字节数对比验证）
            std::cout << "=========================================\n";
            std::cout << "[客户端统计] 本次发送总字节数：" << total_sent_bytes_ << " 字节\n";
            std::cout << "[客户端统计] 构成：数组长度(8字节) + 业务数据 + 结束标记(16字节)\n";
            std::cout << "=========================================\n";

            // 7. 关闭连接
            close_socket();
            return 0;
        }

        /**
         * @brief 发送数组（std::vector接口，现代C++风格）
         *        封装C风格接口，简化vector容器的发送操作
         * @param data_vec 待发送的vector容器
         * @return 同send_array（C风格接口）的返回值
         */
        int send_array(const std::vector<T> &data_vec)
        {
            return send_array(data_vec.data(), data_vec.size());
        }

        /**
         * @brief 获取当前连接状态
         * @return 连接状态（true=已连接，false=未连接）
         */
        bool is_connected() const
        {
            return is_connected_;
        }
    };

} // namespace SocketClient

#endif // CLIENT_SENDER_H
