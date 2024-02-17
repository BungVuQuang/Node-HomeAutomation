set(pri_req infrared_tools HD44780)
idf_component_register(SRCS "IrHandle.c"
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
