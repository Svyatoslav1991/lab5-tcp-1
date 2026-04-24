set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network Test)

add_executable(test_sessionmode
    test_sessionmode.cpp
)

target_include_directories(test_sessionmode
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
)

target_link_libraries(test_sessionmode
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME test_sessionmode COMMAND test_sessionmode)

add_executable(test_clientsettings
    test_clientsettings.cpp
    ../app/src/clientsettings.cpp
    ../app/include/clientsettings.h
    ../app/include/sessionmode.h
)

target_include_directories(test_clientsettings
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
)

target_link_libraries(test_clientsettings
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME test_clientsettings COMMAND test_clientsettings)

add_executable(test_clientcontroller
    test_clientcontroller.cpp
    ../app/src/clientcontroller.cpp
    ../app/include/clientcontroller.h
    ../app/src/packetprotocol.cpp
    ../app/include/packetprotocol.h
    ../app/include/sessionmode.h
)

target_include_directories(test_clientcontroller
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
)

target_link_libraries(test_clientcontroller
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME test_clientcontroller COMMAND test_clientcontroller)

add_executable(test_packetprotocol
    test_packetprotocol.cpp
    ../app/src/packetprotocol.cpp
    ../app/include/packetprotocol.h
)

target_include_directories(test_packetprotocol
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
)

target_link_libraries(test_packetprotocol
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME test_packetprotocol COMMAND test_packetprotocol)

add_executable(test_mainwindow
    test_mainwindow.cpp
    ../app/src/mainwindow.cpp
    ../app/src/mainwindow.ui
    ../app/include/mainwindow.h
    ../app/src/clientcontroller.cpp
    ../app/include/clientcontroller.h
    ../app/src/clientsettings.cpp
    ../app/include/clientsettings.h
    ../app/src/packetprotocol.cpp
    ../app/include/packetprotocol.h
    ../app/include/sessionmode.h
)

target_include_directories(test_mainwindow
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
)

target_link_libraries(test_mainwindow
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME test_mainwindow COMMAND test_mainwindow)
