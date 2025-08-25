# 在 Findlibdivide.cmake 中修改头文件查找路径
find_path(LIBDIVIDE_INCLUDE_DIR
    NAMES libdivide.h  # libdivide 的标志性头文件
    PATHS
        "/Users/hyc/Desktop/gitresp/CryptoMagic/build/install/include"  # 你的自定义路径（优先搜索）
        "/usr/local/include"            # 原路径（作为备选）
        "/usr/include"
    DOC "Directory containing libdivide header files"
)

# 设置输出变量（包含找到的路径）
set(LIBDIVIDE_INCLUDE_DIRS ${LIBDIVIDE_INCLUDE_DIR})