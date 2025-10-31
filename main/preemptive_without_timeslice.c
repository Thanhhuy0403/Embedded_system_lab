#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task1(void *pv){
    while (1){
        printf("Task 1 running\n");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task2(void *pv){
    while (1){
        printf("Task 2 running\n");
    }
}

void app_main_unuse(void){
    xTaskCreate(task1, "Task1", 2048, NULL, 2, NULL);
    xTaskCreate(task2, "Task2", 2048, NULL, 2, NULL);
}
