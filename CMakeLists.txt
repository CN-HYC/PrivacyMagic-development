#最小CMake版本
cmake_minimum_required(VERSION 3.10)
#项目名称
project(CryptoMagic)
#设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
#设置输出文件夹
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/frontend/release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 添加 cmake 模块路径
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
# 设置本项目输出变量（包含找到的路径）
set(LIBDIVIDE_INCLUDE_DIRS ${LIBDIVIDE_INCLUDE_DIR}) 
message(STATUS "libdivide 头文件路径: ${LIBDIVIDE_INCLUDE_DIRS}")

# 添加测试子目录
add_subdirectory(test_demo)

add_subdirectory(OPRFTools)

# 查找OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
message(STATUS "OpenSSL 头文件路径: ${OPENSSL_INCLUDE_DIR}")
add_definitions(-DOPENSSL_NO_SSL3)

#可执行文件名称
add_executable(${PROJECT_NAME} main.cpp logo/PrintLogo.cpp)


# 连接hash库
target_link_libraries(${PROJECT_NAME} PRIVATE demo PRFTools OpenSSL::SSL OpenSSL::Crypto)

