# HWT101 STM32 HAL Driver

面向 STM32F1 HAL 的 HWT101 单轴陀螺仪驱动。驱动使用 UART DMA 接收数据，同时处理 DMA 传输完成（TC）和串口空闲（IDLE）事件，提供偏航角、Z 轴角速度读取，以及偏航角设置、归零和指示灯控制接口。

## 功能

- 解析 HWT101 的 `0x52` 角速度数据帧和 `0x53` 角度数据帧
- 读取偏航角，单位为度，范围为 `0~360°`
- 读取 Z 轴角速度，单位为度每秒
- 设置指定偏航角和偏航角归零
- 控制 HWT101 指示灯

## 文件说明

- `hwt101.h`：数据结构、缓冲区大小和公开 API
- `hwt101.c`：串口接收、数据解析和控制命令实现

## 依赖

- STM32F1 HAL 库（`hwt101.h` 直接包含 `stm32f1xx_hal.h`）
- 一个全双工 UART，接收端使用 DMA
- UART 全局中断和对应 DMA 中断已使能
- UART 波特率与 HWT101 配置一致

HWT101 的 TX 接 MCU 的 RX，HWT101 的 RX 接 MCU 的 TX，并确保双方共地。

## 接入方法

1. 将 `hwt101.c` 和 `hwt101.h` 加入工程，并让编译器能找到头文件目录。
2. 在 CubeMX 或初始化代码中配置 UART 和 RX DMA，开启 UART 全局中断。
3. 在 UART 和 DMA 初始化完成后调用 `hwt101_Init()`。
4. 在 `HAL_UARTEx_RxEventCallback()` 中处理 `IDLE` 或 `TC` 事件，并把接收长度传给 `hwt101_Feed()`。
5. 在主循环或任务中通过 `hwt101_read_yaw()` 和 `hwt101_read_gyro()` 读取数据。

```c
#include "hwt101.h"

static hwt101_HandleTypeDef hwt101;

void App_Init(void)
{
    hwt101_Init(&hwt101, &huart2);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart == hwt101.hwt101_uart)
    {
        HAL_UART_RxEventTypeTypeDef event = HAL_UARTEx_GetRxEventType(huart);

        if (event == HAL_UART_RXEVENT_IDLE || event == HAL_UART_RXEVENT_TC)
        {
            hwt101_Feed(&hwt101, Size);
        }
    }
}

void App_Loop(void)
{
    float yaw;
    float gyro_z;

    if (hwt101_read_yaw(&hwt101, &yaw) == 0)
    {
        /* yaw 的单位为度，范围为 0~360°。 */
    }

    if (hwt101_read_gyro(&hwt101, &gyro_z) == 0)
    {
        /* gyro_z 的单位为度每秒。 */
    }
}
```

`HAL_UARTEx_RxEventCallback()` 是 HAL 的公共弱回调，整个工程中保留一个有效定义即可。工程中还有其它 UART 时，可以在回调内按 `huart` 分派。

## API

### `void hwt101_Init(hwt101_HandleTypeDef *csx, UART_HandleTypeDef *uart)`

绑定 UART、清除 `have_yaw_data` / `have_gyro_data` 数据有效标志，并以 `HWT101_RX_BUFFER_SIZE` 为长度调用 `HAL_UARTEx_ReceiveToIdle_DMA()` 启动 DMA 接收。应在 UART 和 DMA 初始化完成后调用。调用前需保证 `csx` 为静态或全局变量，且其 `rx_buf` 在接收期间不被释放或挪作他用。

### `void hwt101_Feed(hwt101_HandleTypeDef *csx, uint16_t Size)`

在 `HAL_UARTEx_RxEventCallback()` 收到 `IDLE` 或 `TC` 事件时调用。函数以 11 字节为步长扫描本次接收到的数据，校验帧头 `0x55` 和数据类型后解析角速度、偏航角帧，最后重新启动 DMA 接收。

### `uint8_t hwt101_read_yaw(hwt101_HandleTypeDef *csx, float *data)`

读取偏航角，单位为度，范围为 `0~360°`。成功返回 `0` 并写入 `data`，尚未收到有效数据时返回 `1`。

### `uint8_t hwt101_read_gyro(hwt101_HandleTypeDef *csx, float *data)`

