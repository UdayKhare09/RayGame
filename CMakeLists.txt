cmake_minimum_required(VERSION 3.20)
project(RayGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use mold linker
add_link_options("-fuse-ld=mold")

# Find dependencies
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

# Add vk-bootstrap
add_subdirectory(libs/vk-bootstrap)

# Find glslc for shader compilation
find_program(GLSLC_EXECUTABLE glslc HINTS ${Vulkan_SDK}/bin)
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Please install Vulkan SDK.")
endif()

# Shader compilation function
function(compile_shader SHADER_SOURCE SHADER_BINARY)
    add_custom_command(
        OUTPUT ${SHADER_BINARY}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SOURCE} -o ${SHADER_BINARY}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling ${SHADER_SOURCE} to ${SHADER_BINARY}"
    )
endfunction()

# Create shaders directory in build
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

# Compile shaders
set(SHADER_SOURCES 
    src/shaders/raytracer.vert
    src/shaders/raytracer.frag
)
set(SHADER_BINARIES "")

foreach(SOURCE ${SHADER_SOURCES})
    get_filename_component(FILENAME ${SOURCE} NAME)
    set(BINARY ${CMAKE_BINARY_DIR}/shaders/${FILENAME}.spv)
    compile_shader(${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE} ${BINARY})
    list(APPEND SHADER_BINARIES ${BINARY})
endforeach()

include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

# Add executable
add_executable(RayGame 
    src/main.cpp 
    src/Renderer.cpp
    ${SHADER_BINARIES}
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

target_include_directories(RayGame PRIVATE 
    src 
    ${imgui_SOURCE_DIR} 
    ${imgui_SOURCE_DIR}/backends
)

# Include directories
target_include_directories(RayGame PRIVATE 
    ${SDL2_INCLUDE_DIRS} 
    libs/vk-bootstrap/src
)

# Link libraries
target_link_libraries(RayGame PRIVATE 
    vk-bootstrap 
    ${SDL2_LIBRARIES} 
    Vulkan::Vulkan
)

# Copy shaders to executable directory (optional, but good for running)
add_custom_command(TARGET RayGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/shaders
    ${CMAKE_CURRENT_BINARY_DIR}/shaders
)
