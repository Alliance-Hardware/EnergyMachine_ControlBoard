/* ==================== 包含头文件 ==================== */
#include "CyclicBuffer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 获取指定索引位置元素的指针
 * @param buffer_handle 缓冲区句柄指针
 * @param element_index 元素索引位置
 * @return 指向该元素的指针
 */
static inline void* CyclicBuffer_GetElementPointer(CyclicBuffer_t* buffer_handle, uint32_t element_index) {
    return (uint8_t*)buffer_handle->data_buffer + (element_index * buffer_handle->element_size_bytes);
}

/**
 * @brief 计算下一个环形索引
 * @param current_index 当前索引
 * @param maximum_capacity 最大容量
 * @return 下一个索引位置
 */
static inline uint32_t CyclicBuffer_CalculateNextIndex(uint32_t current_index, uint32_t maximum_capacity) {
    return (current_index + 1) % maximum_capacity;
}

/* ==================== 公共API实现 ==================== */

bool CyclicBuffer_Initialize(CyclicBuffer_t* buffer_handle,
                             void* data_buffer,
                             size_t element_size_bytes,
                             size_t maximum_capacity) {
    /* 参数有效性检查 */
    if (buffer_handle == NULL) {
        return false;
    }
    if (data_buffer == NULL) {
        return false;
    }
    if (element_size_bytes == 0) {
        return false;
    }
    if (maximum_capacity == 0) {
        return false;
    }

    /* 初始化缓冲区结构体成员 */
    buffer_handle->data_buffer = data_buffer;
    buffer_handle->element_size_bytes = element_size_bytes;
    buffer_handle->maximum_capacity = maximum_capacity;
    buffer_handle->write_index = 0;
    buffer_handle->read_index = 0;
    buffer_handle->element_count = 0;

    return true;
}

bool CyclicBuffer_Put(CyclicBuffer_t* buffer_handle, const void* element_data) {
    /* 参数有效性检查 */
    if (buffer_handle == NULL) {
        return false;
    }
    if (element_data == NULL) {
        return false;
    }

    bool operation_successful = false;

    /* 进入临界区（禁止任务调度和中断） */
    taskENTER_CRITICAL();

    /* 检查缓冲区是否已满 */
    if (buffer_handle->element_count < buffer_handle->maximum_capacity) {
        /* 获取写入位置指针 */
        void* write_pointer = CyclicBuffer_GetElementPointer(buffer_handle, buffer_handle->write_index);

        /* 复制数据到缓冲区 */
        (void)memcpy(write_pointer, element_data, buffer_handle->element_size_bytes);

        /* 更新写入索引 */
        buffer_handle->write_index = CyclicBuffer_CalculateNextIndex(
            buffer_handle->write_index,
            buffer_handle->maximum_capacity
        );

        /* 更新元素计数 */
        buffer_handle->element_count++;

        operation_successful = true;
    }

    /* 退出临界区 */
    taskEXIT_CRITICAL();

    return operation_successful;
}

bool CyclicBuffer_PutFromISR(CyclicBuffer_t* buffer_handle, const void* element_data) {
    /* 参数有效性检查 */
    if (buffer_handle == NULL) {
        return false;
    }
    if (element_data == NULL) {
        return false;
    }

    /* 进入临界区（中断版） */
    UBaseType_t interrupt_saved_state = taskENTER_CRITICAL_FROM_ISR();

    /* 检查缓冲区是否已满 */
    if (buffer_handle->element_count >= buffer_handle->maximum_capacity) {
        /* 缓冲区已满，退出临界区并返回失败 */
        taskEXIT_CRITICAL_FROM_ISR(interrupt_saved_state);
        return false;
    }

    /* 获取写入位置指针 */
    void* write_pointer = CyclicBuffer_GetElementPointer(buffer_handle, buffer_handle->write_index);

    /* 复制数据到缓冲区 */
    (void)memcpy(write_pointer, element_data, buffer_handle->element_size_bytes);

    /* 更新写入索引 */
    buffer_handle->write_index = CyclicBuffer_CalculateNextIndex(
        buffer_handle->write_index,
        buffer_handle->maximum_capacity
    );

    /* 更新元素计数 */
    buffer_handle->element_count++;

    /* 退出临界区（中断版） */
    taskEXIT_CRITICAL_FROM_ISR(interrupt_saved_state);

    return true;
}

