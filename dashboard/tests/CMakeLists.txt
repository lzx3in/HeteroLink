# =============================================================================
# HeteroLink Host - 单元测试配置
# =============================================================================
# 此 CMakeLists 配置所有单元测试，链接到主项目源代码
# =============================================================================

cmake_minimum_required(VERSION 3.21)

# 查找 Qt6 Test 组件
find_package(Qt6 REQUIRED COMPONENTS Core Test)

# =============================================================================
# 测试源文件
# =============================================================================
# 每个测试文件对应一个独立的测试可执行文件
# =============================================================================

set(TEST_SOURCES
    test_protocol.cpp
    test_dataprocessor.cpp
    test_helpers.cpp
    test_gui_scaffold.cpp
    test_uartchannel.cpp
    test_mqttchannel.cpp
)

# =============================================================================
# 主项目源文件（用于链接到测试）
# =============================================================================
# 测试需要访问主项目的实现代码，因此需要包含源文件
# =============================================================================

# src 目录作为包含根目录
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(MAIN_SOURCES
    ${SRC_DIR}/core/DataProcessor.cpp
    ${SRC_DIR}/core/AlarmSystem.cpp
    ${SRC_DIR}/protocol/UartChannel.cpp
    ${SRC_DIR}/protocol/MqttChannel.cpp
    ${SRC_DIR}/protocol/Protocol.cpp
    ${SRC_DIR}/storage/ConfigManager.cpp
    ${SRC_DIR}/storage/DataLogger.cpp
    ${SRC_DIR}/utils/Logger.cpp
)

set(MAIN_HEADERS
    ${SRC_DIR}/core/DataProcessor.h
    ${SRC_DIR}/core/AlarmSystem.h
    ${SRC_DIR}/protocol/UartChannel.h
    ${SRC_DIR}/protocol/MqttChannel.h
    ${SRC_DIR}/protocol/Protocol.h
    ${SRC_DIR}/storage/ConfigManager.h
    ${SRC_DIR}/storage/DataLogger.h
    ${SRC_DIR}/utils/Logger.h
)

# =============================================================================
# 创建测试可执行文件
# =============================================================================
# 每个测试独立编译，链接到主项目对象文件
# =============================================================================

foreach(test_src ${TEST_SOURCES})
    # 从源文件名提取测试名称
    get_filename_component(test_name ${test_src} NAME_WE)
    
    # 创建测试可执行文件
    add_executable(${test_name}
        ${test_src}
        ${MAIN_SOURCES}
        ${MAIN_HEADERS}
    )
    
    # 链接 Qt 库
    target_link_libraries(${test_name} PRIVATE
        Qt6::Core
        Qt6::Test
        Qt6::SerialPort
    )
    
    # GUI 测试需要额外链接 Widgets
    if(test_name STREQUAL "test_gui_scaffold")
        target_link_libraries(${test_name} PRIVATE
            Qt6::Widgets
        )
    endif()
    
    # UART 测试需要额外链接 SerialPort
    if(test_name STREQUAL "test_uartchannel")
        target_link_libraries(${test_name} PRIVATE
            Qt6::SerialPort
        )
    endif()
    
    # MQTT 测试在启用 MQTT 时需要链接 Mqtt
    if(HAS_QT_MQTT)
        if(test_name STREQUAL "test_mqttchannel")
            target_link_libraries(${test_name} PRIVATE
                Qt6::Mqtt
            )
            target_compile_definitions(${test_name} PRIVATE HAS_QT_MQTT=1)
        endif()
    endif()
    
    # 如果支持 MQTT，也链接
    if(HAS_QT_MQTT)
        target_link_libraries(${test_name} PRIVATE Qt6::Mqtt)
        target_compile_definitions(${test_name} PRIVATE HAS_QT_MQTT=1)
    endif()
    
    # 包含目录（使用 src 作为根目录，这样 #include "core/DataProcessor.h" 能正常工作）
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers
    )
    
    # 启用 Qt MOC/UIC/RCC
    set_target_properties(${test_name} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
    )
    
    # 编译选项
    target_compile_options(${test_name} PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
    )
    
    # 注册测试
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    # 设置测试属性
    set_tests_properties(${test_name} PROPERTIES
        TIMEOUT 30
        LABELS "unit"
    )
endforeach()

# =============================================================================
# 打印测试配置信息
# =============================================================================

message(STATUS "Configured tests:")
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    message(STATUS "  - ${test_name}")
endforeach()
