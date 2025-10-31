#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "stdio.h"

void printTask(void *pvParameters) {
    // Reset và cấu hình GPIO17 làm output
    gpio_reset_pin(GPIO_NUM_17);
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);

    while (1) {
        printf("Off\n");
        gpio_set_level(GPIO_NUM_17, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        printf("On\n");
        gpio_set_level(GPIO_NUM_17, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


void app_main_unuse(void) {
    // Tạo task nhấp nháy LED
    xTaskCreate(
        printTask,      // Hàm thực thi task
        "PrintTask",    // Tên task (chỉ để debug)
        2048,           // Stack size (đơn vị word)
        NULL,           // Không truyền tham số vào task
        5,              // Độ ưu tiên của task
        NULL            // Không cần lấy handle task
    );
}
