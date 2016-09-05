/*
 * dimmer_task.c
 *
 *  Author: JTI
 */ 

#include "lcd_task.h"
#include "dimmer_task.h"

lcd_message_t msg_1;
extern xQueueHandle lcd_msg_queue;

/* Task name is a bit misleading :) */
/* Actually, this sends periodically "seconds" to lcd_task, which will */
/* display them at LCD. Nothing useful, but good example */
void dimmer_task(void *params)
{
    uint8_t second_counter = 0;
    uint32_t msg_status;
    while (1)
    {
        vTaskDelay(1000);
        msg_1.lcd_message_number = LCD_MESSAGE_COUNTER_REPORT;
        msg_1.timedata.seconds = second_counter;
        msg_status = xQueueSend(lcd_msg_queue, &msg_1, 10);
        if (msg_status != pdTRUE)
        {
            /* Message sending error */
        }
        second_counter++;
        if (60 == second_counter)
        {
            second_counter = 0;
        }
    }
}