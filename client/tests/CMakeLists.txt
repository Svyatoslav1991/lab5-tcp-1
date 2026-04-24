set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Test)

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
