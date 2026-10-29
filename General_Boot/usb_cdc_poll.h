/**
 * ******************************************************************************
 * @file    usb_cdc_poll.h
 * @brief   Header file for openBLT non-interrupt USB CDC polling driver.
 * @target  STM32G0B1 / STM32G0C1 / STM32G4xx (USB DRD IP)
 * @author  YourName (GitHub: YourProfile)
 * @license GPL v3 (Aligned with OpenBLT)
 * ******************************************************************************
 */

#ifndef USB_CDC_POLL_H
#define USB_CDC_POLL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "boot.h"
#include "stm32g0xx.h"   

/* ========================================================================== */
/*                                Macro Definitions                           */
/* ========================================================================== */
#define USB_RX_BUF_SIZE         256U    /**< Ring buffer size (Must be power of 2) */

/* ========================================================================== */
/*                               Exported Functions                           */
/* ========================================================================== */

/**
 * @brief  Configure and enable the internal HSI48 clock with CRS auto-calibration.
 * @note   Must be called after OpenBLT's TimerInit() but before MX_USB_Device_Init().
 *         This prevents RCC hardware deadlocks by not modifying the main PLL.
 */
void USB_Clock_Config(void);

/**
 * @brief  Pushes a single byte into the USB RX ring buffer.
 * @param  data: Byte received from USB OUT endpoint.
 */
void USB_Poll_RxBufferWrite(uint8_t data);

/**
 * @brief  Dispatches and drives the underlying USB & Type-C hardware state machines.
 * @note   This replaces the hardware ISR and must be polled continuously in main while(1).
 */
void USB_Poll_HardwareSubsystem(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_CDC_POLL_H */
