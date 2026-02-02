#include "FreeRTOS.h"
#include "RandomData.h"
#include <stdlib.h>
#include "timer.h"
uint8_t GetRandomData(uint8_t* in, uint8_t* out, uint8_t num) {
    srand(get_time());      //每次进入重新初始化种子,避免每次上电首次小能量机关显示相同顺序
    // ==================== 第一部分：参数检查 ====================
    // 参数检查：确保n的值在有效范围内（1或2）
    // 如果n为0或大于2，说明参数错误
    if (num == 0 || num > 2) {
        // 将输出数组的两个位置都设为0，表示无有效结果
        out[0] = out[1] = 0;
        return 0;  // 返回0表示没有选择任何数据
    }

    // ==================== 第二部分：收集有效数据 ====================
    // 定义局部数组和计数器：
    // valid[5]: 临时数组，用于存储输入数组中所有不为0的值
    // cnt: 计数器，记录valid数组中实际存储的有效数据个数
    uint8_t valid[5], cnt = 0;
    // 遍历输入数组的5个元素
    for (uint8_t i = 0; i < 5; i++) {
        // 检查当前元素是否为0
        if (in[i] != 0) {
            uint8_t is_in_out = 0;
            for (uint8_t j = 0; j < 2; j++) {
                if (out[j] == in[i]) {
                    is_in_out = 1;
                    break;
                }
            }
            // 如果不为0，而且和out的数据不同,将其存入valid数组
            if (!is_in_out) {
                valid[cnt] = in[i];
                cnt++;  // 计数器加1
            }
        }
    }

    // ==================== 第三部分：检查是否有有效数据 ====================
    // 如果cnt为0，说明输入数组全是0，没有有效数据
    if (cnt == 0) {
        out[0] = out[1] = 0;
        return 0;
    }

    // ==================== 第四部分：根据n的值选择数据 ====================
    // 情况1：只需要选择1个数据
    if (num == 1) {
        // rand() % cnt: 生成0到cnt-1之间的随机数
        out[0] = valid[rand() % cnt];
        out[1] = 0;
        return 1;       // 返回1表示成功选择了1个数据
    }
    // 情况2：需要选择2个数据
    // 如果有效数据只有1个
    if (cnt == 1) {
        out[0] = out[1] = 0;
        return 0;
    }
    // 有效数据有2个或以上，可以选出2个不同的数据
    // 随机第一个数
    uint8_t idx1 = rand() % cnt;
    // 随机第二个数
    uint8_t idx2;
    // 使用do-while循环确保第二个数与第一个不同
    do {
        idx2 = rand() % cnt;
    } while (idx2 == idx1);   // 如果与第一个相同，重新生成
    // 将选中的数据存入输出数组
    out[0] = valid[idx1];
    out[1] = valid[idx2];
    return 2;    // 返回2表示成功选择了2个不同的数据
}
