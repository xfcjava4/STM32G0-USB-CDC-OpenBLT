/**
 * ******************************************************************************
 * @file    usb_cdc_poll.c
 * @brief   Source file for openBLT non-interrupt USB CDC polling driver.
 *          Resolves the deadlocks between ST HAL and openBLT state machines.
 * ******************************************************************************
 */

#include "usb_cdc_poll.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_cdc.h"

/* ========================================================================== */
/*                               External Handles                             */
/* ========================================================================== */
extern USBD_HandleTypeDef hUsbDeviceFS;     /**< ST USB Device Core Handle */
extern PCD_HandleTypeDef  hpcd_USB_DRD_FS;  /**< ST USB Peripheral Driver Handle */
extern uint8_t            UserRxBufferFS[]; /**< USB Hardware Layer RX FIFO Buffer */

/* ========================================================================== */
/*                               Global Variables                             */
/* ========================================================================== */
/** 
 * @brief USB Receiver Ring Buffer and its indexes.
 * @note  Marked as volatile to guarantee atomic visibility across cross-file polling.
 */
uint8_t          usb_rx_ring_buf[USB_RX_BUF_SIZE];
volatile uint16_t usb_rx_head = 0U;
volatile uint16_t usb_rx_tail = 0U;

/* ========================================================================== */
/*                            Function Implementations                        */
/* ========================================================================== */

void USB_Clock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  RCC_CRSInitTypeDef crsInit = {0};

  /* 1. Enable and stabilize the dedicated internal 48MHz oscillator */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State     = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE; /* Keep openBLT's system PLL untouched */
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while (1); /* Hardware fault blocker */
  }

  /* 2. Route HSI48 output directly into the USB peripheral clock domain */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection    = RCC_USBCLKSOURCE_HSI48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    while (1);
  }

  /* 3. Activate Clock Recovery System (CRS) for crystal-less USB synchronization */
  __HAL_RCC_CRS_CLK_ENABLE();
  
  crsInit.Prescaler             = RCC_CRS_SYNC_DIV1; /* Word-aligned no division */
  crsInit.Source                = RCC_CRS_SYNC_SOURCE_USB; /* Sync target to USB SOF packet */
  crsInit.Polarity              = RCC_CRS_SYNC_POLARITY_RISING;
  crsInit.ReloadValue           = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000U, 1000U);
  crsInit.ErrorLimitValue       = RCC_CRS_ERRORLIMIT_DEFAULT;
  crsInit.HSI48CalibrationValue = RCC_CRS_HSI48CALIBRATION_DEFAULT;

  HAL_RCCEx_CRSConfig(&crsInit);
}

void USB_Poll_RxBufferWrite(uint8_t data)
{
  uint16_t next_head = (usb_rx_head + 1) % USB_RX_BUF_SIZE;
  
  /* Queue-Full Protection: Discard byte if memory bounds are reached */
  if (next_head != usb_rx_tail) 
  {
    usb_rx_ring_buf[usb_rx_head] = data;
    usb_rx_head = next_head;
  }
}

void USB_Poll_HardwareSubsystem(void)
{
  /* Replaces the hardware ISR: Poll Type-C Port connection state machine */
  #ifdef USBPD_PORT0_IRQHandler
    USBPD_PORT0_IRQHandler();
  #endif

  /* Replaces the hardware ISR: Drive the USB peripheral core protocol state machine */
  HAL_PCD_IRQHandler(&hpcd_USB_DRD_FS);
}
