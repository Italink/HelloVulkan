
file(GLOB ADVANCE_EXAMPLE_LIST RELATIVE  ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)

list(REMOVE_ITEM ADVANCE_EXAMPLE_LIST 3rdParty)

foreach(ADVANCE_EXAMPLE_NAME ${ADVANCE_EXAMPLE_LIST})
    if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${ADVANCE_EXAMPLE_NAME})
        add_example(${ADVANCE_EXAMPLE_NAME})
        set_target_properties(${ADVANCE_EXAMPLE_NAME} PROPERTIES FOLDER "Advance")
    endif()
endforeach()

add_subdirectory(3rdParty)

target_link_libraries(02-ImGUI PRIVATE imgui implot)




