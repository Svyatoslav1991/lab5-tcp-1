set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Test)

add_executable(test_server_packetprotocol
    test_packetprotocol.cpp
    ../app/src/packetprotocol.cpp
    ../app/include/packetprotocol.h
)

target_include_directories(test_server_packetprotocol
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
)

target_link_libraries(test_server_packetprotocol
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME test_server_packetprotocol COMMAND test_server_packetprotocol)

add_executable(test_server_servercontroller
    test_servercontroller.cpp
    ../app/src/servercontroller.cpp
    ../app/include/servercontroller.h
    ../app/src/packetprotocol.cpp
    ../app/include/packetprotocol.h
)

target_include_directories(test_server_servercontroller
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
)

target_link_libraries(test_server_servercontroller
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME test_server_servercontroller COMMAND test_server_servercontroller)

add_executable(test_server_serversettings
    test_serversettings.cpp
    ../app/src/serversettings.cpp
    ../app/include/serversettings.h
)

target_include_directories(test_server_serversettings
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../app/include
)

target_link_libraries(test_server_serversettings
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME test_server_serversettings COMMAND test_server_serversettings)
