add_library(PRFTools
    STATIC
    PRF_AES.cpp
)

target_include_directories(PRFTools
    PUBLIC
    ${CMAKE_SOURCE_DIR}/lib)