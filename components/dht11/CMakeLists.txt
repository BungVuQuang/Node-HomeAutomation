set(pri_req esp_timer)
idf_component_register(SRCS "dht11.c"
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
