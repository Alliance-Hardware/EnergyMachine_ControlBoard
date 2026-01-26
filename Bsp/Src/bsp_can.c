#include "can.h"
#include "main.h"
#include "m3508_ctrl.h"
#include "bsp_can.h"
#ifdef CAN
/* ---------- CAN 配置 ---------- */
CAN_RxFrame_TypeDef CanRxFrame;
CAN_TxFrame_TypeDef CanTxFrame = {
  .hcan = &hcan,
  .Header.StdId = 0x00,
  .Header.RTR = CAN_RTR_DATA,
  .Header.IDE = CAN_ID_STD,
  .Header.DLC = 8, // 默认为 8 字节
  .Header.TransmitGlobalTime = DISABLE,
};

/**
 * @brief 配置 CAN1 的滤波器
 * @note CAN1 使用 Filter Bank 0
 */
static void CAN_Filter_Config(void) {
  CAN_FilterTypeDef Can_FilterConfig;

  Can_FilterConfig.FilterBank = 0; // CAN1 使用 Filter Bank 0
  Can_FilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; // 屏蔽模式
  Can_FilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  Can_FilterConfig.FilterIdHigh = 0x0000; // 过滤器 ID
  Can_FilterConfig.FilterIdLow = 0x0000;
  Can_FilterConfig.FilterMaskIdHigh = 0x0000; // 屏蔽码（全 0 接收所有 ID）
  Can_FilterConfig.FilterMaskIdLow = 0x0000;
  Can_FilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; // 接收到 FIFO0
  Can_FilterConfig.FilterActivation = ENABLE;
  Can_FilterConfig.SlaveStartFilterBank =
      14; // 从属 CAN2 的滤波器起始号 (F105)

  if (HAL_CAN_ConfigFilter(&hcan, &Can_FilterConfig) != HAL_OK) {
    Error_Handler();
  }
}
#endif

#ifdef CAN1
/* ---------- CAN1 配置 ---------- */
CAN_RxFrame_TypeDef Can1RxFrame;
CAN_TxFrame_TypeDef Can1TxFrame = {
    .hcan = &hcan1,
    .Header.StdId = 0x00,
    .Header.RTR = CAN_RTR_DATA,
    .Header.IDE = CAN_ID_STD,
    .Header.DLC = 8, // 默认为 8 字节
    .Header.TransmitGlobalTime = DISABLE,
};

/**
 * @brief 配置 CAN1 的滤波器
 * @note CAN1 使用 Filter Bank 0
 */

static void CAN1_Filter_Config(void) {
  CAN_FilterTypeDef Can1_FilterConfig;

  Can1_FilterConfig.FilterBank = 0; // CAN1 使用 Filter Bank 0
  Can1_FilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; // 屏蔽模式
  Can1_FilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  Can1_FilterConfig.FilterIdHigh = 0x0000; // 过滤器 ID
  Can1_FilterConfig.FilterIdLow = 0x0000;
  Can1_FilterConfig.FilterMaskIdHigh = 0x0000; // 屏蔽码（全 0 接收所有 ID）
  Can1_FilterConfig.FilterMaskIdLow = 0x0000;
  Can1_FilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; // 接收到 FIFO0
  Can1_FilterConfig.FilterActivation = ENABLE;
  Can1_FilterConfig.SlaveStartFilterBank =
      14; // 从属 CAN2 的滤波器起始号 (F105)

  if (HAL_CAN_ConfigFilter(&hcan1, &Can1_FilterConfig) != HAL_OK) {
    Error_Handler();
  }
}
#endif


#ifdef CAN2
/* ---------- CAN2 配置 ---------- */
CAN_RxFrame_TypeDef Can2RxFrame;
CAN_TxFrame_TypeDef Can2TxFrame = {
    .hcan = &hcan2,
    .Header.StdId = 0x00,
    .Header.RTR = CAN_RTR_DATA,
    .Header.IDE = CAN_ID_STD,
    .Header.DLC = 8, // 默认为 8 字节
    .Header.TransmitGlobalTime = DISABLE,
};

/**
 * @brief 配置 CAN2 的滤波器
 * @note CAN2 使用 Filter Bank 14 (或更高，具体取决于 CubeMX 配置)
 */
