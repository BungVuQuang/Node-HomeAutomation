set(pri_req )
idf_component_register(SRCS "AlarmHandle.c"
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
