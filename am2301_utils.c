/*
 * am2301_utils.c
 *
 *  Author: JTi
 */ 

#include <asf.h>
#include <FreeRTOS.h>
#include <string.h>
#include "am2301_utils.h"
#include <math.h>

extern volatile uint32_t current_systick;
extern xSemaphoreHandle semaphore;
extern xQueueHandle am2301_messagequeue;

void measure_data_handler(const uint32_t id, const uint32_t index);

/*
 * Convert data from bit-based data into 16/8-bit units.
 * It is more practicable store data as bits in ISR - it is easy to attach bit times into actual data.
 * Final conversion can be done "offline" after the AM2301 has sent the data.
 *
 */
void convent_am2301_data(am2301_processed_data_t *data, am2301_interrupt_data_t *raw_data)
{
    uint32_t    conversion, i, calculated_parity;
    
    memset((void *)data, sizeof(am2301_processed_data_t), 0);
    data->data_valid = 0; /* By default, it is false */
    
    conversion = 0;
    for(i = 0; i < AM2301_NUMBER_OF_HUMIDITY_BITS; i++)
    {
        /* Convert humidity. Bits are from MSB->LSB order */
        conversion <<= 1;
        conversion |= raw_data->databytes[i] & 1;
    }
    data->humidity_int = conversion;
    
    conversion = 0;
    for(i = 0; i < AM2301_NUMBER_OF_TEMPERATURE_BITS; i++)
    {
        /* Convert temperature. Bits are from MSB->LSB order */
        conversion <<= 1;
        conversion |= raw_data->databytes[i+16] & 1;
    }
    data->temp_int = conversion;
    
    /* Check parity */
    
    conversion = 0;
    for(i = 0; i < AM2301_NUMBER_OF_PARITY_BITS; i++)
    {
        conversion <<= 1;
        conversion |= raw_data->databytes[i+32] & 1;
    }
    calculated_parity = (data->humidity_int & 0xff) + ((data->humidity_int >> 8) & 0xff) + (data->temp_int & 0xff) + ((data->temp_int >> 8) & 0xff);
    calculated_parity &= 0xff;

    data->humidity_float = (float)data->humidity_int/10.0;
    
    /* Temperature may be negative, then the MSB is set */
    if ((data->temp_int & 0x8000) == 0x8000)
    {
        /* Below zero... */
        data->temp_int &= 0x7fff;
        data->temp_float =(float)data->temp_int/-10.0;
    }
    else
    {
        data->temp_float = (float)data->temp_int/10.0;        
    }

    if (calculated_parity == conversion)
    {
        /* Parity correct! */
        data->data_valid = MEASUREMENT_STATUS_OK;
    }
    else
    {
        data->data_valid = MEASUREMENT_STATUS_PARITY_ERROR;
    }

    return;
}

/*
 * This is the callback function from PIOA port interrupt handler
 * This is called when a pin change has been detected from configured PIOA pin
 *
 */
void measure_data_handler(const uint32_t id, const uint32_t index)
{
    uint32_t timestamp = 0;
    am2301_message_t msg;
    
    /* Check that callback is received from correct PIO and pin */
    if ((id == ID_PIOA) && (index == PIO_PORT))
    {
        /* Verify, that this is really a falling edge -> databit from pin is zero */
        if ((PIOA->PIO_PDSR & PIO_PORT) == 0)
        {
            /* SysTick counter is a good and accurate time reference for calculating times */
            /* However, it is not very portable because its frequency depends on MCK frequency and possible prescaler */
            
            timestamp = SysTick->VAL;
            current_systick = timestamp;
            msg.bit_time = timestamp;
            xQueueSendFromISR(am2301_messagequeue, &msg, NULL);
        }
    }
    return;
}


/*
 * This function will be called when AM2301 needs initialisation. This occurs either
 * - in very beginning after reset
 * - if an error has been detected when reading data from AM2301, e.g. timeout
 */
