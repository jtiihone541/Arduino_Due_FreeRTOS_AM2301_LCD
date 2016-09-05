/*
 * lcd_task.h
 *
 *  Author: JTi
 */ 


#ifndef LCD_TASK_H_
#define LCD_TASK_H_

#include "am2301_utils.h"

/* LCD commands */
typedef enum
{
    SCREEN_CLEAR = 0,
    CURSOR_RETURN,
    INPUT_SET,
    DISPLAY_SWITCH,
    SHIFT,
    FUNCTION_SET,
    CGRAM_AD_SET,
    DDRAM_AD_SET,
    BUSY_AD_READ_CT,
    DDRAM_DATA_WRITE,
    CGRAM_DATA_WRITE,
    DDRAM_DATA_READ,
    CGRAM_DATA_READ
} lcd_command_name_t;

/* LCD parameter names, prefixed by commands name - directly mapped to correct bit locations */
#define INPUT_SET_INCREMENT_MODE 0x02
#define INPUT_SET_DECREMENT_MODE 0x00
#define INPUT_SET_SHIFT 0x01
#define INPUT_SET_NO_SHIFT 0x00
#define DISPLAY_SWITCH_DISPLAY_ON 0x04
#define DISPLAY_SWITCH_DISPLAY_OFF 0x00
#define DISPLAY_SWITCH_CURSOR_ON 0x02
#define DISPLAY_SWITCH_CURSOR_OFF 0x00
#define DISPLAY_SWITCH_BLINK_ON 0x01
#define DISPLAY_SWITCH_BLINK_OFF 0x00
#define SHIFT_DISPLAY_SHIFT 0x08
#define SHIFT_CURSOR_SHIFT 0x00
#define SHIFT_RIGHT_SHIFT 0x04
#define SHIFT_LEFT_SHIFT 0x00
#define FUNCTION_SET_8D 0x10
#define FUNCTION_SET_4D 0x00
#define FUNCTION_SET_2R 0x08
#define FUNCTION_SET_1R 0x00
#define FUNCTION_SET_5X10 0x04
#define FUNCTION_SET_5X7 0x00

typedef struct  
{
    lcd_command_name_t command_name;
    uint8_t rs;
    uint8_t rw;
    uint8_t command_binary_code;
    uint32_t execution_time_us;
} lcd_command_table_t;


    

typedef struct  
{
    uint8_t dummy;
} lcd_config_t;

typedef struct  
{
    uint8_t seconds;
} seconds_t;

#define LCD_MESSAGE_MEASUREMENT_REPORT 1
#define LCD_MESSAGE_COUNTER_REPORT 2
typedef struct  
{
    uint8_t lcd_message_number;
    union
    {
        am2301_processed_data_t data;
        lcd_config_t configdata;
        seconds_t timedata;
    };
} lcd_message_t;

typedef struct
{
    uint8_t rows;
    uint8_t columns;
} lcd_parameters_t;

void lcd_task(void *lcd_params);
#define LCD_WRITE_DELAY 4000
#define LCD_WRITE_DELAY_SHORT 20
#endif /* LCD_TASK_H_ */