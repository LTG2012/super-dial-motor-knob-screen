cmake_minimum_required(VERSION 3.20)
project(superdial_ui_sim C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE=1 -DLV_LVGL_H_INCLUDE_SIMPLE=1 -DLV_SIMULATOR=1)

set(LVGL_DIR "${CMAKE_SOURCE_DIR}/../idf_code/components/lvgl__lvgl")
set(IDF_MAIN_DIR "${CMAKE_SOURCE_DIR}/../idf_code/main")
set(UI_DIR "${IDF_MAIN_DIR}/ui")

file(GLOB_RECURSE LVGL_SOURCES
    "${LVGL_DIR}/src/*.c"
)

file(GLOB UI_SOURCES
    "${UI_DIR}/*.c"
    "${UI_DIR}/screens/*.c"
    "${UI_DIR}/images/*.c"
    "${UI_DIR}/fonts/*.c"
)

add_executable(superdial_ui_sim
    main.c
    pc_sim_stubs.c
    ${LVGL_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(superdial_ui_sim PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/stubs
    ${LVGL_DIR}
    ${LVGL_DIR}/src
    ${IDF_MAIN_DIR}
    ${IDF_MAIN_DIR}/nvs_data
    ${UI_DIR}
    ${UI_DIR}/screens
    ${UI_DIR}/images
    ${UI_DIR}/fonts
)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(superdial_ui_sim PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
