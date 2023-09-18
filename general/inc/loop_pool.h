/**
 * @file loop_pool.h
 * @brief while(1)中定时循环的实现
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2023-04-07
 * 
 * @copyright Copyright (c) 2023  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */


#ifndef _LOOP_POOL_H_
#define _LOOP_POOL_H_


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */
#include "typedef.h"
#define _looppool_get_tickms() GET_TICK()


/**
 * @brief 在循环中进行定时调用
 * @param phase_ms          相位，第一次运行将会偏相，此值应该小于等于cycle_ms，当为0时与LOOPPOOL_CALL_MS一样
 * @param cycle_ms          定时时间
 * @param action            需要被定时调用的代码块
 */
#define LOOPPOOL_PHASE_CALL_MS(phase_ms, cycle_ms, action) do{      \
        static uint32_t __looppool_last_time = 0;   \
        uint32_t __looppool_current_time = _looppool_get_tickms();\
        uint32_t __looppool_diff_time = __looppool_current_time - __looppool_last_time;\
        __looppool_last_time = __looppool_last_time == 0 ? __looppool_current_time-cycle_ms+phase_ms : __looppool_last_time;  \
        if(cycle_ms == 0) { \
            action; \
        }else if(__looppool_diff_time >= cycle_ms){    \
            __looppool_last_time += __looppool_diff_time/cycle_ms*cycle_ms;   \
            action; \
        }\
    }while(0)


/**
 * @brief 在循环中进行定时调用
 * @param cycle_ms          定时时间
 * @param action            需要被定时调用的代码块
 */
#define LOOPPOOL_CALL_MS(cycle_ms, action) LOOPPOOL_PHASE_CALL_MS(0, cycle_ms, action)



static __attribute__ ((__used__)) int __looppool_bool_debounce(uint32_t debounce_ms, uint32_t new_bool_state, uint32_t *last_time, 
                                        uint32_t *last_state, uint32_t *last_last_state, uint32_t *last_call_time){
    if(debounce_ms == 0) return new_bool_state;
    if(*last_state == 0xffffffff || (_looppool_get_tickms() - *last_call_time) > (debounce_ms/2))
    {
        *last_time = _looppool_get_tickms();
        *last_call_time = _looppool_get_tickms();
        *last_last_state = *last_state = new_bool_state;
        return new_bool_state;
    }
    *last_call_time = _looppool_get_tickms();
    if(*last_state == new_bool_state){
        if((_looppool_get_tickms() - *last_time) > debounce_ms){
            *last_last_state = *last_state;
        }
        return *last_last_state;
    }
    *last_time = _looppool_get_tickms();
    *last_state = !(*last_state);
    return *last_last_state;
}

/**
 * @brief 自动防抖，此宏必须要被定时调用，且调用频率必须远大于防抖的时间
          否则会产生bug
 * @param debounce_ms       防抖时间
 * @param current_bool      当前bool值
 */
#define LOOPPOOL_BOOL_DEBOUNCE(debounce_ms, current_bool)       ({\
    static uint32_t __last_time__ = 0;                                  \
    static uint32_t __last_state__ = 0xffffffff;                        \
    static uint32_t __last_last_state__ = 0;                            \
    static uint32_t __last_call_time__ = 0;                             \
    __looppool_bool_debounce(debounce_ms, !!(current_bool), &__last_time__, &__last_state__, &__last_last_state__, &__last_call_time__);  \
})



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif // _LOOP_POOL_H_


