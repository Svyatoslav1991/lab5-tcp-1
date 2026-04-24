set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(SERVER_APP_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.ui
    src/packetprotocol.cpp
    src/serversettings.cpp
    src/servercontroller.cpp
)

set(SERVER_APP_HEADERS
    include/mainwindow.h
    include/packetprotocol.h
    include/serversettings.h
    include/servercontroller.h
)

add_executable(lab5_tcp_server
    ${SERVER_APP_SOURCES}
    ${SERVER_APP_HEADERS}
)

target_include_directories(lab5_tcp_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(lab5_tcp_server
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
)

if(WIN32 AND QT_VERSION_MAJOR EQUAL 5)
    target_link_libraries(lab5_tcp_server PRIVATE Qt5::WinMain)
endif()
