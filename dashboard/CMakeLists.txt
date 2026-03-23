cmake_minimum_required(VERSION 3.21)
project(HeteroLinkHost VERSION 0.1.0 LANGUAGES CXX)

# vcpkg 清单模式集成
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 使用 vcpkg 提供的 Qt6 和 MQTT
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort Charts)
find_package(Qt6 COMPONENTS Mqtt)
set(QT_VERSION_MAJOR 6)
set(HAS_QT_CHARTS TRUE)
if(Qt6Mqtt_FOUND)
    set(HAS_QT_MQTT TRUE)
else()
    set(HAS_QT_MQTT FALSE)
    message(WARNING "Qt6Mqtt not found, MQTT support will be disabled")
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
    src/ui/LogPanel.cpp
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
    src/ui/LogPanel.h
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

# 链接 Qt 库 (vcpkg 提供)
target_link_libraries(heterolink-host PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Charts
)
if(HAS_QT_MQTT)
    target_link_libraries(heterolink-host PRIVATE Qt6::Mqtt)
    target_compile_definitions(heterolink-host PRIVATE HAS_QT_MQTT=1)
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

# Windows 部署：使用 windeployqt 自动复制 Qt 依赖
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "$ENV{QT_DIR}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET heterolink-host POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" 
                    --release
                    --no-compiler-runtime
                    --no-opengl-sw
                    --no-translations
                    \"$<TARGET_FILE:heterolink-host>\"
            COMMENT "Deploying Qt dependencies with windeployqt..."
        )
    else()
        message(WARNING "windeployqt not found. Qt dependencies will not be deployed automatically.")
    endif()
endif()

# 测试
enable_testing()
add_subdirectory(tests)
if(HAS_QT_MQTT)
    add_subdirectory(examples)
else()
    message(STATUS "Skipping examples (requires Qt6Mqtt)")
endif()
