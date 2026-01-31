#ifndef STM32F105_FR_CYCLICBUFFER_H
#define STM32F105_FR_CYCLICBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 包含头文件 ==================== */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ==================== 类型定义 ==================== */

/**
 * @brief 循环缓冲区结构体
 * @details 用于在任务和中断之间安全传递数据
 */
typedef struct {
    void*              data_buffer;          /**< 数据存储区域指针 */
    size_t             element_size_bytes;   /**< 每个元素的大小（字节） */
    size_t             maximum_capacity;     /**< 最大容量（元素个数） */
    volatile uint32_t  write_index;          /**< 下一个写入位置索引 */
    volatile uint32_t  read_index;           /**< 下一个读取位置索引 */
    volatile uint32_t  element_count;        /**< 当前存储的元素数量 */
} CyclicBuffer_t;

/* ==================== API函数声明 ==================== */

/**
 * @brief 初始化循环缓冲区
 * @param buffer_handle 缓冲区句柄指针
 * @param data_buffer 数据缓冲区指针
 * @param element_size_bytes 单个元素大小（字节）
 * @param maximum_capacity 最大容量（元素个数）
 * @return true: 初始化成功, false: 初始化失败
 */
bool CyclicBuffer_Initialize(CyclicBuffer_t* buffer_handle,
                             void* data_buffer,
                             size_t element_size_bytes,
                             size_t maximum_capacity);

/**
 * @brief 放入元素（在任务中调用）
 * @param buffer_handle 缓冲区句柄指针
 * @param element_data 要放入的元素数据指针
 * @return true: 放入成功, false: 缓冲区已满
 */
bool CyclicBuffer_Put(CyclicBuffer_t* buffer_handle, const void* element_data);

/**
 * @brief 放入元素（在中断服务例程中调用）
 * @param buffer_handle 缓冲区句柄指针
 * @param element_data 要放入的元素数据指针
 * @return true: 放入成功, false: 缓冲区已满
 */
bool CyclicBuffer_PutFromISR(CyclicBuffer_t* buffer_handle, const void* element_data);

/**
 * @brief 取出元素（在任务中调用）
 * @param buffer_handle 缓冲区句柄指针
 * @param element_data 接收数据的指针
 * @return true: 取出成功, false: 缓冲区为空
 */
bool CyclicBuffer_Get(CyclicBuffer_t* buffer_handle, void* element_data);

/**
 * @brief 取出元素（在中断服务例程中调用）
 * @param buffer_handle 缓冲区句柄指针
 * @param element_data 接收数据的指针
 * @return true: 取出成功, false: 缓冲区为空
 */
bool CyclicBuffer_GetFromISR(CyclicBuffer_t* buffer_handle, void* element_data);

/**
 * @brief 检查缓冲区是否为空
 * @param buffer_handle 缓冲区句柄指针
 * @return true: 缓冲区为空, false: 缓冲区非空
 */
bool CyclicBuffer_IsEmpty(CyclicBuffer_t* buffer_handle);

/**
 * @brief 检查缓冲区是否已满
 * @param buffer_handle 缓冲区句柄指针
 * @return true: 缓冲区已满, false: 缓冲区未满
 */
bool CyclicBuffer_IsFull(CyclicBuffer_t* buffer_handle);

/**
 * @brief 获取当前缓冲区中的元素数量
 * @param buffer_handle 缓冲区句柄指针
 * @return 当前元素数量
 */
uint32_t CyclicBuffer_GetCount(CyclicBuffer_t* buffer_handle);

/**
 * @brief 清空缓冲区所有元素
 * @param buffer_handle 缓冲区句柄指针
 */
void CyclicBuffer_Clear(CyclicBuffer_t* buffer_handle);

/* ==================== 宏定义辅助工具 ==================== */
// 全局声明宏（在头文件中使用）
#define CYCLICBUFFER_EXTERN_DECLARE(buffer_name) \
extern CyclicBuffer_t buffer_name##_handle

/**
 * @brief 声明并定义循环缓冲区
 * @param buffer_name 缓冲区名称（用于生成变量名）
 * @param element_type 元素数据类型
 * @param buffer_capacity 缓冲区容量（元素个数）
 *
 * @example CYCLICBUFFER_DECLARE(rx_buffer, CanMessage_t, 32);
 *           生成：rx_buffer_data_array[32] 和 rx_buffer_handle
 */
#define CYCLICBUFFER_DECLARE(buffer_name, element_type, buffer_capacity) \
    element_type buffer_name##_data_array[(buffer_capacity)]; \
    CyclicBuffer_t buffer_name##_handle = { \
        .data_buffer = buffer_name##_data_array, \
        .element_size_bytes = sizeof(element_type), \
        .maximum_capacity = (buffer_capacity), \
        .write_index = 0, \
        .read_index = 0, \
        .element_count = 0 \
    }

/**
 * @brief 初始化已声明的静态循环缓冲区
 * @param buffer_name 缓冲区名称（与声明时一致）
 *
 * @example CYCLICBUFFER_INITIALIZE(rx_buffer);
 */
#define CYCLICBUFFER_INITIALIZE(buffer_name) \
    CyclicBuffer_Initialize( \
        &buffer_name##_handle, \
        buffer_name##_data_array, \
        sizeof(buffer_name##_data_array[0]), \
        sizeof(buffer_name##_data_array) / sizeof(buffer_name##_data_array[0]) \
    )

#ifdef __cplusplus
}
#endif

#endif /* STM32F105_FR_CYCLICBUFFER_H */
