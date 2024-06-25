#ifndef _SEAAS_TEMPERATURE_SYSCALL_H
#define _SEAAS_TEMPERATURE_SYSCALL_H
#include <seaas-temperature.h>
__syscall int init_temperature_sensor_syscall(enum seaas_sensor_type_temperature type);
__syscall int32_t get_temperature(enum seaas_sensor_type_temperature type);
#endif /* _SEAAS_TEMPERATURE_SYSCALL_H */
