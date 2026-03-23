cmake_minimum_required(VERSION 3.16)
project(HeteroLinkHostTests VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Qt 和测试框架（优先 Qt6，回退到 Qt5）
find_package(Qt6 COMPONENTS Core Test QUIET)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Test)
    set(QT_VERSION_MAJOR 5)
endif()

# 测试源文件
set(TEST_SOURCES
    test_protocol.cpp
    test_dataprocessor.cpp
)

# 创建测试可执行文件
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    
    if(QT_VERSION_MAJOR EQUAL 6)
        target_link_libraries(${test_name} PRIVATE
            Qt6::Core
            Qt6::Test
        )
    else()
        target_link_libraries(${test_name} PRIVATE
            Qt5::Core
            Qt5::Test
        )
    endif()
    
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/../src
    )
    
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
