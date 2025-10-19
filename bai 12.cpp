#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stm32f10x.h"
#include <stdint.h>

#define LED1_PIN        GPIO_Pin_0
// ep kieu tan so va xung vao 1 goi tin se duoc gui vao hang doi
typedef struct {
    uint32_t frequency; 
    uint32_t dutyCycle; 
} LedConfig_t;
// khai bao hang doi 
QueueHandle_t xLedConfigQueue;

void GPIO_Config(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.GPIO_Pin = LED1_PIN;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init_struct);

    GPIO_ResetBits(GPIOA, LED1_PIN); 
}

// task dieu khien led 
void LedControllerTask(void *pvParameters) {
    LedConfig_t currentConfig;
	// cau hinh khoi dong
    currentConfig.frequency = 1;
    currentConfig.dutyCycle = 50;

    uint32_t period_ms, on_time_ms, off_time_ms;

    for (;;) {
		// lay du lieu tu hang doi ten-diachi-thoigiancho
        xQueueReceive(xLedConfigQueue, &currentConfig, (TickType_t)0);
		
        if (currentConfig.frequency > 0 && currentConfig.dutyCycle <= 100) {
            period_ms = 1000 / currentConfig.frequency; 
            on_time_ms = (period_ms * currentConfig.dutyCycle) / 100;
            off_time_ms = period_ms - on_time_ms;

            if (on_time_ms > 0) {
                GPIO_SetBits(GPIOA, LED1_PIN);
                vTaskDelay(pdMS_TO_TICKS(on_time_ms));
            }

            if (off_time_ms > 0) {
                GPIO_ResetBits(GPIOA, LED1_PIN);
                vTaskDelay(pdMS_TO_TICKS(off_time_ms));
            }
            if(on_time_ms == 0 && period_ms > 0){
                 GPIO_ResetBits(GPIOA, LED1_PIN);
                 vTaskDelay(pdMS_TO_TICKS(period_ms));
            }

        } else {
            GPIO_ResetBits(GPIOA, LED1_PIN);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// task gui tin
void ConfigGeneratorTask(void *pvParameters) {
	LedConfig_t configToSend;
	//mang gia tri se gui di
    const LedConfig_t configs[] = {
        {1, 25},   //1HZ 
        {5, 10},   // 5Hz 
        {10, 10},  // 10HZ
    };
    int configIndex = 0;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // gui cau hinh vao hang doi moi 5s

        configToSend = configs[configIndex];
		// gui du lieu : hang doi dich - du lieu gui - thoi gian cho 
        xQueueSend(xLedConfigQueue, &configToSend, (TickType_t)0);

        configIndex++;
        if (configIndex >= 3) { 
            configIndex = 0; // quay lai tu dau
        }
    }
}

int main(void) {
    GPIO_Config();
	//tao hang doi chua 3 goi tin cung luc
    xLedConfigQueue = xQueueCreate(3, sizeof(LedConfig_t));

    if (xLedConfigQueue != NULL) {
        xTaskCreate(LedControllerTask, "LED_Ctrl", 256, NULL, 2, NULL);
        xTaskCreate(ConfigGeneratorTask, "Gen_Config", 256, NULL, 1, NULL);

        vTaskStartScheduler();
    }

    while (1) {}
}
