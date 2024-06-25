/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/dsp/print_format.h>
#include <stdio.h>

K_THREAD_STACK_DEFINE(dht_thread_stack_area, 1024);
struct k_thread dht_thread;
K_THREAD_STACK_DEFINE(bmp_thread_stack_area, 2048);
struct k_thread bmp_thread;

SENSOR_DT_READ_IODEV(iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(bosch_bme280),
		{SENSOR_CHAN_AMBIENT_TEMP, 0},
		{SENSOR_CHAN_HUMIDITY, 0},
		{SENSOR_CHAN_PRESS, 0});
RTIO_DEFINE(ctx, 1, 1);

void dht_read(void *p1, void *p2, void *p3)
{
	const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);

	while (!device_is_ready(dht11)) {
		printf("Device %s is not ready\n", dht11->name);
		k_sleep(K_SECONDS(2));
	}

	printf("[DHT] Zephyr starting\n");
	while (true) {
		int rc = sensor_sample_fetch(dht11);

		if (rc != 0) {
			printf("Sensor fetch failed: %d\n", rc);
			k_sleep(K_MSEC(100));
			continue;
		}

		struct sensor_value temperature;
		struct sensor_value humidity;

		rc = sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP,
					&temperature);
		if (rc == 0) {
			rc = sensor_channel_get(dht11, SENSOR_CHAN_HUMIDITY,
						&humidity);
		}
		if (rc != 0) {
			printf("get failed: %d\n", rc);
			break;
		}

		printf("[DHT]: Temp: %.5fC Humidity: %.5f%%\n",
		       sensor_value_to_double(&temperature),
		       sensor_value_to_double(&humidity));
		k_sleep(K_SECONDS(2));
	}
}

void bmp_read(void *p1, void *p2, void *p3)
{
	const struct device *const bmp280 = DEVICE_DT_GET_ONE(bosch_bme280);

	while (!device_is_ready(bmp280)) {
		printf("Device %s is not ready\n", bmp280->name);
		k_sleep(K_SECONDS(2));
	}

	printf("[BMP] Zephyr starting\n");
	while (true) {
		uint8_t buf[128];
		int rc = sensor_read(&iodev, &ctx, buf, 128);

		if (rc != 0) {
			printf("[BMP] Sensor read failed: %d\n", rc);
			break;
		}

		/* Can we move it above? */
		const struct sensor_decoder_api *decoder;
		rc = sensor_get_decoder(bmp280, &decoder);
		if (rc != 0) {
			printf("[BMP] get decode failed: %d\n", rc);
		}

		uint32_t temp_fit = 0;
		struct sensor_q31_data temp_data = {0};
		decoder->decode(buf,
				(struct sensor_chan_spec) {SENSOR_CHAN_AMBIENT_TEMP, 0},
				&temp_fit, 1, &temp_data);

		printf("[BMP]: Temp: %s%d.%dC\n",
		       PRIq_arg(temp_data.readings[0].temperature, 6, temp_data.shift));
		k_sleep(K_SECONDS(2));
	}
}

int main(void)
{
	k_tid_t dht_thread_tid = k_thread_create(&dht_thread, dht_thread_stack_area,
					 K_THREAD_STACK_SIZEOF(dht_thread_stack_area),
					 dht_read,
					 NULL, NULL, NULL,
					 5, 0, K_SECONDS(2));
	k_tid_t bmp_thread_tid = k_thread_create(&bmp_thread, bmp_thread_stack_area,
					 K_THREAD_STACK_SIZEOF(bmp_thread_stack_area),
					 bmp_read,
					 NULL, NULL, NULL,
					 5, 0, K_SECONDS(2));
	k_thread_join(bmp_thread_tid, K_FOREVER);
	k_thread_join(dht_thread_tid, K_FOREVER);

	return 0;
}
