project(ash-ui)

# Build yoga layout engine.
file(GLOB_RECURSE ASH_YOGA_SOURCE ${ASH_THIRD_PARTY_DIR}/yoga/yoga/*.cpp)
add_library(ash-yoga STATIC ${ASH_YOGA_SOURCE})
target_include_directories(ash-yoga PUBLIC ${ASH_THIRD_PARTY_DIR}/yoga/.)
add_library(ash::yoga ALIAS ash-yoga)

# Build ash ui system.
add_library(${PROJECT_NAME} STATIC
    ./source/controls/button.cpp
    ./source/controls/collapse.cpp
    ./source/controls/dock_area.cpp
    ./source/controls/dock_element.cpp
    ./source/controls/dock_window.cpp
    ./source/controls/font_icon.cpp
    ./source/controls/image.cpp
    ./source/controls/input.cpp
    ./source/controls/label.cpp
    ./source/controls/panel.cpp
    ./source/controls/scroll_view.cpp
    ./source/controls/tree.cpp
    ./source/controls/view.cpp
    ./source/element_layout.cpp
    ./source/element_tree.cpp
    ./source/element.cpp
    ./source/font.cpp
    ./source/renderer.cpp
    ./source/ui_pipeline.cpp
    ./source/ui.cpp)
add_library(ash::ui ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        ash::common
        ash::core
        ash::graphics
    PRIVATE
        freetype
        ash::yoga)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)