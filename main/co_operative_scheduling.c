#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task_high(void *pvParameters){
    while (1) {
        printf("HIGH priority task running\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task_low(void *pvParameters){
    while (1) {
        printf("LOW priority task running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main_unuse(void){
    xTaskCreate(task_high, "task_high", 2048, NULL, 2, NULL);
    xTaskCreate(task_low, "task_low", 2048, NULL, 1, NULL);
}
