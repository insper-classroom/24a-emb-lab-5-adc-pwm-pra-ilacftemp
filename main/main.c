/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

QueueHandle_t xQueueAdc;

#define JOY_X_PIN 26
#define JOY_Y_PIN 27
#define DEAD_ZONE 30

typedef struct adc {
    int axis;
    int val;
} adc_t;

void x_task(void *p) {
    adc_t data;
    adc_init();

    while (1) {
        adc_gpio_init(JOY_X_PIN);
        adc_select_input(0);
        data.axis = 0;
        int val = adc_read();
        val -= 2048;
        val /= 8;
        if (abs(val) < DEAD_ZONE) {
            val = 0;
        }
        data.val = val;
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    adc_t data;
    adc_init();

    while (1) {
        adc_gpio_init(JOY_Y_PIN);
        adc_select_input(1);
        data.axis = 1;
        int val = adc_read();
        val -= 2048;
        val /= 8;
        if (abs(val) < DEAD_ZONE) {
            val = 0;
        }
        data.val = val;
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_task(void *p) {
    adc_t data;

    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);
        int msb = data.val >> 8;
        int lsb = data.val & 0xFF;

        uart_putc_raw(uart0, data.axis);
        uart_putc_raw(uart0, lsb);
        uart_putc_raw(uart0, msb);
        uart_putc_raw(uart0, -1);
    }
}

int main() {
    stdio_init_all();
    uart_init(uart0, 115200);

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(x_task, "x_task", 4096, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}