# check CMAKE version
cmake_minimum_required(VERSION 3.0)

# create project
project(Ray_compute)

# use C++ 20
set(CMAKE_CXX_STANDARD 20)

# add files
set(SRCS
	src/main.cpp
	src/drawablemesh.cpp
    )
    
set(HEADERS
	src/utils.h
	src/drawablemesh.h
    )
	

# Add source directories
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/src" PROJECT_SRCS)

# Dependencies folder
set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libs")
include_directories(SYSTEM "${LIBS_DIR}")
	
################################# EXTERNAL LIBS ######################


# OpenGL
find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
  include_directories(SYSTEM ${OPENGL_INCLUDE_DIR})
endif(OPENGL_FOUND)
# flag for conditional compilation
add_compile_definitions(USE_OPENGL)


# GLEW (download binaries for windows)
set(GLEW_DIR "${LIBS_DIR}/third_party/glew-2.1.0")
include_directories(${GLEW_DIR}/include)
link_directories(${GLEW_DIR}/lib/Release/x64)
# use glew32s only
SET(GLEW_LIBS glew32s.lib)
add_definitions(-DGLEW_STATIC -DGLEW_NO_GLU)


# GLFW (to compile before)
set(GLFW_DIR "${LIBS_DIR}/third_party/glfw-3.4")
include_directories(${GLFW_DIR}/include)
link_directories(${GLFW_DIR}/build/src/Release)
SET(GLFW_LIBS glfw3.lib)


# GLM (Header only)
include_directories(SYSTEM "${LIBS_DIR}/third_party/glm-1.0.1")


# ImGui (just add sources to project)
set(IMGUI_DIR "${LIBS_DIR}/third_party/imgui")
aux_source_directory("${IMGUI_DIR}" PROJECT_SRCS)
# Add appropriate backends
set(IMGUI_BCK "${IMGUI_DIR}/backends/imgui_impl_glfw.h"
			  "${IMGUI_DIR}/backends/imgui_impl_opengl3.h"
			  "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
			  "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp")					
include_directories(SYSTEM "${IMGUI_DIR}")
include_directories(SYSTEM "${IMGUI_DIR}/backends")


################################# BUILD PROJECT ######################

SET(PROJECT_LIBRARIES ${PROJECT_LIBRARIES} ${LIBQGLVIEWER_LIBS_PATHS})###
	
# Add executable for project
add_executable(${PROJECT_NAME} ${PROJECT_SRCS} ${SRCS} ${HEADERS} ${IMGUI_BCK})

target_link_libraries(${PROJECT_NAME} ${GLFW_LIBS} ${GLEW_LIBS} ${OPENGL_LIBRARIES})

# Install executable
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

