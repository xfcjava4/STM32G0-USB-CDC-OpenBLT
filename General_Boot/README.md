# OpenBLT STM32G0/G4 USB-CDC Polling Driver 🚀

[!IAR Status](https://shields.io)
[!License](https://shields.io)

本驱动专为 **STM32G0B1 / STM32G0C1 / STM32G4** 等
新型 **USB DRD 内核** 的 MCU 芯片量身打造。

成功解决在 **OpenBLT 纯轮询、无中断** 架构下，
移植 ST 官方 HAL 库 USB-CDC 所引发的时钟死锁、
中断锁死、2字节截断断流、异步发送覆盖丢包等
一整套让人抓狂的连环硬件级底层大坑。

让你**彻底抛弃慢吞吞的物理串口（UART/RS232）**，
直接利用板载的 **Type-C 接口虚拟串口**，
实现**秒级固件闪电升级**！

---

## 🌟 核心技术痛点破解

在纯轮询无中断环境下，ST 原厂驱动存在连环状态冲突。
本项目通过以下硬核架构实现完美闭环：

* **时钟死锁破解**：
  独立使能内部 `HSI48` + `CRS`（时钟恢复系统）自动校准。
  绝不动主 PLL 寄存器，完美顺应 `TimerInit()` 启动时序。
  实现**无外部晶振（Crystal-less）**的高精度 USB 通信。

* **硬件级 NAK 流控拦截**：
  彻底移除 `CDC_Receive_FS` 中急躁的接收复位触发。
  将其后置于 OpenBLT 环形队列彻底消费完数据的最顶层。
  利用 USB 协议天然的 **NAK 机制**控制电脑发包节奏。
  **根治了纯轮询下 2 字节截断断流的死穴**。

* **安全单发状态监视**：
  用 `hcdc->TxState` 纯状态机监视锁定总线竞争。
  取代盲目的发送触发，
  **实现了单字节高速对轰下的零重发、零丢包**。

* **0 Warning 编译品质**：
  针对 IAR 严格的 `Warning[Pa082]` 警告
  （即单条语句交叉访问 `volatile` 变量）。
  采用**无锁环形队列局部快照技术**拆分读取。
  彻底抹平未定义行为隐痕，保证代码极度严谨。

---

## 🛠️ 快速移植说明 (4步通关)

整个驱动模块高度解耦，仅需以下 4 步即可嵌入工程：

### 1. 加入核心驱动
将本仓库的 `usb_cdc_poll.c/.h` 加入工程编译树。

### 2. 覆盖接口文件
用本仓库的 `usbd_cdc_if.c/.h` 直接覆盖原工程
由 CubeMX 生成的同名文件。
> **注**：本文件的 `CDC_Receive_FS` 已为你移除了
> 末尾的接收复位触发，流控完全移交给了上层应用。

### 3. 重定向 `rs232.c`
打开 OpenBLT 的 `rs232.c`，在顶部包含头文件：
```c
#include "usb_cdc_poll.h"
#include "usbd_cdc_if.h"
```
用本仓库示例中的 `Rs232ReceiveByte` 和
`Rs232TransmitByte` 函数，
直接替换原本读写底层硬件 USART 寄存器的旧函数。

### 4. 构建 `main.c` 纯轮询大底座
确保 `blt_config.h` 中已关闭物理 UART：
`#define BOOT_COM_UART_ENABLE (0)`
随后将 `main.c` 整理为如下清晰的时序：

```c
void main(void)
{  
  HAL_Init();  
  SystemClock_Config();
  
  /* 1. 初始化 Boot 业务（1ms 计数器就绪） */
  BootInit();
  
  /* 2. 开启 USB 专用 HSI48 + CRS 时钟 */
  USB_Clock_Config();
  
  /* 3. 初始化并启动 USB CDC 设备 */
  MX_USB_Device_Init();    
  USBD_Start(&hUsbDeviceFS);    
  
  /* 保持硬件中断完全关闭，主循环进行纯轮询工作 */
  while (1)
  {
    /* 4. 驱动 Type-C 与 USB 核心协议栈状态机 */
    USB_Poll_HardwareSubsystem();
    
    /* 5. 运行 Boot 业务层，自动消费数据 */
    BootTask();
  }
}
```

---

## ⚠️ 避坑硬核小贴士

* **多文件共享变量**：
  定义环形队列 `usb_rx_ring_buf` 及其读写索引时，
  **千万不要加 `static` 关键字**！
  否则变量可见性被隔离，两边玩“平行时空”直接死锁。

* **APP 端的配套修改**：
  Bootloader 烧录完成并成功跳转到 APP 后，
  **APP 的 `main` 函数开头必须将中断向量表偏移**！
  （例如 `SCB->VTOR = 0x08005000;`）
  否则进入 APP 后各外设的中断将无法正常响应。

---

## 📂 Project Directory Structure

```text
STM32G0-OpenBLT-USB-CDC/
├── General_Boot/                     # Core Bootloader implementation folder
│   ├── bin/                          # Binary output directory (.bin, .hex, .srec)
│   ├── ide/                          # IAR Embedded Workbench project workspace
│   │   └── STM32G0B1_Boot.eww        # Main IAR workspace file
│   ├── lib/                          # Underlying core library files
│   ├── USB_Device/                   # ST Microelectronics USB Device Stack Middleware
│   │   └── App/
│   │       ├── usbd_cdc_if.c         # Modified media interface (NAK deferred flow control)
│   │       └── usbd_cdc_if.h         # Global export header for CDC transmission APIs
│   ├── flash_layout.c                # Flash sector and block sizing configuration
│   ├── hooks.c                       # OpenBLT native callback hook implementations
│   ├── main.c                        # Application entrance (Contains pure polling while(1) loop)
│   ├── usb_cdc_poll.c                # Hardware driver (HSI48 clock initialization & CRS engine)
│   ├── usb_cdc_poll.h                # Ring buffer geometry settings and hardware driver prototypes
│   ├── blt_conf.h                    # Global openBLT macro profile (Contains BOOT_COM_USB_CDC_ENABLE)
│   ├── stm32g0b1xx_flash.icf         # IAR linker script defining memory limits (0x08000000 - 0x08005FFF)
│   └── startup_stm32g0b1xx.s         # Low-level assembly reset handler vector definitions
├── Source/                           # OpenBLT base repository source code
│   └── ARMCM0_STM32G0/               # Target architecture port files for Cortex-M0+ G0 series
│       └── rs232.c                   # Serial port redirection module with conditional compilation macro
├── App_Prog/                         # Example reference application (Vector Table offset to 0x08006000)
└── README.md                         # This international documentation file
```

## 🤝 贡献与致谢

本项目在开发过程中经历了一场极其硬核的底层死磕。
如果你觉得它拯救了你饱受折磨的头发，欢迎点一个 **Star 🌟**！