static void CAN2_Filter_Config(void) {
  CAN_FilterTypeDef Can2_FilterConfig;

  Can2_FilterConfig.FilterBank = 14; // CAN2 使用 Filter Bank 14
  Can2_FilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  Can2_FilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
  Can2_FilterConfig.FilterIdHigh = 0x0000;
  Can2_FilterConfig.FilterIdLow = 0x0000;
  Can2_FilterConfig.FilterMaskIdHigh = 0x0000;
  Can2_FilterConfig.FilterMaskIdLow = 0x0000;
  Can2_FilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; // 接收到 FIFO0
  Can2_FilterConfig.FilterActivation = ENABLE;
  // F105 中 CAN2 的滤波器配置依赖于 CAN1 的 SlaveStartFilterBank

  if (HAL_CAN_ConfigFilter(&hcan2, &Can2_FilterConfig) != HAL_OK) {
    Error_Handler();
  }
}
#endif

/* ---------- CAN1 & CAN2 初始化 ---------- */
void BSP_CAN_Init(void) {
#ifdef CAN
  // 1. 配置 CAN 滤波器
  CAN_Filter_Config();
  // 2. 激活中断
  if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) !=
      HAL_OK) {
    Error_Handler();
  }
  // 3. 启动 CAN 外设
  if (HAL_CAN_Start(&hcan) != HAL_OK) {
    Error_Handler();
  }
#endif
#ifdef CAN1
    // 1. 配置 CAN1 滤波器
    CAN1_Filter_Config();
    // 2. 激活中断
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) !=
        HAL_OK) {
      Error_Handler();
    }
    // 3. 启动 CAN 外设
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
      Error_Handler();
    }
#endif
#ifdef CAN2
    // 1. 配置 CAN2 滤波器
    CAN2_Filter_Config();
    // 2. 激活中断
    if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) !=
        HAL_OK) {
      Error_Handler();
    }
    // 3. 启动 CAN 外设
    if (HAL_CAN_Start(&hcan2) != HAL_OK) {
      Error_Handler();
    }
#endif
}

/* ---------- CAN 发送函数 ---------- */
/**
 * @brief 发送 CAN 消息
 * @param hcan: CAN 句柄 (&hcan1 或 &hcan2)
 * @param id: 消息 ID (StdId)
 * @param tx_data: 待发送的 8 字节数据
 */
void BSP_CAN_SendMsg(CAN_HandleTypeDef *hcan, uint16_t id, uint8_t *tx_data) {
  uint32_t mailbox;
  CAN_TxHeaderTypeDef tx_header = {
      .StdId = id,
      .RTR = CAN_RTR_DATA,
      .IDE = CAN_ID_STD,
      .DLC = 8,
      .TransmitGlobalTime = DISABLE,
  };
#ifndef CAN
  // 确保 hcan 是有效的 CAN 句柄
  if (hcan == &hcan1) {
    // 使用 CAN1 的 TxHeader
  } else if (hcan == &hcan2) {
    // 使用 CAN2 的 TxHeader
  } else {
    return;
  }
#endif
  // 发送消息，使用 HAL_CAN_AddTxMessage 函数
  if (HAL_CAN_AddTxMessage(hcan, &tx_header, tx_data, &mailbox) != HAL_OK) {
  }
}

/* ---------- CAN 接收中断回调函数 ---------- */

// 重新实现 HAL_CAN_RxFifo0MsgPendingCallback (标准 HAL 库的命名)
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
#ifndef CAN
  if (hcan == &hcan1) {
    // CAN_RxHeaderTypeDef rx1_header;
    // uint8_t rx1_data[8];
    // HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx1_header, rx1_data);
    // HUB75_CAN_RxCallback(rx1_header.StdId, rx1_data); // 传入标识符和数据
  } else if (hcan == &hcan2) {
    // CAN_RxHeaderTypeDef rx_header;
    // uint8_t rx_data[8];
    // HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);
    // M3508_CAN_RxCallback(rx_header.StdId, rx_data); // 传入标识符和数据
  }
#endif

#ifdef CAN

#endif
}