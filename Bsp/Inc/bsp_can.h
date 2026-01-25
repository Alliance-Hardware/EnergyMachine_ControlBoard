#ifndef BSP_CAN_H
#define BSP_CAN_H

#include "can.h"
#include "main.h"
#ifdef CAN
extern CAN_HandleTypeDef hcan;
#endif
#ifdef CAN1
extern CAN_HandleTypeDef hcan1;
#endif
#ifdef CAN2
extern CAN_HandleTypeDef hcan2;
#endif
// 外部声明 CubeMX 生成的 CAN 句柄

// 标准 CAN 的 Tx/Rx Header 结构体
// 标准 CAN 外设的数据帧长度固定为 8 字节，无需在结构体中存储 Data 数组

typedef struct {
  CAN_HandleTypeDef *hcan;
  CAN_TxHeaderTypeDef Header;
  // 实际数据在发送函数中作为参数传入
} CAN_TxFrame_TypeDef;

typedef struct {
  CAN_HandleTypeDef *hcan;
  CAN_RxHeaderTypeDef Header;
  uint8_t Data[8]; // 接收时用于存储数据
} CAN_RxFrame_TypeDef;

// 外部声明 CAN 的 Tx/Rx 结构体

#ifdef CAN
extern CAN_TxFrame_TypeDef CanTxFrame;
extern CAN_RxFrame_TypeDef CanRxFrame;
#endif

#ifdef CAN1
extern CAN_TxFrame_TypeDef Can1TxFrame;
extern CAN_RxFrame_TypeDef Can1RxFrame;
#endif

#ifdef CAN2
extern CAN_TxFrame_TypeDef Can2TxFrame;
extern CAN_RxFrame_TypeDef Can2RxFrame;
#endif

// 外部函数声明
extern void BSP_CAN_Init(void);
extern void BSP_CAN_SendMsg(CAN_HandleTypeDef *hcan, uint16_t id,
                            uint8_t *tx_data);

#endif