cmake_minimum_required(VERSION 3.15)

project(tracerlite LANGUAGES C)

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/$<CONFIG>)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.c")

add_executable(tracerlite ${SOURCES})

target_compile_features(tracerlite PRIVATE c_std_11)
target_include_directories(tracerlite PRIVATE ${CMAKE_SOURCE_DIR}/include ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(tracerlite PRIVATE TRACERLITE_PROJECT_DIR=\"${CMAKE_SOURCE_DIR}\")
target_link_libraries(tracerlite PRIVATE glfw ${Vulkan_LIBRARIES})

if(WIN32)
    target_compile_definitions(tracerlite PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()


find_program(GLSLC_EXECUTABLE NAMES glslc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found; install the Vulkan SDK or set VULKAN_SDK")
endif()

set(VERT_SPV ${CMAKE_SOURCE_DIR}/shaders/vert.spv)
set(FRAG_SPV ${CMAKE_SOURCE_DIR}/shaders/frag.spv)

add_custom_command(
    OUTPUT ${VERT_SPV}
    COMMAND ${GLSLC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/shaders/shader.vert -o ${VERT_SPV}
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/shader.vert
    COMMENT "Compiling shader.vert to vert.spv"
)

add_custom_command(
    OUTPUT ${FRAG_SPV}
    COMMAND ${GLSLC_EXECUTABLE} ${CMAKE_SOURCE_DIR}/shaders/shader.frag -o ${FRAG_SPV}
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/shader.frag
    COMMENT "Compiling shader.frag to frag.spv"
)

add_custom_target(shaders DEPENDS ${VERT_SPV} ${FRAG_SPV})
add_dependencies(tracerlite shaders)