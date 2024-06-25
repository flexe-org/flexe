#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/dsp/print_format.h>
#include <stdio.h>
#include <seaas-temperature-syscalls.h>

/* Dynamic allocation? */
struct device *seaas_temperature_sensors[SEAAS_MAX_TEMPERATURE_SENSORS];

/* Structs for the bme280 driver */
SENSOR_DT_READ_IODEV(iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(bosch_bme280),
		{SENSOR_CHAN_AMBIENT_TEMP, 0},
		{SENSOR_CHAN_HUMIDITY, 0},
		{SENSOR_CHAN_PRESS, 0});
RTIO_DEFINE(ctx, 1, 1);

/* Initialize systemcall */
int z_impl_init_temperature_sensor_syscall(enum seaas_sensor_type_temperature type)
{
	struct device *sensor = NULL;
	if (type == SEAAS_SENSOR_TYPE_TEMPERATURE_ANY)
		type = SEAAS_SENSOR_TYPE_TEMPERATURE_BMP280;
	switch (type) {
		case SEAAS_SENSOR_TYPE_TEMPERATURE_BMP280:
			sensor = seaas_temperature_sensors[SEAAS_SENSOR_TYPE_TEMPERATURE_BMP280];
			break;
		case SEAAS_SENSOR_TYPE_TEMPERATURE_DHT11:
			sensor = seaas_temperature_sensors[SEAAS_SENSOR_TYPE_TEMPERATURE_DHT11];
			break;
		default:
			printf("Unknown type for temperature sensor");
			return -19;
	}
	if (!device_is_ready(sensor)) {
		printf("Device %s is not ready\n", sensor->name);
		return -1;
	}
	return type;
}

int z_vrfy_init_temperature_sensor_syscall(enum seaas_sensor_type_temperature type)
{
	return z_impl_init_temperature_sensor_syscall(type);
}
#include <syscalls/init_temperature_sensor_syscall_mrsh.c>

/* Wrapper */
int init_temperature_sensor(const char *sensor_type)
{

	enum seaas_sensor_type_temperature stype;
	if (sensor_type == NULL) {
		/* XXX Select the best, we need to select it using specific logic */
		stype = SEAAS_SENSOR_TYPE_TEMPERATURE_BMP280;
	} else if (!strcmp(sensor_type, "dht11")) {
		stype = SEAAS_SENSOR_TYPE_TEMPERATURE_DHT11;
	} else if (!strcmp(sensor_type, "bmp280")) {
		stype = SEAAS_SENSOR_TYPE_TEMPERATURE_BMP280;
	} else {
		/* Unknown sensor */
		return -19;
	}

	return init_temperature_sensor_syscall(stype);
}

double get_temperature_dht11(void)
{
	struct device *dht11 = seaas_temperature_sensors[SEAAS_SENSOR_TYPE_TEMPERATURE_DHT11];
	struct sensor_value temperature = {0};
	int rc = sensor_sample_fetch(dht11);
	if (rc != 0) {
		printf("[dht11] Sensor fetch failed: %d\n", rc);
		return -1;
	}

	rc = sensor_channel_get(dht11, SENSOR_CHAN_AMBIENT_TEMP,
				&temperature);
	if (rc != 0) {
		printf("get failed: %d\n", rc);
		return -1;
	}
	return sensor_value_to_double(&temperature);
}

double get_temperature_bmp280(void)
{

	uint8_t buf[128] = {0};
	const struct sensor_decoder_api *decoder;
	uint32_t temp_fit = 0;
	struct sensor_q31_data temp_data = {0};

	int rc = sensor_read(&iodev, &ctx, buf, 128);
	if (rc != 0) {
		printf("[BMP280] Sensor read failed: %d\n", rc);
		return -1;
	}

	rc = sensor_get_decoder(seaas_temperature_sensors[SEAAS_SENSOR_TYPE_TEMPERATURE_BMP280], &decoder);
	if (rc != 0) {
		printf("[BMP280] Sensor decode failed: %d\n", rc);
		return -1;
	}

	decoder->decode(buf,
			(struct sensor_chan_spec) {SENSOR_CHAN_AMBIENT_TEMP, 0},
			&temp_fit, 1, &temp_data);
	uint32_t integer = __PRIq_arg_get_int(temp_data.readings[0].temperature, temp_data.shift);
	uint32_t frac = __PRIq_arg_get_frac(temp_data.readings[0].temperature, 6, temp_data.shift);

	return (double) integer + (double)0.000001 * (double)frac;
}


int32_t z_impl_get_temperature(enum seaas_sensor_type_temperature type)
{
	double out = .0;
        switch (type) {
                case SEAAS_SENSOR_TYPE_TEMPERATURE_DHT11:
                        out = get_temperature_dht11();
			break;
                case SEAAS_SENSOR_TYPE_TEMPERATURE_BMP280:
                        out = get_temperature_bmp280();
			break;
                default:
                        printf("Unknown sensor type\n");
                        return (0x1 << 31);
        }
	/* Map to int32 to use standard registers for syscalls */
	return (int32_t) (out * 100000.0);
}

int32_t z_vrfy_get_temperature(enum seaas_sensor_type_temperature type)
{

	return z_impl_get_temperature(type);
}
#include <syscalls/get_temperature_mrsh.c>
