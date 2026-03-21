cmake_minimum_required(VERSION 3.16)
project(HeteroLinkHost VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 优先查找 Qt6，如果没有则使用 Qt5
find_package(Qt6 COMPONENTS Core Gui Widgets SerialPort QUIET)
if(Qt6_FOUND)
    find_package(Qt6 OPTIONAL_COMPONENTS Charts Mqtt)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets SerialPort)
    find_package(Qt5 OPTIONAL_COMPONENTS Charts)
    # Qt5 没有官方的 MQTT 模块，需要手动查找
    find_path(QT_MQTT_INCLUDE_DIR QMqttClient
        PATHS /usr/include/qt5/QMqtt /usr/local/include/qt5/QMqtt
        PATH_SUFFIXES Qt5QMqtt
    )
    set(QT_VERSION_MAJOR 5)
endif()

# 检查是否找到 Qt MQTT
if(Qt6_FOUND AND TARGET Qt6::Mqtt)
    set(HAS_QT_MQTT TRUE)
    message(STATUS "Qt6 MQTT module found")
elseif(QT_MQTT_INCLUDE_DIR)
    set(HAS_QT_MQTT TRUE)
    message(STATUS "Qt5 MQTT module found (manual)")
else()
    set(HAS_QT_MQTT FALSE)
    message(STATUS "Qt MQTT module not found - MQTT support disabled")
endif()

# 源文件
set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/core/DeviceManager.cpp
    src/core/DataProcessor.cpp
    src/core/AlarmSystem.cpp
    src/protocol/UartChannel.cpp
    src/protocol/MqttChannel.cpp
    src/protocol/Protocol.cpp
    src/ui/MainWindow.cpp
    src/ui/DevicePanel.cpp
    src/ui/DataWidget.cpp
    src/ui/ConfigPanel.cpp
    src/storage/ConfigManager.cpp
    src/storage/DataLogger.cpp
    src/utils/Logger.cpp
)

set(HEADERS
    src/app/Application.h
    src/core/DeviceManager.h
    src/core/DataProcessor.h
    src/core/AlarmSystem.h
    src/protocol/UartChannel.h
    src/protocol/MqttChannel.h
    src/protocol/Protocol.h
    src/ui/MainWindow.h
    src/ui/DevicePanel.h
    src/ui/DataWidget.h
    src/ui/ConfigPanel.h
    src/storage/ConfigManager.h
    src/storage/DataLogger.h
    src/utils/Logger.h
)

set(UI_FILES
    src/ui/MainWindow.ui
    src/ui/DevicePanel.ui
    src/ui/DataWidget.ui
    src/ui/ConfigPanel.ui
)

set(RESOURCES
    resources/resources.qrc
)

# 创建可执行文件
add_executable(heterolink-host
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCES}
)

# 链接 Qt 库
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(heterolink-host PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::SerialPort
    )
    if(TARGET Qt6::Charts)
        target_link_libraries(heterolink-host PRIVATE Qt6::Charts)
        target_compile_definitions(heterolink-host PRIVATE HAS_QT_CHARTS)
    endif()
    if(TARGET Qt6::Mqtt)
        target_link_libraries(heterolink-host PRIVATE Qt6::Mqtt)
        target_compile_definitions(heterolink-host PRIVATE HAS_QT_MQTT)
    endif()
else()
    target_link_libraries(heterolink-host PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::SerialPort
    )
    if(TARGET Qt5::Charts)
        target_link_libraries(heterolink-host PRIVATE Qt5::Charts)
        target_compile_definitions(heterolink-host PRIVATE HAS_QT_CHARTS)
    endif()
    # Qt5 MQTT 手动链接
    if(HAS_QT_MQTT)
        target_include_directories(heterolink-host PRIVATE ${QT_MQTT_INCLUDE_DIR})
        target_compile_definitions(heterolink-host PRIVATE HAS_QT_MQTT)
        # 需要链接 Qt5MQTT 库（如果可用）
        find_library(QT_MQTT_LIBRARY NAMES Qt5MQTT Qt5QMqtt
            PATHS /usr/lib /usr/local/lib
            PATH_SUFFIXES x86_64-linux-gnu
        )
        if(QT_MQTT_LIBRARY)
            target_link_libraries(heterolink-host PRIVATE ${QT_MQTT_LIBRARY})
        endif()
    endif()
endif()

# 包含目录
target_include_directories(heterolink-host PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# 编译选项
target_compile_options(heterolink-host PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# 安装规则
install(TARGETS heterolink-host
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 测试
enable_testing()
add_subdirectory(tests)

# 示例程序
add_subdirectory(examples)
