get_filename_component(ProjectId ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${ProjectId})

file(GLOB PROJECT_SOURCE FILES *.h *.cpp)

file(GLOB SHADER_SOURCE FILES *.vert *.frag)
source_group("GLSL Files" FILES ${SHADER_SOURCE})

add_executable(${ProjectId} 
    ${PROJECT_SOURCE}
    ${SHADER_SOURCE}
)

add_custom_command(TARGET ${ProjectId}  POST_BUILD     
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  
        ${CMAKE_CURRENT_SOURCE_DIR}/qt256.png 
        ${CMAKE_CURRENT_BINARY_DIR}
        )

foreach(SHADER ${SHADER_SOURCE})
    target_glsl_to_spriv(${ProjectId} ${SHADER})
endforeach()

target_link_libraries(${ProjectId} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(${ProjectId} PROPERTIES 
                      FOLDER "Examples")
