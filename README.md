# HWT101 STM32 HAL Driver

面向 STM32F4 HAL 的 HWT101 单轴陀螺仪驱动。驱动使用 UART DMA 空闲中断接收数据，提供偏航角、Z 轴角速度读取，以及偏航角设置、归零和指示灯控制接口。

## 功能

- 解析 HWT101 的 `0x52` 角速度数据帧和 `0x53` 角度数据帧
- 读取偏航角，单位为度
- 读取 Z 轴角速度，单位为度每秒
- 设置指定偏航角和偏航角归零
- 控制 HWT101 指示灯

## 文件说明

- `hwt101.h`：数据结构、缓冲区大小和公开 API
- `hwt101.c`：串口接收、数据解析和控制命令实现

## 依赖

- STM32F4 HAL 库
- 一个全双工 UART，接收端使用 DMA
- UART 全局中断和对应 DMA 中断已使能
- UART 波特率与 HWT101 配置一致

HWT101 的 TX 接 MCU 的 RX，HWT101 的 RX 接 MCU 的 TX，并确保双方共地。

## 接入方法

1. 将 `hwt101.c` 和 `hwt101.h` 加入工程，并让编译器能找到头文件目录。
2. 在 CubeMX 或初始化代码中配置 UART 和 RX DMA，开启 UART 全局中断。
3. 在 UART 和 DMA 初始化完成后调用 `hwt101_Init()`。
4. 在 `HAL_UARTEx_RxEventCallback()` 中把接收长度传给 `hwt101_Feed()`。
5. 在主循环或任务中通过 `hwt101_read_yaw()` 和 `hwt101_read_gyro()` 读取数据。

```c
#include "hwt101.h"

static hwt101_csx hwt101;

void App_Init(void)
{
    hwt101_Init(&hwt101, &huart2);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == hwt101.hwt101_uart)
    {
        hwt101_Feed(&hwt101, Size);
    }
}

void App_Loop(void)
{
    float yaw;
    float gyro_z;

    if (hwt101_read_yaw(&hwt101, &yaw) == 0)
    {
        /* yaw 的单位为度。 */
    }

    if (hwt101_read_gyro(&hwt101, &gyro_z) == 0)
    {
        /* gyro_z 的单位为度每秒。 */
    }
}
```

`HAL_UARTEx_RxEventCallback()` 是 HAL 的公共弱回调，整个工程中只能保留一个有效定义。若工程中还有其它 UART，需要在回调内按 `huart` 分派。

## API

### `void hwt101_Init(hwt101_csx *csx, UART_HandleTypeDef *uart)`

绑定 UART、清除数据有效标志，并启动 DMA 空闲中断接收。应在 UART 和 DMA 初始化完成后调用。

### `void hwt101_Feed(hwt101_csx *csx, uint16_t Size)`

在 `HAL_UARTEx_RxEventCallback()` 中调用。函数检查 11 字节数据帧，解析角速度或偏航角，并重新启动 DMA 接收。

### `uint8_t hwt101_read_yaw(hwt101_csx *csx, float *data)`

读取偏航角。成功返回 `0` 并写入 `data`，尚未收到有效数据时返回 `1`。

### `uint8_t hwt101_read_gyro(hwt101_csx *csx, float *data)`

读取 Z 轴角速度。成功返回 `0` 并写入 `data`，尚未收到有效数据时返回 `1`。

### `void hwt101_set_yaw(hwt101_csx *csx, float yaw)`

将传感器当前偏航角设置为 `yaw`，单位为度。函数依次发送解锁、设置角度和保存寄存器命令，并在相邻命令之间等待 200 ms。

### `void hwt101_reset_yaw(hwt101_csx *csx)`

将当前偏航角设置为 `0`，内部调用 `hwt101_set_yaw()`。

### `void hwt101_led(hwt101_csx *csx, uint8_t led_state)`

控制 HWT101 指示灯。`led_state` 为 `1` 时点亮，为 `0` 时熄灭。

## 协议与实现说明

- 数据帧按 11 字节、帧头 `0x55` 解析。
- `0x52` 帧的字节 6-7 被解析为 Z 轴角速度，量程换算为 `+/-2000 dps`。
- `0x53` 帧的字节 6-7 被解析为偏航角，量程换算为 `+/-180 deg`。
- 当前实现只检查帧长度、帧头和数据类型，未校验数据帧最后一个字节的 checksum。
- `HWT101_RX_BUFFER_SIZE` 为 12，但有效数据帧长度为 11，多出的 1 字节用于预留。
- `hwt101_set_yaw()`、`hwt101_reset_yaw()` 和 `hwt101_led()` 使用阻塞式 UART 发送；不要在中断或对实时性要求很高的任务中调用。`hwt101_set_yaw()` 最多会阻塞约 400 ms，并考虑了串口发送等待时间。

## 许可证

本项目采用 MIT License，详见 `LICENSE`。
