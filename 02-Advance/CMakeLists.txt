file(GLOB ADVANCE_EXAMPLE_LIST RELATIVE  ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)

list(REMOVE_ITEM ADVANCE_EXAMPLE_LIST Component)

foreach(ADVANCE_EXAMPLE_NAME ${ADVANCE_EXAMPLE_LIST})
    if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${ADVANCE_EXAMPLE_NAME})
        add_example(${ADVANCE_EXAMPLE_NAME})
        set_target_properties(${ADVANCE_EXAMPLE_NAME} PROPERTIES FOLDER "Advance")
    endif()
endforeach()

add_library(QFpsCamera
    STATIC
    Ex00-QFpsCamera/QFpsCamera.cpp
    Ex00-QFpsCamera/QFpsCamera.h
)
target_link_libraries(QFpsCamera PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(QFpsCamera PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Ex00-QFpsCamera)
set_target_properties(QFpsCamera PROPERTIES FOLDER "Advance/Component")

target_link_libraries(Ex01-SkyBox_CubeMaps PRIVATE QFpsCamera)
target_link_libraries(Ex03-GPUParticles PRIVATE QFpsCamera)
target_link_libraries(Ex04-QImGUI PRIVATE imgui implot)
target_link_libraries(Ex05-AssimpLoadStaticMesh PRIVATE assimp QFpsCamera)
target_link_libraries(Ex06-AssimpLoadSkeletonMesh PRIVATE assimp QFpsCamera)

add_custom_command(TARGET Ex05-AssimpLoadStaticMesh POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:assimp> $<TARGET_FILE_DIR:Ex05-AssimpLoadStaticMesh>
    COMMAND_EXPAND_LISTS
)
add_custom_command(TARGET Ex06-AssimpLoadSkeletonMesh POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:assimp> $<TARGET_FILE_DIR:Ex06-AssimpLoadSkeletonMesh>
    COMMAND_EXPAND_LISTS
)