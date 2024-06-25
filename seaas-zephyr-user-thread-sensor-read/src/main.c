/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <seaas-temperature.h>
#include <zephyr/syscalls/seaas-temperature-syscalls.h>

K_THREAD_STACK_DEFINE(dht_thread_stack_area, 1024);
struct k_thread dht_thread;
K_THREAD_STACK_DEFINE(bmp_thread_stack_area, 2048);
struct k_thread bmp_thread;
void seaas_sensor_read(void *_sensor_type, void *p2, void *p3)
{
	char *sensor_type = _sensor_type;
	int sfd = init_temperature_sensor(sensor_type);
	if (sfd < 0) {
		printf("[%s] Init failed\n", sensor_type);
		return;
	}

	while (true) {
		double temperature = (double)get_temperature(sfd) * .00001;
		printf("[%s]: Temp: %.5fC\n", sensor_type, temperature);
		k_sleep(K_MSEC(2000));
	}
}


int main(void)
{
	const struct device *const dht11 = DEVICE_DT_GET_ONE(aosong_dht);
	const struct device * bmp280 = DEVICE_DT_GET_ONE(bosch_bme280);

	if (sizeof(seaas_temperature_sensors) / sizeof(*seaas_temperature_sensors) < SEAAS_MAX_TEMPERATURE_SENSORS) {
		printf("Reserved space for sensors is too small\n");
		return -1;
	}

	seaas_temperature_sensors[SEAAS_SENSOR_TYPE_TEMPERATURE_DHT11] = dht11;
	seaas_temperature_sensors[SEAAS_SENSOR_TYPE_TEMPERATURE_BMP280] = bmp280;

	k_tid_t dht_thread_tid = k_thread_create(&dht_thread, dht_thread_stack_area,
					 K_THREAD_STACK_SIZEOF(dht_thread_stack_area),
					 seaas_sensor_read,
					 "dht11", NULL, NULL,
					 5, K_USER, K_SECONDS(2));
	k_tid_t bmp_thread_tid = k_thread_create(&bmp_thread, bmp_thread_stack_area,
					 K_THREAD_STACK_SIZEOF(bmp_thread_stack_area),
					 seaas_sensor_read,
					 "bmp280", NULL, NULL,
					 5, K_USER, K_NO_WAIT);
	k_thread_join(dht_thread_tid, K_FOREVER);
	k_thread_join(bmp_thread_tid, K_FOREVER);

	return 0;
}
