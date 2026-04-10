cmake_minimum_required(VERSION 3.16)
project(SSR_Project)

set(CMAKE_CXX_STANDARD 17)

# GLAD
add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# GLFW
set(GLFW_PATH ${CMAKE_SOURCE_DIR}/external/GLFW)
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION ${GLFW_PATH}/lib/glfw3.lib
    INTERFACE_INCLUDE_DIRECTORIES ${GLFW_PATH}/include
)

# GLM
set(GLM_PATH ${CMAKE_SOURCE_DIR}/external/glm)

# ImGui
add_library(imgui
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    external/imgui
    external/imgui/backends
    external/GLFW/include
    external/glad/include
)

# Executable
add_executable(SSR_Project src/main.cpp)

target_include_directories(SSR_Project PRIVATE
    external/glad/include
    external/GLFW/include
    external/glm
    external/stb
    external/imgui
    external/imgui/backends
)

target_link_libraries(SSR_Project
    glad
    glfw
    imgui
    opengl32
)



# Set the working directory to the project root when debugging
set_target_properties(SSR_Project PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "C:/Users/naudiyag/source/repos/SSR_Project/SSR_Project"
)