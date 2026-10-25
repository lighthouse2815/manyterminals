cmake_minimum_required(VERSION 3.20)
project(ManyTerminals VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)
find_package(KF6Parts REQUIRED)
find_package(KF6CoreAddons REQUIRED)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/TerminalTile.cpp
    src/TerminalTile.h
)

add_executable(manyterminals ${SOURCES})

target_link_libraries(manyterminals PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    KF6::Parts
    KF6::CoreAddons
)

install(TARGETS manyterminals DESTINATION bin)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(window-controls-test
        tests/WindowControlsTest.cpp
        src/MainWindow.cpp
        src/TerminalTile.cpp
    )
    target_include_directories(window-controls-test PRIVATE src)
    target_link_libraries(window-controls-test PRIVATE
        Qt6::Widgets Qt6::Test KF6::Parts KF6::CoreAddons
    )
    add_test(NAME window-controls COMMAND window-controls-test)
    set_tests_properties(window-controls PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
