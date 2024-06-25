#ifndef _SEAAS_TEMPERATURE_H
#define _SEAAS_TEMPERATURE_H

enum seaas_sensor_type_temperature {
        SEAAS_SENSOR_TYPE_TEMPERATURE_DHT11,
        SEAAS_SENSOR_TYPE_TEMPERATURE_BMP280,
        SEAAS_MAX_TEMPERATURE_SENSORS,
        SEAAS_SENSOR_TYPE_TEMPERATURE_ANY
};

int init_temperature_sensor(const char *sensor_type);
double get_temperature_dht11(void);
double get_temperature_bmp280(void);

extern struct device *seaas_temperature_sensors[SEAAS_MAX_TEMPERATURE_SENSORS];

#endif /* _SEAAS_TEMPERATURE_H */

