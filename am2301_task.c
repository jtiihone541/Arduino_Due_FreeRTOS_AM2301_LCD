/*
 * am2301_task.c
 *
 *  Author: JTi
 */ 
#include <asf.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <string.h>
#include "am2301_task.h"
#include "am2301_utils.h"
#include "lcd_task.h"

volatile uint32_t current_systick;
xQueueHandle am2301_messagequeue;
am2301_interrupt_data_t am2301_data;
am2301_processed_data_t data;
am2301_message_t am2301_message;
extern xQueueHandle lcd_msg_queue;
lcd_message_t measurement_results;

void am2301_task(void *parameters)
{
    uint32_t i;
    uint32_t msg_status;
    uint8_t timeout_counter = 0;
    am2301_messagequeue = xQueueCreate(2, sizeof(am2301_message_t));
    initialise_am2301(&am2301_data);
    while (1)
    {
        vTaskDelay(10000);
        start_measurement(&am2301_data);
        timeout_counter = 0;
        for (i = 0; i < 42; i++)
        {
            /* Bit loop for data sent by AM2301 */
            /* Port A ISR gives semaphore and have also set the "current_systick" from SysTick timer */
            //xSemaphoreTake(semaphore,10);
            msg_status = xQueueReceive(am2301_messagequeue, &am2301_message, 100);
            if (pdFALSE == msg_status)
            {
                /* If timeout in message receive occurs, then AM2301 has not responded in time */
                /* Most likely a connection problem towards it */
                
                timeout_counter++;
                msg_status = pdTRUE;
            }
            current_systick = am2301_message.bit_time;
            
            handle_am2031_bit(&am2301_data, current_systick);
        }
        stop_measurement();
        memset(&measurement_results, 0, sizeof(lcd_message_t));
        measurement_results.lcd_message_number = LCD_MESSAGE_MEASUREMENT_REPORT;
        
        /* Measurement is now complete */
        if (timeout_counter == 0)
        {
            convent_am2301_data(&data, &am2301_data);
            memcpy(&measurement_results.data, &data, sizeof(am2301_processed_data_t));
        }
        else
        {
            /* Timeout occured, report timeout situation in data_valid field */
            measurement_results.data.data_valid = MEASUREMENT_STATUS_TIMEOUT;
        }
        
        msg_status = xQueueSend(lcd_msg_queue, &measurement_results, 10);
        if (msg_status != pdPASS)
        {
            /* Message sending failed */
            msg_status = 0;
        }
        
    } /* endwhile */        
    return;
}