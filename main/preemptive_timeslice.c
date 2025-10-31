#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct task_parmam_t{
    char *nameTask;
    int period;
} task_param_t;

void task(void *pvParameters){
    task_param_t *param = (task_param_t *) pvParameters;
    while (1){
        printf("[%lu ms] %s is running\n", xTaskGetTickCount() * portTICK_PERIOD_MS, (*param).nameTask);
        vTaskDelay(pdMS_TO_TICKS((*param).period));
    }
}

void app_main_unuse(void) {
    static task_param_t taskFast = { "Task 1", 200 };
    static task_param_t taskSlow = { "Task 2", 800 };
    xTaskCreate(task, "Task 1", 2048, &taskFast, 2, NULL);
    xTaskCreate(task, "Task 2", 2048, &taskSlow, 2, NULL);
}
