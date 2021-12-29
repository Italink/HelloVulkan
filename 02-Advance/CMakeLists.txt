file(GLOB ADVANCE_EXAMPLE_LIST RELATIVE  ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)

list(REMOVE_ITEM ADVANCE_EXAMPLE_LIST Component)

foreach(ADVANCE_EXAMPLE_NAME ${ADVANCE_EXAMPLE_LIST})
    if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${ADVANCE_EXAMPLE_NAME})
        add_example(${ADVANCE_EXAMPLE_NAME})
        set_target_properties(${ADVANCE_EXAMPLE_NAME} PROPERTIES FOLDER "Advance")
    endif()
endforeach()

add_subdirectory(Component)

target_link_libraries(Ex04-QImGUI PRIVATE imgui implot)

target_link_libraries(Ex00-QFPSCamera PRIVATE QFPSCamera)

target_link_libraries(Ex01-SkyBox_CubeMaps PRIVATE QFPSCamera)

target_link_libraries(Ex05-Assimp PRIVATE assimp)

add_custom_command(TARGET Ex05-Assimp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:assimp> $<TARGET_FILE_DIR:Ex05-Assimp>
    COMMAND_EXPAND_LISTS
)