# 定义一个名为"demo"的静态链接库目标
# add_library：CMake命令，用于创建库目标
# demo：指定生成的库的名称，后续可通过该名称引用此库
# STATIC：指定库类型为静态库（.a/.lib），编译时会将代码打包到引用它的目标中
# HashDemo.cpp：构建该库所需的源文件（此处为哈希演示功能的实现文件）
add_library(demo
    STATIC
    HashDemo.cpp
    PRFDemo.cpp
    PRPDemo.cpp
)
# 查找OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
message(STATUS "OpenSSL 头文件路径: ${OPENSSL_INCLUDE_DIR}")
add_definitions(-DOPENSSL_NO_SSL3)
# 为"demo"库设置头文件搜索路径
# target_include_directories：为指定目标配置头文件目录
# demo：目标库名称，即上面定义的"demo"
# PUBLIC：访问权限修饰符，含义是：
#   - 编译"demo"库自身时需要搜索此目录以找到依赖的头文件（如HashDemo.h）
#   - 所有依赖"demo"库的其他目标（如可执行文件）会自动继承此搜索路径
# ${CMAKE_SOURCE_DIR}/lib：头文件所在的目录，CMAKE_SOURCE_DIR是CMake预定义变量，指向项目根目录
# 作用：确保编译"demo"库及其依赖项时，能正确找到所需的头文件（避免"头文件未找到"错误）
#target_include_directories(demo PUBLIC ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(demo PUBLIC PRFTools OpenSSL::Crypto OpenSSL::SSL)