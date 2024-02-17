set(pri_req nvs_flash ConfigType utilities peripherals)
idf_component_register(SRCS "ConfigType.c"
    INCLUDE_DIRS "."
    REQUIRES ${pri_req}
)
