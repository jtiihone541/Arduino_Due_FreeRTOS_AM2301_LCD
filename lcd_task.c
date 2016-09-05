/*
 * lcd_task.c
 *
 *  Author: JTi
 */ 

/*
 * LCD connections - ARDUINO
 * EN/6     42/PA19
 * RS/4     44/PC19
 * RW/5     45/PC18
 * DB4/11   47/PC16
 * DB5/12   46/PC17
 * DB6/13   49/PC14
 * DB7/14   48/PC15
 */

#include <asf.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <stdio.h>
#include "lcd_task.h"
xQueueHandle lcd_msg_queue;
lcd_message_t lcd_message;

lcd_parameters_t lcd_1602c;
lcd_command_table_t lcd_commands[13] = { \
    {SCREEN_CLEAR, 0, 0, 0x01, 1640},\
    {CURSOR_RETURN, 0, 0, 0x02, 1640},\
    {INPUT_SET, 0, 0, 0x04, 40},\
    {DISPLAY_SWITCH, 0, 0, 0x08, 40},\
    {SHIFT, 0, 0, 0x10, 40},\
    {FUNCTION_SET, 0, 0, 0x20, 40},\
    {CGRAM_AD_SET, 0, 0, 0x40, 40},\
    {DDRAM_AD_SET, 0, 0, 0x80, 40},\
    {BUSY_AD_READ_CT, 0, 1, 0x00, 40},\
    {CGRAM_DATA_WRITE, 1, 0, 0x00, 40},\
    {DDRAM_DATA_WRITE, 1, 0, 0x00, 40},\
    {CGRAM_DATA_READ, 1, 1, 0x00, 40},\
    {DDRAM_DATA_READ, 1, 1, 0x00, 40}
};

void lcd_init_ioport(void);
void lcd_write_4bit_command(uint8_t rs, uint8_t rw, uint8_t db7, uint8_t db6, uint8_t db5, uint8_t db4);
void lcd_write_command(lcd_command_name_t command, uint8_t parameter);
void lcd_write_character(uint8_t chr);
void lcd_write_string(uint8_t row, uint8_t column, const char *ptr);
void lcd_init_sequence2(void);

void lcd_init_ioport(void)
{
    pmc_enable_periph_clk(ID_PIOC);
    pmc_enable_periph_clk(ID_PIOA);
    pio_set_output(PIOC, PIO_PC19, HIGH, DISABLE, ENABLE);
    pio_set_output(PIOC, PIO_PC18, HIGH, DISABLE, ENABLE);
    pio_set_output(PIOC, PIO_PC16, HIGH, DISABLE, ENABLE);
    pio_set_output(PIOC, PIO_PC17, HIGH, DISABLE, ENABLE);
    pio_set_output(PIOC, PIO_PC14, HIGH, DISABLE, ENABLE);
    pio_set_output(PIOC, PIO_PC15, HIGH, DISABLE, ENABLE);
    pio_set_output(PIOA, PIO_PA19, LOW, DISABLE, ENABLE);
}



void lcd_write_4bit_command(uint8_t rs, uint8_t rw, uint8_t db7, uint8_t db6, uint8_t db5, uint8_t db4)
{
    uint32_t i;
    pio_set_output(PIOA, PIO_PA19, LOW, DISABLE, ENABLE);
    pio_set_output(PIOC, PIO_PC19, rs, DISABLE, ENABLE);
    for(i = 0; i < 20; i++);
    pio_set_output(PIOC, PIO_PC18, rw, DISABLE, ENABLE);
    for(i = 0; i < 20; i++);
    pio_set_output(PIOC, PIO_PC15, db7, DISABLE, ENABLE);
    pio_set_output(PIOC, PIO_PC14, db6, DISABLE, ENABLE);
    pio_set_output(PIOC, PIO_PC17, db5, DISABLE, ENABLE);
    pio_set_output(PIOC, PIO_PC16, db4, DISABLE, ENABLE);
    pio_set_output(PIOA, PIO_PA19, HIGH, DISABLE, ENABLE);
    for(i = 0; i < 100; i++);
    pio_set_output(PIOA, PIO_PA19, LOW, DISABLE, ENABLE);
    for(i = 0; i < 2000; i++);
}

void lcd_write_command(lcd_command_name_t command, uint8_t parameter)
{
    uint8_t cpc;
    uint8_t rs,rw;
    uint32_t i;
    
    cpc = lcd_commands[command].command_binary_code | parameter; /* Command and Parameter Combined... */
    rs = lcd_commands[command].rs;
    rw = lcd_commands[command].rw;
    lcd_write_4bit_command(rs, rw, (cpc >> 7) & 1, (cpc >> 6) & 1, (cpc >> 5) & 1, (cpc >> 4) & 1);
    lcd_write_4bit_command(rs, rw, (cpc >> 3) & 1, (cpc >> 2) & 1, (cpc >> 1) & 1, cpc & 1);
    
    /* Execute delay according to commands' delay value */
    for (i = 0; i < 50 * lcd_commands[command].execution_time_us; i++);
    return;
}
void lcd_write_character(uint8_t chr)
{
    lcd_write_command(DDRAM_DATA_WRITE, chr);
}

