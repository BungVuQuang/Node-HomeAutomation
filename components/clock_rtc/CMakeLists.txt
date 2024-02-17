set(pri_req nvs_flash main ble_mesh_handle ConfigType)
idf_component_register(SRCS "clock_rtc.c"
    INCLUDE_DIRS "."
    REQUIRES ${pri_req}
)
