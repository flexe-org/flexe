#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <dht.h>
#include <bmp280.h>

#define SENSOR_TYPE DHT_TYPE_DHT11
#define DHT_GPIO_PIN GPIO_NUM_15

void dht_test(void *pvParameters)
{
    float temperature, humidity;

    while (1)
    {
        if (dht_read_float_data(SENSOR_TYPE, DHT_GPIO_PIN, &humidity, &temperature) == ESP_OK)
            printf("[DHT] Temp: %.5fC Humidity: %.5f%%\n", temperature, humidity);
        else
            printf("[DHT] Could not read data from sensor\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void bmp_test(void *pvParameters)
{
    bmp280_t dev;
    bmp280_params_t params;
    float temperature, pressure, humidity;
    printf("[BMP] starting\n");
    memset(&dev, 0, sizeof(dev));
    i2cdev_init();

    if (bmp280_init_default_params(&params) != ESP_OK) {
	    printf("[BMP] Cannot get default params\n");
	    return;
    }

    if (bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_0, 0, GPIO_NUM_0, GPIO_NUM_1) != ESP_OK) {
	    printf("[BMP] Cannot initialize BMP280\n");
	    return;
    }

    if (bmp280_init(&dev, &params) != ESP_OK) {
	    printf("[BMP] Cannot init bmp280\n");
	    return;
    }

    while (1)
    {
        if (bmp280_read_float(&dev, &temperature, &pressure, &humidity) == ESP_OK)
            printf("[BMP] Temp: %.5fC Humidity: %.5f%% Pressure: %.5fP\n", temperature, humidity, pressure);
        else
            printf("[BMP] Could not read data from sensor\n");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    xTaskCreate(dht_test, "dht_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    xTaskCreate(bmp_test, "bmp_test", 4096, NULL, 5, NULL);
    vTaskDelay(pdMS_TO_TICKS(1000000));
}
