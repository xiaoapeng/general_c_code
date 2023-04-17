/**
 * @file debug.h
 * @brief debug用
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2021-04-11
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

#define LOG_INTERVAL_TIME 		20			/* N分钟一个文件 */
#define LOG_MAX_FILE_COUNT		10			/* 最多能存储M个文件 */
#define LOG_DIR_PATH			"/userdata/log"	    /* 日志路径 */

typedef enum _DBG_LEVEL
{
    DBG_ERR = 1,
    DBG_WARNING,
    DBG_SYS,
    DBG_INFO,
    DBG_DEBUG,
}DBG_LEVEL;

extern DBG_LEVEL dbg_level;

extern int debug_printf(int print_time,const char *format, ...);
extern void debug_phex(const void *buf,  int len);


extern int debug_init(void);
extern void debug_exit(void);
#define DBG_print_hex(level,buf,len) 	 if(level<=dbg_level) debug_phex((buf),(len))
#define DBG_print_fl(level,s,params...)  if(level<=dbg_level) debug_printf(1, "[%s, %d]: " s "\r\n", __FUNCTION__, __LINE__, ##params)
#define DBG_print(level,s,params...)     if(level<=dbg_level) debug_printf(1, s "\r\n", ##params)
#define DBG_printf(level,s,params...)    if(level<=dbg_level) debug_printf(1, s , ##params)
#define DBG_printf_raw(level,s,params...) 	 if(level<=dbg_level) debug_printf(0, s ,##params)
#define DBG_set_dbg_Level(_level) 		 (dbg_level = (_level))

/**
 * @brief 打印错误原因，且执行 action 语句
 */
#define DBG_INIT_ERROR_EXEC(expression, action)  do{            \
    if(expression){                                             \
        DBG_print_fl(DBG_ERR,"("#expression") execute {"#action"}"); \
        action;                                                 \
    }                                                           \
}while(0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __DEBUG__H__ */



