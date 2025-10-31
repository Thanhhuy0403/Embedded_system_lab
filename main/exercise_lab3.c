#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

void vTaskFunction(void *pvParameters) {
    char *pcTaskName;
    const TickType_t xDelay250ms = pdMS_TO_TICKS(250);

    pcTaskName = (char *) pvParameters;

    for (;;) {
        printf("%s", pcTaskName);
        vTaskDelay(xDelay250ms*4);
    }
}

static const char *pcTextForTask1 = "Task 1 is running\r\n";
static const char *pcTextForTask2 = "Task 2 is running\r\n";

volatile uint32_t ulIdleCycleCount = 0UL;

void app_main_unuse(void) {
    xTaskCreate(
        vTaskFunction,
        "Task 1",
        2048,
        (void *)pcTextForTask1,
        1,
        NULL
    );

    xTaskCreate(
        vTaskFunction,
        "Task 2",
        2048,
        (void *)pcTextForTask2,
        2,
        NULL
    );
}
