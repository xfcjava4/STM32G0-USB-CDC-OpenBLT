/**
 * ******************************************************************************
 * @file    usbd_cdc_if.h
 * @brief   Header file for openBLT non-interrupt USB CDC polling media layer.
 * ******************************************************************************
 */

#ifndef __USBD_CDC_IF_H__
#define __USBD_CDC_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_cdc.h"

/* ========================================================================== */
/*                                 Exported Macros                            */
/* ========================================================================== */
/* Define packet sizes if not defined in compiler preprocessor */
#ifndef CDC_DATA_FS_OUT_PACKET_SIZE
#define CDC_DATA_FS_OUT_PACKET_SIZE                 64U
#endif

/* ========================================================================== */
/*                             Exported Variables                             */
/* ========================================================================== */
extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;
extern uint8_t UserRxBufferFS[CDC_DATA_FS_OUT_PACKET_SIZE];

/* ========================================================================== */
/*                             Exported Functions                             */
/* ========================================================================== */

/**
  * @brief  Exposed transmission wrapper for external file invocation (e.g., rs232.c).
  *         Fires a raw packet payload over full-speed IN endpoint.
  */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CDC_IF_H__ */
