get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ProjectId})

file(GLOB PROJECT_SOURCE FILES *.h *.cpp)

file(GLOB SHADER_SOURCE FILES *.vert *.frag *.comp)
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