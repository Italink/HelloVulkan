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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets  REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets  REQUIRED)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DNOMINMAX -DVK_USE_PLATFORM_WIN32_KHR)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
else()
    message(FATAL_ERROR, "Vulkan-Hpp: unhandled platform for samples!")
endif()

function(add_example SUB_DIR)
    get_filename_component(ProjectId ${SUB_DIR} NAME)
    file(GLOB PROJECT_SOURCE FILES ${CMAKE_CURRENT_LIST_DIR}/${SUB_DIR}/*.h  ${CMAKE_CURRENT_LIST_DIR}/${SUB_DIR}/*.cpp)

    file(GLOB SHADER_SOURCE FILES   ${CMAKE_CURRENT_LIST_DIR}/${SUB_DIR}/*.vert  ${SUB_DIR}/*.frag   ${CMAKE_CURRENT_LIST_DIR}/${SUB_DIR}/*.comp)
    foreach(SHADER ${SHADER_SOURCE})
        target_glsl_to_spriv(${ProjectId} ${SHADER})
    endforeach()
    source_group("GLSL Files" FILES ${SHADER_SOURCE})

    add_executable(${ProjectId} 
        ${PROJECT_SOURCE}
        ${SHADER_SOURCE}
    )
    target_link_libraries(${ProjectId} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
endfunction()

add_subdirectory(01-QuickStart)
add_subdirectory(02-Advance)