读取 Z 轴角速度，单位为度每秒。成功返回 `0` 并写入 `data`，尚未收到有效数据时返回 `1`。

### `void hwt101_set_yaw(hwt101_HandleTypeDef *csx, float yaw)`

将传感器当前偏航角设置为 `yaw`，单位为度。函数依次发送解锁（`FF AA 69 88 B5`）、设置角度（`FF AA 76 <低字节> <高字节>`）和保存寄存器（`FF AA 00 00 00`）三条命令，并在相邻命令之间调用 `HAL_Delay(200)`。

### `void hwt101_reset_yaw(hwt101_HandleTypeDef *csx)`

将当前偏航角设置为 `0`，内部调用 `hwt101_set_yaw(csx, 0)`。

### `void hwt101_led(hwt101_HandleTypeDef *csx, uint8_t led_state)`

控制 HWT101 指示灯，发送命令 `FF AA 1B <~led_state> 00`。`led_state` 为 `1` 时点亮，为 `0` 时熄灭。

## 协议与实现说明

- HWT101 的每个数据帧固定为 11 字节，帧头为 `0x55`。
- HWT101 连续输出时通常先发送一个 `0x52` 角速度帧，紧接着发送一个 `0x53` 角度帧。两个帧之间不保证出现 UART 空闲位，因此不能只依赖 `IDLE` 中断切帧。
- 驱动使用 22 字节 DMA 缓冲区，一次接收通常包含两个连续的 11 字节帧；`hwt101_Feed()` 会按偏移 `0` 和 `11` 逐个解析。
- 如果模块输出之间产生了 `IDLE`，回调收到不足 22 字节时，`hwt101_Feed()` 仍会解析其中完整的 11 字节帧。
- `0x52` 帧的字节 6-7 被解析为 Z 轴角速度，量程换算为 `2000 dps`。角速度保留符号，逆时针为正、顺时针为负。
- `0x53` 帧的字节 6-7 被解析为偏航角，先按 `int16_t` 解释再换算，通常为 `0~360°`，负值情况见下方说明。
- 数据解析以帧长度、帧头和数据类型作为有效性判断条件；帧头不是 `0x55` 的帧会被跳过。
- `HWT101_RX_BUFFER_SIZE` 为 22，对应连续输出的 `0x52` 和 `0x53` 两个完整帧。
- 读取函数不会清除数据有效标志，因此可以反复读取最近一次收到的数据；`hwt101_Init()` 被调用时才会重新置为无效。
- `hwt101_set_yaw()`、`hwt101_reset_yaw()` 和 `hwt101_led()` 使用阻塞式 `HAL_UART_Transmit()`（超时为 `HAL_MAX_DELAY`），需在主循环或普通任务中调用，不要在中断里调用。`hwt101_set_yaw()` 会在相邻控制命令之间各等待 200 ms，因此阻塞时间约 400 ms。

角速度和偏航角的原始值均由字节 6-7 按小端格式组合：

```text
raw = frame[7] << 8 | frame[6]
gyro_z = (int16_t)raw * 2000 / 32768
yaw    = (int16_t)raw * 180 / 32768
```

示例数据：

```text
55 52 00 00 04 00 03 00 00 00 AE
55 53 00 00 00 00 3C F0 E4 27 DF
```

第一行是 `0x52` 角速度帧，第二行是 `0x53` 角度帧。第二帧 byte6/7 为 `3C F0`，换算过程如下：

```text
raw  = 0xF03C = 61500
raw 作为 int16_t 解释为 -4036
yaw  = -4036 * 180 / 32768 = -22.17°
yaw 映射到 0~360° 后约为 337.83°
```

需要注意的是，`hwt101.c` 中偏航角分支先做 `(int16_t)` 转换、再除以 `32768.0`，若直接把 `raw` 当无符号数计算，则 `yaw = 61500 * 180 / 32768 = 337.83°`。两者在当前位置等价，但当原始值为负时，有符号解释的中间结果是负角度（如 `-22.17°`），并不自动落在 `0~360°`。若需要严格保证输出范围，请调用方对结果做一次 `if (yaw < 0) yaw += 360;`，或按无符号方式理解偏航角。

## 许可证

本项目采用 MIT License，详见 [`LICENSE`](LICENSE)。
