cmake_minimum_required(VERSION 3.12)

project(Vulkan-Exmaples CXX)

set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets  REQUIRED)

add_definitions(-DVK_NO_PROTOTYPES)

function(target_glsl_to_spriv PROJECT_TARGET SHADER_PATH OUTPUT_DIR)
    get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)
    string(REPLACE "." "_" NEW_SHADER_NAME ${SHADER_NAME})
    set(BINARY_SHADER_HEADER_PATH ${OUTPUT_DIR}/${NEW_SHADER_NAME}.inl)
    add_custom_command(
        OUTPUT ${BINARY_SHADER_HEADER_PATH}
        COMMAND glslangValidator.exe ${SHADER_PATH} -V -o ${BINARY_SHADER_HEADER_PATH} --vn ${NEW_SHADER_NAME} --spirv-val
        MAIN_DEPENDENCY ${SHADER_PATH}
    )       
    #set_property(SOURCE ${SHADER_PATH} APPEND PROPERTY OBJECT_DEPENDS ${BINARY_SHADER_HEADER_PATH})
    set_property(TARGET ${PROJECT_TARGET} APPEND PROPERTY SOURCES ${BINARY_SHADER_HEADER_PATH})  
    source_group("Shader Files/Generated Header" FILES ${BINARY_SHADER_HEADER_PATH})
endfunction()

function(add_example SUB_DIR)
    get_filename_component(ProjectId ${SUB_DIR} NAME)

    file(GLOB PROJECT_SOURCE FILES ${CMAKE_CURRENT_LIST_DIR}/${SUB_DIR}/*.h  ${CMAKE_CURRENT_LIST_DIR}/${SUB_DIR}/*.cpp ${CMAKE_CURRENT_LIST_DIR}/${SUB_DIR}/*.qrc)

    file(GLOB SHADER_SOURCE FILES   ${CMAKE_CURRENT_LIST_DIR}/${SUB_DIR}/*.vert  ${SUB_DIR}/*.frag   ${CMAKE_CURRENT_LIST_DIR}/${SUB_DIR}/*.comp)

    source_group("Shader Files/GLSL" FILES ${SHADER_SOURCE})
    source_group("Source Files" FILES ${PROJECT_SOURCE})

    add_executable(${ProjectId} 
        ${PROJECT_SOURCE}
        ${SHADER_SOURCE}
    )

    set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${ProjectId})
    set(PROJECT_SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/${ProjectId}/Shaders)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR})
    file(MAKE_DIRECTORY ${PROJECT_SHADER_DIR})

    target_include_directories(${ProjectId} PUBLIC ${PROJECT_SHADER_DIR})

    set_target_properties(${ProjectId} 
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )

    foreach(SHADER ${SHADER_SOURCE})
        target_glsl_to_spriv(${ProjectId} ${SHADER} ${PROJECT_SHADER_DIR})
    endforeach()

    target_link_libraries(${ProjectId} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
endfunction()

add_subdirectory(01-QuickStart)
add_subdirectory(02-Advance)
add_subdirectory(3rdParty)