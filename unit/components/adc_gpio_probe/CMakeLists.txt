idf_component_register(
    SRCS "adc_gpio_probe.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_gpio esp_adc
)
