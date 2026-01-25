#ifndef M3508_CTRL_H
#define M3508_CTRL_H

#include <stdbool.h>
#include <stdint.h>

// 控制模式枚举
typedef enum {
  M3508_MODE_CURRENT, // 电流环（直接控制电流）
  M3508_MODE_SPEED,   // 速度环（目标转速）
  M3508_MODE_POSITION // 位置环（目标角度）
} M3508_Mode_t;

// PID参数结构体
typedef struct {
  float kp;             // 比例系数
  float ki;             // 积分系数
  float kd;             // 微分系数
  float output_limit;   // 输出限幅
  float integral_limit; // 积分限幅
} M3508_PID_Params_t;

// 电机反馈数据
typedef struct {
  uint16_t angle;   // 机械角度 (0~8191)
  int16_t speed;    // 转速 (rpm，带符号)
  int16_t current;  // 实际电流 (mA，带符号)
  uint8_t temp;     // 温度 (°C)
  uint8_t err_code; // 错误码 (0为正常)
} M3508_Feedback_t;

// 电机控制句柄（包含PID相关参数）
typedef struct {
  uint8_t id;                   // 电机ID (1~8)
  M3508_Mode_t mode;            // 控制模式
  M3508_PID_Params_t speed_pid; // 速度环PID参数
  M3508_PID_Params_t pos_pid;   // 位置环PID参数

  // 目标值（根据模式选择使用）
  float target_pos;     // 目标角度 (单位：°，范围0~360)
  float target_speed;   // 目标转速 (单位：rpm)
  float target_current; // 目标电流 (单位：A)

  // 内部状态量
  M3508_Feedback_t feedback; // 反馈数据
  float pos_set;             // 位置环输出（作为速度环目标）
  float speed_set;           // 速度环输出（作为电流环目标）
  void (*can_send)(uint16_t id, uint8_t *data); // CAN发送回调

  // PID内部变量
  float pos_err;      // 位置误差
  float pos_err_last; // 上一次位置误差
  float pos_integral; // 位置积分

  float speed_err;      // 速度误差
  float speed_err_last; // 上一次速度误差
  float speed_integral; // 速度积分
} M3508_Handle_t;

// 初始化电机管理器
void M3508_ManagerInit(M3508_Handle_t *motors[], uint8_t count);

// 初始化单个电机
void M3508_Init(M3508_Handle_t *motor, uint8_t id,
                void (*can_send)(uint16_t, uint8_t *));
// 在运行时更改电机句柄对应的电机ID
uint8_t M3508_ChangeMotorId(M3508_Handle_t *motor, uint8_t new_id);
// 设置控制模式
void M3508_SetMode(M3508_Handle_t *motor, M3508_Mode_t mode);

// 设置PID参数
void M3508_SetPIDParams(M3508_Handle_t *motor, M3508_PID_Params_t *speed_params,
                        M3508_PID_Params_t *pos_params);

// 设置目标值（根据当前模式自动生效）
void M3508_SetTarget(M3508_Handle_t *motor, float target);

// 执行PID计算（需周期性调用，建议1ms~10ms）
void M3508_PIDCalculate(void);

// 批量发送电流指令
void M3508_SendAll(void);

// CAN接收回调
void M3508_CAN_RxCallback(uint16_t std_id, uint8_t *data);

#endif