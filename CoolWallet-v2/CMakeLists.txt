cmake_minimum_required(VERSION 3.16)
project(CoolWallet VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Widgets)
add_executable(CoolWallet
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    resources.qrc
)
target_include_directories(CoolWallet PRIVATE include ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(CoolWallet Qt5::Widgets)
