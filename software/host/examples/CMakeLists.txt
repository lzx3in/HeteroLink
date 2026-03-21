# 示例程序构建配置

# MQTT 测试程序
add_executable(mqtt_test
    mqtt_test.cpp
)

target_link_libraries(mqtt_test PRIVATE
    heterolink-host
)

target_include_directories(mqtt_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
