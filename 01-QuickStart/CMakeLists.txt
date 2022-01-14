file(GLOB QUICK_START_EXAMPLE_LIST RELATIVE  ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)

foreach(QUICK_START_EXAMPLE_NAME ${QUICK_START_EXAMPLE_LIST})
    if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${QUICK_START_EXAMPLE_NAME})
        add_example(${QUICK_START_EXAMPLE_NAME})
        set_target_properties(${QUICK_START_EXAMPLE_NAME} PROPERTIES FOLDER "01-QuickStart")
    endif()
endforeach()


target_link_libraries(Ex01-Shader PUBLIC glslang SPIRV glslang-default-resource-limits)
add_custom_command(TARGET Ex01-Shader PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:glslang> $<TARGET_FILE_DIR:Ex01-Shader>
    COMMAND_EXPAND_LISTS
)
add_custom_command(TARGET Ex01-Shader PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SPIRV> $<TARGET_FILE_DIR:Ex01-Shader>
    COMMAND_EXPAND_LISTS
)