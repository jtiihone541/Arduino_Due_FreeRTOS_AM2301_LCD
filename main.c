/*
 *
 */
#include <asf.h>

#include "lcd_task.h"
#include "am2301_task.h"
#include "dimmer_task.h"

void vApplicationMallocFailedHook()
{
    return;
}

int main (void)
{
    /* Arduino DUE library code for initialisation, i.e. clock system */
    sysclk_init();
    board_init();

    /* Just create application tasks and start scheduler... */
    xTaskCreate(am2301_task, "MEAS", 100, NULL, 0, NULL);
    xTaskCreate(lcd_task, "LCD", 300, NULL, 0, NULL);
    xTaskCreate(dimmer_task, "DIM", 100, NULL, 0, NULL);
    vTaskStartScheduler();
}
