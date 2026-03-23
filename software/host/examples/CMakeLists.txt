cmake_minimum_required(VERSION 3.16)
project(mqtt_test LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# 查找 Qt6 核心组件
find_package(Qt6 REQUIRED COMPONENTS Core Network)
# MQTT 已经在父 CMakeLists.txt 中配置，直接使用 target
if(NOT TARGET Qt6::Mqtt)
    message(FATAL_ERROR "Qt6::Mqtt target not available - MQTT support is required")
endif()

add_executable(mqtt_test
    mqtt_test.cpp
)

target_link_libraries(mqtt_test PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Mqtt
)

target_include_directories(mqtt_test PRIVATE
    ${CMAKE_SOURCE_DIR}/../src
)

# 暂时禁用 mqtt_test 构建，因为需要链接主程序
# 用户可以单独构建和运行主程序
set_target_properties(mqtt_test PROPERTIES EXCLUDE_FROM_ALL TRUE)
