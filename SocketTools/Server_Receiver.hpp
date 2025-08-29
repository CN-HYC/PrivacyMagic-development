#ifndef SERVER_RECEIVER_H
#define SERVER_RECEIVER_H

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <cstdint>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

// 服务器端套接字命名空间，隔离与客户端的同名类/函数
namespace SocketTools
{

    /**
     * @brief 自定义64位网络字节序转主机字节序函数
     *        解决系统可能没有ntohll函数的兼容性问题
     * @param net64 网络字节序的64位整数
     * @return 转换后的主机字节序64位整数
     */
    inline uint64_t custom_ntohll(uint64_t net64)
    {
        // 检测系统字节序：小端存储（LSB）还是大端存储（MSB）
        const uint16_t endian_test = 0x0001;
        const bool is_little_endian = (*reinterpret_cast<const uint8_t *>(&endian_test) == 0x01);

        if (is_little_endian)
        {
            // 小端系统：手动翻转8字节顺序（网络字节序为大端）
            uint64_t host64 = 0;
            const uint8_t *net_bytes = reinterpret_cast<const uint8_t *>(&net64);
            uint8_t *host_bytes = reinterpret_cast<uint8_t *>(&host64);
            for (int i = 0; i < 8; ++i)
            {
                host_bytes[7 - i] = net_bytes[i]; // 第i字节与第7-i字节交换
            }
            return host64;
        }
        else
        {
            // 大端系统：网络字节序与主机字节序一致，无需转换
            return net64;
        }
    }

    /**
     * @brief 通用数组接收服务器模板类
     *        支持接收任意可序列化类型T的数组，自动处理网络通信与数据统计
     * @tparam T 接收的数据类型（需支持二进制序列化）
     */
    template <typename T>
    class Receiver
    {
    private:
        uint16_t port_;           // 服务器监听端口
        int server_fd_;           // 服务器套接字文件描述符
        int client_fd_;           // 客户端连接套接字文件描述符
        bool is_running_;         // 服务器运行状态标记
        size_t total_recv_bytes_; // 累计接收的总字节数（单次连接）

        /**
         * @brief 接收数据并累加统计字节数
         *        封装recv系统调用，统一处理接收逻辑和字节统计
         * @param buf 接收缓冲区指针
         * @param buf_len 期望接收的字节数
         * @return 成功接收返回true，失败返回false
         */
        bool recv_and_count(void *buf, size_t buf_len)
        {
            // 调用系统recv函数接收数据
            ssize_t recv_len = ::recv(client_fd_, buf, buf_len, 0);

            // 处理接收错误
            if (recv_len <= 0)
            {
                std::cerr << "[错误] 接收数据失败（累计统计异常）：";
                if (recv_len < 0)
                    perror("系统错误"); // 打印系统错误详情（如权限、中断等）
                else
                    std::cerr << "客户端主动断开连接\n";
                return false;
            }

            // 累加接收的字节数（转换为无符号类型避免负数问题）
            total_recv_bytes_ += static_cast<size_t>(recv_len);
            return true;
        }

