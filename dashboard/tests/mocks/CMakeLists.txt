# =============================================================================
# HeteroLink Host - Mock 库配置
# =============================================================================
# 创建 Mock 库供测试使用
# =============================================================================

# Mock 源文件
set(MOCK_SOURCES
    MockUartChannel.cpp
    MockMqttChannel.cpp
    MockDeviceManager.cpp
)

set(MOCK_HEADERS
    MockUartChannel.h
    MockMqttChannel.h
    MockDeviceManager.h
)

# 创建静态库
add_library(heterolink_mocks STATIC
    ${MOCK_SOURCES}
    ${MOCK_HEADERS}
)

# 链接必要的库
target_link_libraries(heterolink_mocks PUBLIC
    Qt6::Core
    Qt6::SerialPort
    heterolink-host  # 链接主项目以访问基类
)

# 包含目录
target_include_directories(heterolink_mocks PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

# 启用 Qt MOC
set_target_properties(heterolink_mocks PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# 编译选项
target_compile_options(heterolink_mocks PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# 打印配置信息
message(STATUS "Mock library configured: heterolink_mocks")
