#ifndef __HWT101_H

#define __HWT101_H

#include "stm32f4xx_hal.h"

#define HWT101_RX_BUFFER_SIZE 12 //接收缓冲区大小，预留多一字节

typedef struct
{
    UART_HandleTypeDef *hwt101_uart; //通信串口的指针
    uint8_t rx_buf[HWT101_RX_BUFFER_SIZE];
    float yaw_data; //偏航角数据
    float gyro_data; //角速度数据
    uint8_t have_yaw_data; //数据是否有效
    uint8_t have_gyro_data;
}hwt101_csx;

void hwt101_Init(hwt101_csx *csx,UART_HandleTypeDef *uart);
void hwt101_Feed(hwt101_csx *csx,uint16_t Size);
uint8_t hwt101_read_gyro(hwt101_csx *csx,float *data);
uint8_t hwt101_read_yaw(hwt101_csx *csx,float *data);
void hwt101_set_yaw(hwt101_csx *csx,float yaw);
void hwt101_reset_yaw(hwt101_csx *csx);
void hwt101_led(hwt101_csx *csx,uint8_t led_state);
#endif