cmake_minimum_required(VERSION 3.20)
project(HeteroLinkHostTests VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt6 和测试框架
find_package(Qt6 REQUIRED COMPONENTS Core Test)

# 测试源文件
set(TEST_SOURCES
    test_protocol.cpp
    test_dataprocessor.cpp
)

# 创建测试可执行文件
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    
    target_link_libraries(${test_name} PRIVATE
        Qt6::Core
        Qt6::Test
    )
    
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/../src
    )
    
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
