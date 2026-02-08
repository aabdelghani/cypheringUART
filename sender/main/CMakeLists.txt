idf_component_register(SRCS "main.c" "aes_wrapper.c" "../../tiny-AES-c/aes.c"
                    INCLUDE_DIRS "." "../../tiny-AES-c"
                    PRIV_REQUIRES mbedtls esp_driver_uart esp_driver_gpio)
