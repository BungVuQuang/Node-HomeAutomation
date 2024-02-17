set(pri_req)
idf_component_register(SRCS "BH1750.c"
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
