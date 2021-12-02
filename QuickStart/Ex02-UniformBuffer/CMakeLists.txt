get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ProjectId})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
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

file(GLOB PROJECT_SOURCE FILES *.h *.cpp)

file(GLOB SHADER_SOURCE FILES *.vert *.frag)
source_group("GLSL Files" FILES ${SHADER_SOURCE})

add_executable(${ProjectId} 
    ${PROJECT_SOURCE}
    ${SHADER_SOURCE}
)

foreach(SHADER ${SHADER_SOURCE})
    target_glsl_to_spriv(${ProjectId} ${SHADER})
endforeach()

target_link_libraries(${ProjectId} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(${ProjectId} PROPERTIES 
                      FOLDER "Examples")