void initialise_am2301(am2301_interrupt_data_t *am2301)
{
    memset((void *)am2301, 0, sizeof(am2301_interrupt_data_t));
    am2301->zero_bit_limit = 8000;
    pmc_enable_periph_clk(ID_PIOA);
    pio_set_output(PIOA, PIO_PORT, HIGH, DISABLE, ENABLE);
    vTaskDelay(100);
    /* Do a dummy read sequence first to awake AM2301 */
    pio_set_output(PIOA, PIO_PORT, LOW, DISABLE, ENABLE);
    vTaskDelay(5);  /* Keep port pin in low state for 5 ticks */
    pio_set_input(PIOA, PIO_PORT, PIO_PULLUP|PIO_DEGLITCH);
    vTaskDelay(2000);
    pio_set_output(PIOA, PIO_PORT, HIGH, DISABLE, ENABLE);
    vTaskDelay(100);
    pio_handler_set(PIOA, ID_PIOA, PIO_PORT, PIO_IT_FALL_EDGE, measure_data_handler);
    
    return;
}
/*
 * This function is called when measurement is finished.
 * Disable interrupts from AM2301 direction, and put pin HIGH and OUTPUT.
 */
void stop_measurement(void)
{
    NVIC_DisableIRQ(PIOA_IRQn);
    pio_disable_interrupt(PIOA, PIO_PORT);
    pio_set_output(PIOA, PIO_PORT, HIGH, DISABLE, ENABLE);
    return;
}

/*
 * This function is called when a new measurement is being started
 * It is assumed that I/O line for AM2301 is held HIGH and is configured as OUTPUT
 * Prior to calling this.
 */
void start_measurement(am2301_interrupt_data_t *am2301)
{
    uint32_t i;
    memset((void *)am2301, 0, sizeof(am2301_interrupt_data_t));
    am2301->zero_bit_limit = 8000;
    
    /* Set pin into output mode and drive it LOW for 5ms -> start measurement cmd for AM2301 */
    pio_set_output(PIOA, PIO_PORT, LOW, DISABLE, ENABLE);
    vTaskDelay(5);
    
    /* Now change it to input mode, to get response from AM2301 */
    pio_set_input(PIOA, PIO_PORT, PIO_PULLUP|PIO_DEGLITCH);
    pio_enable_interrupt(PIOA, PIO_PORT);
    for(i = 0; i < 1000; i++); /* Some "stabilisation time" to prevent unexpected pin interrupts */
    NVIC_ClearPendingIRQ(PIOA_IRQn);
    i = PIOA->PIO_PSR;
    NVIC_EnableIRQ(PIOA_IRQn);
    return;
}

/*
 * Calculate times for "falling edge to falling edge", i.e. AM2301 data bit length
 * "1" pulses have some 50% longer duration than "0" pulses (up to 85us)
 * So use value of 8000 SysTick tics (1/84MHz) == 95us as a decision point for bit value.
 * Refer to AM2301 datasheet for details.
 *
 */
void handle_am2031_bit(am2301_interrupt_data_t *am2301, uint32_t bit_time)
{
    uint8_t bit_value = 0;
    static uint32_t previous_bit_time;
    uint32_t actual_bit_time;

    am2301->bit_counter++;
    if (am2301->bit_counter < 3)
    {
        /* First 2 bits "falling edges" are the "start of acknowledge and start of first databit -> ignore those */
        previous_bit_time = bit_time;
        return;
    }
    /* bits 2-42 are the actual data bits */
    if (previous_bit_time < bit_time)
    {
        /* SysTick has wrapped */
        actual_bit_time = (SysTick->LOAD - bit_time) + previous_bit_time;
    }
    else
    {
        actual_bit_time = previous_bit_time-bit_time;
    }
    previous_bit_time=bit_time;
    if (actual_bit_time > am2301->zero_bit_limit)
    {
        /* bit time is longer than shorter zero bit time -> it must be binary '1' */
        bit_value = 1;
    }
    else
    {
        bit_value = 0;
    }
    /* Store the bit */
    am2301->bit_time_table[am2301->bit_counter - 3] = actual_bit_time;
    am2301->databytes[am2301->bit_counter - 3] = bit_value;
    am2301->abs_time_table[am2301->bit_counter - 3] = bit_time;
    if (am2301->bit_counter == AM2031_MAX_SAMPLES)
    {
        stop_measurement();
    }
    return;
}