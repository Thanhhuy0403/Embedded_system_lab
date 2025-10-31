#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

TimerHandle_t xTimer1, xTimer2;

int count1 = 0;
int count2 = 0;

void vTimerCallback(TimerHandle_t xTimer) {
    int timer_id = (int) pvTimerGetTimerID(xTimer);

    if (timer_id == 1) {
        printf("ahihi\n");
        count1++;
        if (count1 >= 10) {
            printf("Timer 1 stop after 10 prints\n");
            xTimerStop(xTimer, 0);
        }
    }
    else if (timer_id == 2) {
        printf("ihaha\n");
        count2++;
        if (count2 >= 5) {
            printf("Timer 2 stop after 5 prints\n");
            xTimerStop(xTimer, 0);
        }
    }
}

void app_main(void){
    xTimer1 = xTimerCreate(
        "Timer1",
        pdMS_TO_TICKS(2000),
        pdTRUE,
        (void*)1,
        vTimerCallback
    );

    xTimer2 = xTimerCreate(
        "Timer2",
        pdMS_TO_TICKS(3000),
        pdTRUE,
        (void*)2,
        vTimerCallback
    );

    if (xTimer1 != NULL && xTimer2 != NULL){
        xTimerStart(xTimer1, 0);
        xTimerStart(xTimer2, 0);
    }else{
        printf("Failed to create timers!\n");
    }
}
