#include "hwt101.h"

#define HWT101_DATA_HEAD 0x55
#define HWT101_GYRO_DATA_TYPE 0x52
#define HWT101_YAW_DATA_TYPE 0x53

#define HWT101_DELAY_BETWEEN_TWO_CMD 200 //两条命令之间的延时
#define HWT101_UART_MAX_WAIT_TIME HAL_MAX_DELAY //最大延时

uint8_t cmd_unlock[]={0xFF,0xAA,0x69,0x88,0xB5}; //解锁寄存器命令
uint8_t cmd_save[]={0xFF,0xAA,0x00,0x00,0x00}; //保存寄存器命令

/**
 * @brief 初始化hwt101
 * 
 * @param csx 句柄
 * @param uart 与hwt101连接的串口
 */
void hwt101_Init(hwt101_csx *csx,UART_HandleTypeDef *uart)
{
    csx->hwt101_uart=uart;
    csx->have_gyro_data=0;
    csx->have_yaw_data=0;
    
    HAL_UARTEx_ReceiveToIdle_DMA(uart, csx->rx_buf, HWT101_RX_BUFFER_SIZE);
}

/**
 * @brief 在DMA空闲中断里面调用
 * 
 * @param csx hwt101句柄
 * @param Size DMA传进来的字节数
 */
void hwt101_Feed(hwt101_csx *csx,uint16_t Size)
{
    //一次DMA可能收到0x52和0x53两个连续帧，逐个解析
    for(uint16_t offset=0; offset+11<=Size; offset+=11)
    {
        uint8_t *frame=&csx->rx_buf[offset];

        if(frame[0]!=HWT101_DATA_HEAD)
        {
            continue;
        }

        if(frame[1]==HWT101_GYRO_DATA_TYPE) //如果数据包是角速度
        {
            uint16_t raw = ((frame[7] << 8) | frame[6]);
            csx->gyro_data = raw / 32768.0 * 2000.0;
            csx->have_gyro_data=1; //已经有角速度数据了
        }
        else if(frame[1]==HWT101_YAW_DATA_TYPE) //如果数据包是偏航角
        {
            uint16_t raw = ((frame[7] << 8) | frame[6]);
            csx->yaw_data = raw / 32768.0 * 180.0;
            csx->have_yaw_data=1; //已经有偏航角数据了
        }
    }
    //重启dma
    HAL_UARTEx_ReceiveToIdle_DMA(csx->hwt101_uart, csx->rx_buf, HWT101_RX_BUFFER_SIZE); 
}
/**
 * @brief 用于读取角速度数据
 * 
 * @param csx 上下文
 * @param data 接收数据的变量
 * @return uint8_t 读取是否成功，成功返回0，失败返回1
 */
uint8_t hwt101_read_gyro(hwt101_csx *csx,float *data)
{
    if(csx->have_gyro_data == 1)
    {
        *data=csx -> gyro_data;
        return 0;
    }
    return 1;
}

/**
 * @brief 用于读取偏航角数据
 * 
 * @param csx 上下文
 * @param data 接收数据的变量
 * @return uint8_t 读取是否成功，成功返回0，失败返回1
 */
uint8_t hwt101_read_yaw(hwt101_csx *csx,float *data)
{
    if(csx->have_yaw_data == 1)
    {
        *data=csx -> yaw_data;
        return 0;
    }
    return 1;
}

/**
 * @brief 写入指定偏航角
 * 
 * @param csx 上下文
 * @param yaw 指定偏航角,单位°
 */
void hwt101_set_yaw(hwt101_csx *csx,float yaw)
{
    int16_t raw_yaw = (int16_t)((yaw * 32768.0f) / 180.0f);

    uint8_t cmd[]={0xFF,0xAA,0x76,0x00,0x00};
    cmd[3]=raw_yaw & 0xFF; //提取低八位
    cmd[4] = (raw_yaw >> 8) & 0xFF; //提取高八位
    HAL_UART_Transmit(csx->hwt101_uart,cmd_unlock,sizeof(cmd_unlock),HWT101_UART_MAX_WAIT_TIME);
    HAL_Delay(HWT101_DELAY_BETWEEN_TWO_CMD);
    HAL_UART_Transmit(csx->hwt101_uart,cmd,sizeof(cmd),HWT101_UART_MAX_WAIT_TIME);
    HAL_Delay(HWT101_DELAY_BETWEEN_TWO_CMD);
    HAL_UART_Transmit(csx->hwt101_uart,cmd_save,sizeof(cmd_save),HWT101_UART_MAX_WAIT_TIME);
}
/**
 * @brief 清零偏航角
 * 
 * @param csx 上下文
 */
void hwt101_reset_yaw(hwt101_csx *csx)
{
    hwt101_set_yaw(csx,0);
}

/**
 * @brief 设置led亮灭
 * 
 * @param csx 上下文
 * @param led_state 1是亮，0是灭
 */
void hwt101_led(hwt101_csx *csx,uint8_t led_state)
{
    uint8_t cmd[]={0xFF,0xAA,0x1B,~led_state,0x00};
    HAL_UART_Transmit(csx->hwt101_uart,cmd,sizeof(cmd),HWT101_UART_MAX_WAIT_TIME);
}
