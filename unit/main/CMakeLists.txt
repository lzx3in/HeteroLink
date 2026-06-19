idf_component_register(
    SRCS "app_main.c"
    PRIV_REQUIRES esp-tls esp_wifi esp_event nvs_flash esp_netif protocol_examples_common mqtt spi_dma adc_gpio_probe
    INCLUDE_DIRS "."
)
