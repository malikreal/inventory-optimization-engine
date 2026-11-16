cmake_minimum_required(VERSION 3.16)

project(InventoryOptimization LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.2.1
)

FetchContent_MakeAvailable(crow)

add_executable(inventory_server main.cpp)

target_link_libraries(inventory_server PRIVATE
    Crow::Crow
    Threads::Threads
)
