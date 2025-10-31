#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef struct {
    int targetTaskID;  
    int payload;       
} Request;

#define TASK_A_ID 1
#define TASK_B_ID 2

QueueHandle_t requestQueue;
void reception_task(void *pv){
    Request req;
    int counter = 0;
    while (1) {
        counter++;
        if (counter % 5 == 0) {
            req.targetTaskID = 99;
        } else if (counter % 2 == 0) {
            req.targetTaskID = TASK_A_ID;
        } else {
            req.targetTaskID = TASK_B_ID;
        }

        req.payload = counter;

        printf("[Reception] Send request: target=%d, data=%d\n", req.targetTaskID, req.payload);
        xQueueSend(requestQueue, &req, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_A(void *pv){
    Request recv;
    while (1) {
        if (xQueueReceive(requestQueue, &recv, portMAX_DELAY) == pdTRUE) {
            if (recv.targetTaskID == TASK_A_ID) {
                printf("Task A handling request, data=%d\n", recv.payload);
            } else {
                xQueueSendToFront(requestQueue, &recv, 0);
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
    }
}

void task_B(void *pv){
    Request recv;
    while (1) {
        if (xQueueReceive(requestQueue, &recv, portMAX_DELAY) == pdTRUE) {
            if (recv.targetTaskID == TASK_B_ID) {
                printf("Task B handling request, data=%d\n", recv.payload);
            } else {
                xQueueSendToFront(requestQueue, &recv, 0);
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
    }
}

void error_task(void *pv){
    Request recv;
    while (1) {
        if (xQueueReceive(requestQueue, &recv, portMAX_DELAY) == pdTRUE) {
            printf("!! ERROR: No task accepts request for target=%d, data=%d → ignored\n", recv.targetTaskID, recv.payload);
        }
    }
}

void app_main_unuse(void){
    requestQueue = xQueueCreate(5, sizeof(Request));
    xTaskCreate(reception_task, "reception_task", 4096, NULL, 3, NULL);
    xTaskCreate(task_A, "task_A", 4096, NULL, 2, NULL);
    xTaskCreate(task_B, "task_B", 4096, NULL, 2, NULL);
    xTaskCreate(error_task, "error_task", 4096, NULL, 1, NULL);
}