bool CyclicBuffer_Get(CyclicBuffer_t* buffer_handle, void* element_data) {
    /* 参数有效性检查 */
    if (buffer_handle == NULL) {
        return false;
    }
    if (element_data == NULL) {
        return false;
    }

    bool operation_successful = false;

    /* 进入临界区（禁止任务调度和中断） */
    taskENTER_CRITICAL();

    /* 检查缓冲区是否为空 */
    if (buffer_handle->element_count > 0) {
        /* 获取读取位置指针 */
        void* read_pointer = CyclicBuffer_GetElementPointer(buffer_handle, buffer_handle->read_index);

        /* 从缓冲区复制数据到目标位置 */
        (void)memcpy(element_data, read_pointer, buffer_handle->element_size_bytes);

        /* 更新读取索引 */
        buffer_handle->read_index = CyclicBuffer_CalculateNextIndex(
            buffer_handle->read_index,
            buffer_handle->maximum_capacity
        );

        /* 更新元素计数 */
        buffer_handle->element_count--;

        operation_successful = true;
    }

    /* 退出临界区 */
    taskEXIT_CRITICAL();

    return operation_successful;
}

bool CyclicBuffer_GetFromISR(CyclicBuffer_t* buffer_handle, void* element_data) {
    /* 参数有效性检查 */
    if (buffer_handle == NULL) {
        return false;
    }
    if (element_data == NULL) {
        return false;
    }

    /* 进入临界区（中断版） */
    UBaseType_t interrupt_saved_state = taskENTER_CRITICAL_FROM_ISR();

    /* 检查缓冲区是否为空 */
    if (buffer_handle->element_count == 0) {
        /* 缓冲区为空，退出临界区并返回失败 */
        taskEXIT_CRITICAL_FROM_ISR(interrupt_saved_state);
        return false;
    }

    /* 获取读取位置指针 */
    void* read_pointer = CyclicBuffer_GetElementPointer(buffer_handle, buffer_handle->read_index);

    /* 从缓冲区复制数据到目标位置 */
    (void)memcpy(element_data, read_pointer, buffer_handle->element_size_bytes);

    /* 更新读取索引 */
    buffer_handle->read_index = CyclicBuffer_CalculateNextIndex(
        buffer_handle->read_index,
        buffer_handle->maximum_capacity
    );

    /* 更新元素计数 */
    buffer_handle->element_count--;

    /* 退出临界区（中断版） */
    taskEXIT_CRITICAL_FROM_ISR(interrupt_saved_state);

    return true;
}

bool CyclicBuffer_IsEmpty(CyclicBuffer_t* buffer_handle) {
    if (buffer_handle == NULL) {
        return true;
    }

    /* 注意：检查是否为空不需要进入临界区，
       因为即使读到的值不是最新的，也不会导致系统错误 */
    return (buffer_handle->element_count == 0);
}

bool CyclicBuffer_IsFull(CyclicBuffer_t* buffer_handle) {
    if (buffer_handle == NULL) {
        return true;
    }

    /* 注意：检查是否已满不需要进入临界区，
       因为即使读到的值不是最新的，也不会导致系统错误 */
    return (buffer_handle->element_count >= buffer_handle->maximum_capacity);
}

uint32_t CyclicBuffer_GetCount(CyclicBuffer_t* buffer_handle) {
    if (buffer_handle == NULL) {
        return 0;
    }

    /* 注意：获取数量不需要进入临界区，
       因为即使读到的值不是最新的，也不会导致系统错误 */
    return buffer_handle->element_count;
}

void CyclicBuffer_Clear(CyclicBuffer_t* buffer_handle) {
    if (buffer_handle == NULL) {
        return;
    }

    /* 进入临界区（禁止任务调度和中断） */
    taskENTER_CRITICAL();

    /* 重置缓冲区状态 */
    buffer_handle->write_index = 0;
    buffer_handle->read_index = 0;
    buffer_handle->element_count = 0;

    /* 退出临界区 */
    taskEXIT_CRITICAL();
}
