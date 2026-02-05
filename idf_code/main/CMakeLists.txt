file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "ui/*.c")
file(GLOB_RECURSE DISPLAY ${CMAKE_SOURCE_DIR} "display/*.c")
file(GLOB_RECURSE DIAL ${CMAKE_SOURCE_DIR} "dial/*.cpp")
file(GLOB_RECURSE USB ${CMAKE_SOURCE_DIR} "usb_device/*.c")
file(GLOB_RECURSE POWER ${CMAKE_SOURCE_DIR} "dial_power/*.c")
file(GLOB_RECURSE NVS_DATA ${CMAKE_SOURCE_DIR} "nvs_data/*.c")
file(GLOB_RECURSE WIFI ${CMAKE_SOURCE_DIR} "wifi/*.c")
file(GLOB_RECURSE SNTP ${CMAKE_SOURCE_DIR} "sntp_time/*.c")
file(GLOB_RECURSE SPIFFS ${CMAKE_SOURCE_DIR} "spiffs_init/*.c")
idf_component_register(SRCS "main.c" ${SRC_UI} ${DIAL} ${DISPLAY} ${USB} ${POWER} ${NVS_DATA} ${WIFI} ${SNTP} ${SPIFFS}
                    INCLUDE_DIRS "." "ui" "dial" "display" "usb_device" "dial_power" "nvs_data" "wifi" "sntp_time" "spiffs_init"
                    )

