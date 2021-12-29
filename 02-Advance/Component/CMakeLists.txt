cmake_minimum_required (VERSION 3.8.1)

add_library(imgui
    STATIC
    imgui/imconfig.h
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_internal.h
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
)
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

add_library(implot
    STATIC
    implot/implot.h
    implot/implot.cpp
    implot/implot_internal.h
    implot/implot_items.cpp
    implot/implot_demo.cpp
)
target_include_directories(implot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
target_include_directories(implot PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/implot)
target_link_libraries(implot PRIVATE imgui)


function(get_all_targets var)
    set(targets)
    get_all_targets_recursive(targets ${CMAKE_CURRENT_SOURCE_DIR})
    set(${var} ${targets} PARENT_SCOPE)
endfunction()

macro(get_all_targets_recursive targets dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        get_all_targets_recursive(${targets} ${subdir})
    endforeach()
    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${targets} ${current_targets})
endmacro()

add_subdirectory(assimp)

add_library(QFPSCamera
    STATIC
    QFPSCamera/QFPSCamera.cpp
    QFPSCamera/QFPSCamera.h
)
target_link_libraries(QFPSCamera PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(QFPSCamera PUBLIC ${CMAKE_CURRENT_LIST_DIR}/QFPSCamera)

get_all_targets(TARGET_LIST)

foreach(TARGET_ITEM ${TARGET_LIST})
    set_target_properties(${TARGET_ITEM} PROPERTIES FOLDER "Advance/Component")
endforeach()


