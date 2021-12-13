cmake_minimum_required(VERSION 3.12)
project(Vulkan-Exmaples CXX)

set(GLSL_VALIDATOR glslangValidator.exe)

function(target_glsl_to_spriv PROJECT_TARGET SHADER_PATH)
    get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)
    string(REPLACE "." "_" NEW_SHADER_NAME  ${SHADER_NAME})
    set(BINARY_SHADER_PATH ${CMAKE_CURRENT_BINARY_DIR}/${NEW_SHADER_NAME}.spv )
    add_custom_command(
        OUTPUT ${BINARY_SHADER_PATH}
        COMMAND ${GLSL_VALIDATOR} ${SHADER_PATH} -V -o ${BINARY_SHADER_PATH} 
        MAIN_DEPENDENCY ${SHADER_PATH}
    )       
    #set_property(SOURCE ${SHADER_PATH} APPEND PROPERTY OBJECT_DEPENDS ${BINARY_SHADER_PATH})
    #set_property(TARGET ${PROJECT_TARGET} APPEND PROPERTY SOURCES ${BINARY_SHADER_PATH})  
endfunction()

set(EXAMPLE_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/Examples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets  REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets  REQUIRED)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DNOMINMAX -DVK_USE_PLATFORM_WIN32_KHR)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
else()
    message(FATAL_ERROR, "Vulkan-Hpp: unhandled platform for samples!")
endif()

file(GLOB EXAMPLE_DIR_LIST RELATIVE ${EXAMPLE_ROOT_DIR} ${EXAMPLE_ROOT_DIR}/*)
foreach(EXAMPLE_DIR ${EXAMPLE_DIR_LIST})
    add_subdirectory(${EXAMPLE_ROOT_DIR}/${EXAMPLE_DIR})
endforeach()
add_custom_target(Vulkan-Exmaples)