        /**
         * @brief 接收数组数据的核心逻辑
         *        先接收数组长度，再根据长度接收完整数组数据
         * @param recv_data 用于存储接收数据的vector容器
         * @return 接收成功返回true，失败返回false
         */
        bool recv_array(std::vector<T> &recv_data)
        {
            uint64_t net_len = 0; // 网络字节序的数组长度

            // 步骤1：接收数组长度（固定8字节，uint64_t类型）
            if (!recv_and_count(&net_len, sizeof(net_len)))
            {
                std::cerr << "[错误] 接收数组长度失败\n";
                return false;
            }

            // 将网络字节序的长度转换为主机字节序
            const size_t data_len = static_cast<size_t>(custom_ntohll(net_len));
            if (data_len == 0)
            {
                std::cerr << "[错误] 接收的数组长度为0，无效数据\n";
                return false;
            }

            // 计算数组总字节数（元素个数 × 单个元素字节数）
            const size_t total_bytes = data_len * sizeof(T);
            std::vector<char> byte_buffer(total_bytes); // 临时缓冲区存储原始字节
            size_t remaining = total_bytes;             // 剩余未接收的字节数
            char *buf_ptr = byte_buffer.data();

            // 步骤2：循环接收数组数据（处理TCP粘包/分包问题）
            while (remaining > 0)
            {
                // 计算本次接收的缓冲区位置和长度
                const char *recv_buf = buf_ptr + (total_bytes - remaining);
                const size_t recv_len = remaining;

                // 接收数据（使用const_cast移除const，不修改原始数据）
                ssize_t recv_bytes = ::recv(
                    client_fd_,
                    const_cast<char *>(recv_buf),
                    recv_len,
                    0);

                // 处理接收错误
                if (recv_bytes <= 0)
                {
                    std::cerr << "[错误] 接收数组数据失败：";
                    if (recv_bytes < 0)
                        perror("系统错误");
                    else
                        std::cerr << "客户端断开连接\n";
                    return false;
                }

                // 累加接收字节数并更新剩余字节数
                total_recv_bytes_ += static_cast<size_t>(recv_bytes);
                remaining -= static_cast<size_t>(recv_bytes);
            }

            // 将接收的字节数据转换为目标类型数组
            recv_data.resize(data_len);
            std::memcpy(recv_data.data(), byte_buffer.data(), total_bytes);

            std::cout << "[成功] 接收数组：" << data_len << "元素，业务数据字节数：" << total_bytes << "\n";
            return true;
        }

        /**
         * @brief 初始化服务器套接字
         *        包括创建套接字、设置端口复用、绑定端口、开始监听
         * @return 初始化成功返回true，失败返回false
         */
        bool init_server()
        {
            // 创建TCP套接字（IPv4协议，字节流套接字）
            server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd_ == -1)
            {
                std::cerr << "[错误] 创建套接字失败：";
                perror("系统错误");
                return false;
            }

            // 设置端口复用（避免服务器重启时端口被占用）
            const int opt = 1;
            if (::setsockopt(
                    server_fd_,
                    SOL_SOCKET,   // 套接字级别选项
                    SO_REUSEADDR, // 允许地址复用
                    &opt,
                    static_cast<socklen_t>(sizeof(opt))) < 0)
            {
                std::cerr << "[错误] 设置端口复用失败：";
                perror("系统错误");
                ::close(server_fd_); // 清理资源
                server_fd_ = -1;
                return false;
            }

            // 配置服务器地址结构
            struct sockaddr_in server_addr;
            std::memset(&server_addr, 0, sizeof(server_addr)); // 初始化清零
            server_addr.sin_family = AF_INET;                  // IPv4协议
            server_addr.sin_addr.s_addr = INADDR_ANY;          // 监听所有网卡地址
            server_addr.sin_port = htons(port_);               // 端口转换为网络字节序

            // 绑定套接字到指定端口
            if (::bind(
                    server_fd_,
                    reinterpret_cast<const struct sockaddr *>(&server_addr),
                    static_cast<socklen_t>(sizeof(server_addr))) == -1)
            {
                std::cerr << "[错误] 绑定端口失败（端口：" << port_ << "）：";
                perror("系统错误");
                ::close(server_fd_);
                server_fd_ = -1;
                return false;
            }

            // 开始监听端口（最大等待队列长度为5）
            if (::listen(server_fd_, 5) == -1)
            {
                std::cerr << "[错误] 监听端口失败（端口：" << port_ << "）：";
                perror("系统错误");
                ::close(server_fd_);
                server_fd_ = -1;
                return false;
            }

            std::cout << "[成功] 服务器启动，监听端口：" << port_ << "\n";
            is_running_ = true;
            return true;
        }

        /**
         * @brief 接受客户端连接
         *        阻塞等待客户端连接，成功后初始化客户端套接字
         * @return 接受连接成功返回true，失败返回false
         */
        bool accept_client()
        {
            struct sockaddr_in client_addr; // 客户端地址信息
            socklen_t client_addr_len = sizeof(client_addr);

            // 接受客户端连接（阻塞调用，直到有连接到来）
            client_fd_ = ::accept(
                server_fd_,
                reinterpret_cast<struct sockaddr *>(&client_addr),
                &client_addr_len);

            if (client_fd_ == -1)
            {
                std::cerr << "[错误] 接受客户端连接失败：";
                perror("系统错误");
                return false;
            }

            // 连接成功，输出客户端信息并重置接收统计
            std::cout << "[成功] 客户端连接：IP=" << inet_ntoa(client_addr.sin_addr)
                      << ", 套接字描述符=" << client_fd_ << "\n";
            total_recv_bytes_ = 0; // 重置统计值，确保单次连接单独统计
            return true;
        }

