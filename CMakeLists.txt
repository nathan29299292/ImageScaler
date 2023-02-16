cmake_minimum_required(VERSION 3.5)

project(ImageScaler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ImageScaler main.cpp image.h image.cpp)

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})
target_link_libraries(ImageScaler ${PNG_LIBRARY})

install(TARGETS ImageScaler
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
