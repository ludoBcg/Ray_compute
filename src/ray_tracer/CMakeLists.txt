# check CMAKE version
cmake_minimum_required(VERSION 3.0)

# create project
project(Ray_tracer)

# use C++ 20
set(CMAKE_CXX_STANDARD 20)

# add files
set(SRCS
	utils.h
	pathTracing.cpp
    )

find_package(OpenMP)

# Add executable for project
add_executable(${PROJECT_NAME} ${SRCS} )

target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX )


# Install executable
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