void lcd_write_string(uint8_t row, uint8_t column, const char *ptr)
{
    /* "row" and "column" start from zero */
    
    uint8_t lcd_screen_address = 0, row_base = 0;
    uint8_t i;
    
    switch (row)
    {
        case 0:
            row_base = 0;
            break;
            
        case 1:
            row_base = 0x40;
            break;
            
        default:
            row_base = 0;
            break;
    }
    lcd_screen_address = row_base + column;
    lcd_write_command(DDRAM_AD_SET, lcd_screen_address);

    /* Data address set, send string char by char */
    i = 0;
    for (i = 0; i < lcd_1602c.columns; i++)
    {
        if (ptr[i] == 0) break;
        lcd_write_character(ptr[i]);
    }

    return;
}




void lcd_init_sequence2(void)
{
    uint32_t i;
    vTaskDelay(100);
    lcd_write_4bit_command(LOW, LOW, LOW, LOW, HIGH, HIGH);
    vTaskDelay(5);
    lcd_write_4bit_command(LOW, LOW, LOW, LOW, HIGH, HIGH);
    vTaskDelay(5);
    lcd_write_4bit_command(LOW, LOW, LOW, LOW, HIGH, HIGH);
    vTaskDelay(1);
    lcd_write_4bit_command(LOW, LOW, LOW, LOW, HIGH, LOW);
    for(i = 0; i < LCD_WRITE_DELAY;i++);
    lcd_write_command(FUNCTION_SET, FUNCTION_SET_4D | FUNCTION_SET_2R | FUNCTION_SET_5X7);
    lcd_write_command(DISPLAY_SWITCH, DISPLAY_SWITCH_DISPLAY_OFF);
    lcd_write_command(SCREEN_CLEAR, 0);
    lcd_write_command(INPUT_SET, INPUT_SET_INCREMENT_MODE|INPUT_SET_NO_SHIFT);
    lcd_write_command(DISPLAY_SWITCH, DISPLAY_SWITCH_DISPLAY_ON);
    
    return;
}

/*
 * lcd_task takes care of LCD update.
 * LCD is used solely by lcd_task, and all display write requests are handled through
 * message queue interface. This method effectively removes possible race conditions normally handled by mutexes.
 * Reduces also possibility to deadlocks caused by uncontrolled use of mutexes.
 *
 */
void lcd_task(void *lcd_params)
{
    uint32_t msg_status;
    char displayline[16];
    lcd_1602c.columns = 16;
    lcd_1602c.rows = 2;
    lcd_msg_queue = xQueueCreate(2, sizeof(lcd_message_t));
    if (NULL == lcd_msg_queue)
    {
        lcd_msg_queue = NULL;
    }
    lcd_init_ioport();
    lcd_init_sequence2();
    lcd_write_string(0, 0, "Initialising\0");
    while(1)
    {
        /* LCD main loop */
        msg_status = xQueueReceive(lcd_msg_queue, &lcd_message, 100000);
        if (pdTRUE != msg_status)
        {
            /* Most likely reason for failure is timeout. Just ignore it, because it may be normal */
            /* situation that there is no need to update display for a long time */
            msg_status = pdTRUE;
        }
        switch(lcd_message.lcd_message_number)
        {
            case 1:
                /* Measurement data from am2301_task */
                switch (lcd_message.data.data_valid)
                {
                    case MEASUREMENT_STATUS_OK:
                        /* Data is valid, output it as such */
                        snprintf(displayline,16, "Temp: %.1f %c%s ", lcd_message.data.temp_float, 0xdf, "C");
                        lcd_write_string(0, 0, displayline);
                        snprintf(displayline,16, "Hum:  %.1f %s ", lcd_message.data.humidity_float, "%");
                        lcd_write_string(1, 0, displayline);
                        break;
                        
                    case MEASUREMENT_STATUS_TIMEOUT:
                        /* Timeout occurred in measurement. Some error in AM2301 connection?!? */
                        snprintf(displayline, 16, "Timeout!");
                        lcd_write_command(SCREEN_CLEAR, 0);
                        lcd_write_string(0,0, displayline);
                        break;
                        
                    case MEASUREMENT_STATUS_PARITY_ERROR:
                    default:
                        /* Data is not valid, parity error */
                        snprintf(displayline, 16, "Parity error!");
                        lcd_write_command(SCREEN_CLEAR, 0);
                        lcd_write_string(0,0, displayline);
                        break;
                }
                
                msg_status = pdTRUE;
                break;
                
           case 2:
                /* Running counter from dimmer_task, nothing useful - but just for example */
                snprintf(displayline, 3, "%02d", lcd_message.timedata.seconds);
                lcd_write_string(1, 14, displayline);
                break;
                
           default:
                break;
        }
        

    }
    return;
}