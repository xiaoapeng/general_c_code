/**
 * @file debug.h
 * @brief debug用
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2023-03-17
 * 
 * @copyright Copyright (c) 2023  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef __DEBUG__H__
#define __DEBUG__H__



#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include <time.h>
/* ######################### CONFIG ######################### */
#define DEBUG_DEFAULT_LEVEL         DBG_DEBUG            /* 默认打印等级 */
#define DEBUG_DEFAULT_TIMEZONE      (8*60*60)           /* 时间戳偏移 */
#define DEBUG_BUF_SIZE              128                 /* print格式化缓冲区 */
#define DEBUG_USE_LOCK              0                   /* 使用互斥锁 */
#define DEBUG_USE_TIME              0                   /* 使用时间显示 */


//extern void debug_pus(char* s);                         /* 实现此函数，即可使用debug功能 */
#include "CH58x_common.h"
static inline void debug_pus(char* s){
    UART1_SendString((uint8_t*)s, strlen(s));
}

extern void debug_lock(void);                           /* 实现此函数，互斥锁：加锁 */
extern void debug_unlock(void);                         /* 实现此函数，互斥锁：解锁 */
extern time_t debug_time(time_t *t);                    /* 实现此参数，日志时间显示 */
/* ########################################################## */

typedef enum _DBG_LEVEL
{
    DBG_ERR = 1,
    DBG_WARNING,
    DBG_SYS,
    DBG_INFO,
    DBG_DEBUG,
}DBG_LEVEL;

extern DBG_LEVEL dbgLevel;

extern int debug_printf(int is_lock, int is_print_time,const char *format, ...);
extern void debug_phex(const void *buf,  int len);


//extern int debug_init(void);
//extern void debug_exit(void);
#define DBG_print_hex(level,buf,len) 	 if(level<=dbgLevel)       debug_phex((buf),(len))

#define DBG_print_fl(level,s,params...)  if(level<=dbgLevel)       debug_printf(1, 1, "[%s, %d]: " s "\r\n", __FUNCTION__, __LINE__, ##params)
#define DBG_print(level,s,params...)     if(level<=dbgLevel)       debug_printf(1, 1, s "\r\n", ##params)
#define DBG_printf(level,s,params...)    if(level<=dbgLevel)       debug_printf(1, 1, s , ##params)
#define DBG_printf_raw(level,s,params...) 	 if(level<=dbgLevel)   debug_printf(1, 1, s ,##params)
#define DBG_set_dbg_Level(_level) 		 (dbgLevel = (_level))


/**
 * @brief 打印错误原因，且执行 action 语句
 */
#define DBG_ERROR_EXEC(expression, label, action)  do{          \
    if(expression){                                             \
        DBG_print_fl(DBG_ERR,label ": ("#expression") execute {"#action"}"); \
        action;                                                 \
    }                                                           \
}while(0)


/**
 * @brief 打印错误原因，且执行 action 语句
 */
#define DBG_INIT_ERROR_EXEC(expression, action) DBG_ERROR_EXEC(expression, "init", action)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __DEBUG__H__ */



