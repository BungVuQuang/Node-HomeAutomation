# set(pri_req infrared_tools HD44780)
set(component_srcs "panasonic_frame.c"
                   "panasonic_ir.c")

idf_component_register(SRCS "${component_srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
