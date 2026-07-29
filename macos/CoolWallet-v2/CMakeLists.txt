cmake_minimum_required(VERSION 3.21.1)

project(CoolWallet VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(CoolWallet MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
    resources.qrc
)

target_include_directories(
    CoolWallet
    PRIVATE
        include
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(
    CoolWallet
    PRIVATE
        Qt5::Widgets
)

set_target_properties(
    CoolWallet
    PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "CoolWallet"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.cool-coin.coolwallet"
        MACOSX_BUNDLE_BUNDLE_VERSION "27.1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "27.1.0"
)
