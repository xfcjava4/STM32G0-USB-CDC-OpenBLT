/**
 * ******************************************************************************
 * @file    usbd_cdc_if.c
 * @brief   Source file for openBLT non-interrupt USB CDC polling media layer.
 *          Provides seamless data redirection to openBLT ring buffer.
 * @target  STM32G0B1 / STM32G0C1 / STM32G4xx (USB DRD IP)
 * ******************************************************************************
 */

#include "usbd_cdc_if.h"
#include "usb_cdc_poll.h"

/* ========================================================================== */
/*                               External Handles                             */
/* ========================================================================== */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* ========================================================================== */
/*                               Private Variables                            */
/* ========================================================================== */
static USBD_CDC_LineCodingTypeDef LineCoding =
{
  115200, /* baud rate */
  0x00,   /* stop bits-1 */
  0x00,   /* parity - none */
  0x08    /* nb. of bits 8 */
};

/** @brief USB core receive buffer, sized to full packet width */
uint8_t UserRxBufferFS[CDC_DATA_FS_OUT_PACKET_SIZE];

/* ========================================================================== */
/*                               Private Functions                             */
/* ========================================================================== */
static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len);
static int8_t CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t *Len, uint8_t epnum);

/** @brief USB CDC Interface Operations Structure */
USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS,
  CDC_TransmitCplt_FS
};

static int8_t CDC_Init_FS(void)
{
  /* Connect the hardware level RX pointer to our global storage array */
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  
  /* Prime the endpoint to expect the first packet from host */
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  
  return (USBD_OK);
}

static int8_t CDC_DeInit_FS(void)
{
  return (USBD_OK);
}

static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  switch (cmd)
  {
    case CDC_SEND_ENCAPSULATED_COMMAND:
      break;
    case CDC_GET_ENCAPSULATED_RESPONSE:
      break;
    case CDC_SET_COMM_FEATURE:
      break;
    case CDC_GET_COMM_FEATURE:
      break;
    case CDC_CLEAR_COMM_FEATURE:
      break;

    case CDC_SET_LINE_CODING:
      LineCoding.bitrate    = (uint32_t)(pbuf[0] | (pbuf[1] << 8) |\
                              (pbuf[2] << 16) | (pbuf[3] << 24));
      LineCoding.format     = pbuf[4];
      LineCoding.paritytype = pbuf[5];
      LineCoding.datatype   = pbuf[6];
      break;

    case CDC_GET_LINE_CODING:
      pbuf[0] = (uint8_t)(LineCoding.bitrate);
      pbuf[1] = (uint8_t)(LineCoding.bitrate >> 8);
      pbuf[2] = (uint8_t)(LineCoding.bitrate >> 16);
      pbuf[3] = (uint8_t)(LineCoding.bitrate >> 24);
      pbuf[4] = LineCoding.format;
      pbuf[5] = LineCoding.paritytype;
      pbuf[6] = LineCoding.datatype;
      break;

    case CDC_SET_CONTROL_LINE_STATE:
      break;
    case CDC_SEND_BREAK:
      break;
    default:
      break;
  }
  return (USBD_OK);
}

static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  uint32_t data_len = *Len;

  if ((data_len > 0U) && (Buf != NULL))
  {
    /* Stream bytes directly into the non-interrupt shared ring buffer */
    for (uint32_t i = 0U; i < data_len; i++)
    {
      USB_Poll_RxBufferWrite(Buf[i]);
    }
  }

  /**
   * @note  [CRITICAL NAK FLOW CONTROL]
   *        Instant endpoint re-triggering is disabled here.
   *        It is deferred to Rs232ReceiveByte() inside rs232.c to implement 
   *        hardware-level flow control and prevent packet truncation.
   */
  return (USBD_OK);
}

/**
  * @brief  Transmits raw data over USB IN endpoint.
  * @note   Keep this function GLOBAL (non-static) so it can be called from rs232.c.
  * @param  Buf: Buffer of data to be transmitted.
  * @param  Len: Number of data to be transmitted (in bytes).
  * @retval USBD_OK if success, USBD_BUSY if pipe is locked.
  */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
  
  if (hcdc == NULL)
  {
    return USBD_FAIL;
  }
  
  /* Pipeline busy check */
  if (hcdc->TxState != 0U)
  {
    return USBD_BUSY;
  }
  
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
  result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  
  return result;
}

static int8_t CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t *Len, uint8_t epnum)
{
  UNUSED(Buf);
  UNUSED(Len);
  UNUSED(epnum);
  return (USBD_OK);
}