        /**
         * @brief 检查结束标记
         *        验证是否收到客户端发送的"ARRAY_FINISHED"结束标记
         * @return 标记匹配返回true，不匹配或接收失败返回false
         */
        bool check_end_flag()
        {
            char end_flag[32] = {0};                                 // 存储接收的结束标记
            const size_t expect_flag_len = strlen("ARRAY_FINISHED"); // 期望的标记长度（16字节）

            // 接收结束标记并统计字节数
            if (!recv_and_count(end_flag, expect_flag_len))
            {
                std::cerr << "[错误] 接收结束标记失败\n";
                return false;
            }

            // 验证标记是否匹配
            if (std::string(end_flag) == "ARRAY_FINISHED")
            {
                std::cout << "[成功] 收到结束标记（标记字节数：" << expect_flag_len << "）\n";
                return true;
            }
            else
            {
                std::cerr << "[错误] 结束标记不匹配，收到：" << end_flag << "\n";
                return false;
            }
        }

        /**
         * @brief 关闭所有套接字
         *        安全关闭服务器和客户端套接字，避免资源泄漏
         */
        void close_sockets()
        {
            if (client_fd_ != -1)
            {
                ::close(client_fd_);
                client_fd_ = -1;
            }
            if (server_fd_ != -1)
            {
                ::close(server_fd_);
                server_fd_ = -1;
            }
            is_running_ = false;
            std::cout << "[信息] 所有套接字已关闭\n";
        }

    public:
        /**
         * @brief 构造函数
         * @param port 服务器监听端口（默认8080）
         */
        explicit Receiver(uint16_t port = 8080)
            : port_(port), server_fd_(-1), client_fd_(-1),
              is_running_(false), total_recv_bytes_(0) {}

        /**
         * @brief 析构函数
         *        确保服务器退出时关闭所有打开的套接字
         */
        ~Receiver()
        {
            if (is_running_)
                close_sockets();
        }

        /**
         * @brief 禁止拷贝构造和赋值操作
         *        避免套接字文件描述符被重复关闭导致错误
         */
        Receiver(const Receiver &) = delete;
        Receiver &operator=(const Receiver &) = delete;

        /**
         * @brief 获取本次连接接收的总字节数
         * @return 总接收字节数（size_t类型）
         */
        size_t get_total_recv_bytes() const
        {
            return total_recv_bytes_;
        }

        /**
         * @brief 启动服务器主流程
         *        初始化服务器→接受连接→接收数组→验证结束标记
         * @param output_data 用于存储接收结果的vector
         * @return 0=成功，1=初始化失败，2=接受连接失败，3=接收数组失败，4=结束标记验证失败
         */
        int run(std::vector<T> &output_data)
        {
            // 步骤1：初始化服务器
            if (!init_server())
                return 1;

            // 步骤2：接受客户端连接
            if (!accept_client())
            {
                close_sockets();
                return 2;
            }

            // 步骤3：接收数组数据
            if (!recv_array(output_data))
            {
                close_sockets();
                return 3;
            }

            // 步骤4：验证结束标记
            if (!check_end_flag())
            {
                close_sockets();
                return 4;
            }

            // 输出接收统计结果（与客户端发送的字节数对比验证）
            std::cout << "=========================================\n";
            std::cout << "[服务器统计] 本次接收总字节数：" << total_recv_bytes_ << " 字节\n";
            std::cout << "[服务器统计] 构成：数组长度(8字节) + 业务数据 + 结束标记(16字节)\n";
            std::cout << "=========================================\n";

            // 调试输出：打印接收的数组内容
            std::cout << "[调试] 接收数组：";
            for (const auto &val : output_data)
                std::cout << val << " ";
            std::cout << "\n";

            // 关闭连接，释放资源
            close_sockets();
            return 0;
        }

        /**
         * @brief 获取         * @return 服务器运行状态（true=运行中，false=已停止）
         */
        bool is_running() const { return is_running_; }
    };

} // namespace SocketServer

#endif // SERVER_RECEIVER_H
