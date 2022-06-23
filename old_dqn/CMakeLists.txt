cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(main VERSION 0.0.1
				DESCRIPTION "Lets get torch Working for a DQN"
				LANGUAGES CXX)


find_package(Torch REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main "${TORCH_LIBRARIES}")
set_property(TARGET main PROPERTY CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_C_FLAGS "-w -O0 -ggdb")
set(CMAKE_C_FLAGS_DEBUG "-w -O0 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "-w -O0 -ggdb")
set(CMAKE_CXX_FLAGS "-w -O0 -ggdb")
set(CMAKE_CXX_FLAGS_DEBUG "-w -O0 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-w -O0 -ggdb")