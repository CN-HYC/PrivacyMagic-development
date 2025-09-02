# 添加库
add_library(PRFTools
    STATIC
    PRF_AES.cpp
)

# 添加头文件检索路径
#target_include_directories(PRFTools PUBLIC ${CMAKE_SOURCE_DIR}/lib)