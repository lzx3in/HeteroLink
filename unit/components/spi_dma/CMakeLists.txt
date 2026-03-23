idf_component_register(
    SRCS "spi_dma.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_spi esp_driver_gpio esp_hal_dma
)
