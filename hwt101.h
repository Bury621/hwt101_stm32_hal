#ifndef __HWT101_H

#define __HWT101_H

#include "stm32f1xx_hal.h"

#define HWT101_RX_BUFFER_SIZE 22 //HWT101连续发送0x52和0x53两个11字节帧

typedef struct
{
    UART_HandleTypeDef *hwt101_uart; //通信串口的指针
    uint8_t rx_buf[HWT101_RX_BUFFER_SIZE];
    volatile float yaw_data; //偏航角数据
    volatile float gyro_data; //角速度数据
    volatile uint8_t have_yaw_data; //数据是否有效
    volatile uint8_t have_gyro_data;
}hwt101_HandleTypeDef;

void hwt101_Init(hwt101_HandleTypeDef *csx,UART_HandleTypeDef *uart);
void hwt101_Feed(hwt101_HandleTypeDef *csx,uint16_t Size);
uint8_t hwt101_read_gyro(hwt101_HandleTypeDef *csx,float *data);
uint8_t hwt101_read_yaw(hwt101_HandleTypeDef *csx,float *data);
void hwt101_set_yaw(hwt101_HandleTypeDef *csx,float yaw);
void hwt101_reset_yaw(hwt101_HandleTypeDef *csx);
void hwt101_led(hwt101_HandleTypeDef *csx,uint8_t led_state);
#endif
