cmake_minimum_required(VERSION 3.10)
project(SolarSystemOpenGL CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
pkg_search_module(GLEW REQUIRED glew)

# GLM is header-only; locate it via pkg-config or standard paths
find_path(GLM_INCLUDE_DIR glm/glm.hpp
    PATHS /usr/include /usr/local/include
    REQUIRED
)

set(SOURCES
    src/main.cpp
    src/Shader.cpp
    src/Camera.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${OPENGL_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIR}
    src
)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${GLFW_CFLAGS_OTHER}
    ${GLEW_CFLAGS_OTHER}
)

target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${GLEW_LIBRARIES}
)

# Copy shaders alongside the executable after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying shaders to build directory"
)
