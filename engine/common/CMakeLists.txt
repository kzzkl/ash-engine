project(ash-common)

add_library(${PROJECT_NAME} STATIC
    ./source/log.cpp)
add_library(ash::common ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        spdlog::spdlog
        nlohmann_json::nlohmann_json)

install(DIRECTORY ./include/ DESTINATION include/ash)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)