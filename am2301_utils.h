/*
 * am2301_utils.h
 *
 *  Author: JTi
 */ 


#ifndef AM2301_UTILS_H_
#define AM2301_UTILS_H_

#include <asf.h>

#define AM2031_MAX_SAMPLES 42

#define PIO_PORT PIO_PA2

/* Measurement message status in "data_valid" field */
#define MEASUREMENT_STATUS_OK 1
#define MEASUREMENT_STATUS_PARITY_ERROR 2
#define MEASUREMENT_STATUS_TIMEOUT 3

/* AM2301 databit format information */
#define AM2301_NUMBER_OF_TEMPERATURE_BITS 16
#define AM2301_NUMBER_OF_HUMIDITY_BITS 16
#define AM2301_NUMBER_OF_PARITY_BITS 8

typedef struct
{
    uint8_t bit_counter;
    uint8_t interrupt_type;
    uint32_t zero_bit_limit;
    uint32_t bit_time_table[AM2031_MAX_SAMPLES];
    uint32_t abs_time_table[AM2031_MAX_SAMPLES];
    uint32_t databytes[AM2031_MAX_SAMPLES];
    
} am2301_interrupt_data_t;

typedef struct
{
    uint32_t    humidity_int;
    uint32_t    temp_int;
    float       humidity_float;
    float       temp_float;
    uint8_t     data_valid;
} am2301_processed_data_t;


typedef struct
{
    uint32_t    bit_number;
    uint32_t    bit_time;    
} am2301_message_t;

void initialise_am2301(am2301_interrupt_data_t *am2301);
void stop_measurement(void);
void start_measurement(am2301_interrupt_data_t *am2301);
void handle_am2031_bit(am2301_interrupt_data_t *am2301, uint32_t bit_time);
void convent_am2301_data(am2301_processed_data_t *data, am2301_interrupt_data_t *raw_data);

#endif /* AM2301_UTILS_H_ */