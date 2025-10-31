#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// --- Cấu hình GPIO ---
#define LED_PIN     GPIO_NUM_17
#define BUTTON_PIN  GPIO_NUM_0

void student_id_task(void *pvParameters) {
    const char *student_id = "2211257";
    while (1) {
        printf("Student ID: %s\n", student_id);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void button_task(void *pvParameters) {
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);

    int last_state = 1;
    int press_time = 0;
    while (1) {
        int current_state = gpio_get_level(BUTTON_PIN);
        if (last_state == 1 && current_state == 0) {
            printf("ESP32\n");
            press_time = 0;
        } 
        else if (last_state == 0 && current_state == 0) {
            press_time += 50;
            if (press_time >= 1000) {
                printf("ESP32\n");
                press_time = 0;
            }
        } 
        else if (last_state == 0 && current_state == 1) {
            press_time = 0;
        }

        last_state = current_state;
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}


void app_main_unuse(void) {
    printf("=== ESP-IDF FreeRTOS Demo ===\n");

    xTaskCreate(
        student_id_task,
        "student_id_task",
        2048,
        NULL,
        5,
        NULL
    );

    xTaskCreate(
        button_task,
        "button_task",
        2048,
        NULL,
        5,
        NULL
    );
}
