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
**/

#ifndef __DEBUG__H__
#define __DEBUG__H__

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


enum dbg_level{
#define  DBG_SUPPRESS                0
#define  DBG_ERR                     1
#define  DBG_WARNING                 2
#define  DBG_SYS                     3
#define  DBG_INFO                    4
#define  DBG_DEBUG                   5
    _DBG_SUPPRESS = DBG_SUPPRESS,    /* 意味着不输出任何信息 */
    _DBG_ERR = DBG_ERR,
    _DBG_WARNING = DBG_WARNING,
    _DBG_SYS = DBG_SYS,
    _DBG_INFO = DBG_INFO,
    _DBG_DEBUG = DBG_DEBUG,
};

enum dbg_flags {
    DBG_FLAGS_WALL_CLOCK         = 0x01, /* [%Y/%m/%d %H:%M:%S] */
    DBG_FLAGS_MONOTONIC_CLOCK    = 0x02, /* [xxxx.xxx] */
    DBG_FLAGS_DEBUG_TAG          = 0x04, /* [DBG/ERR/WARN/SYS/INFO] */
};

#define DEBUG_CONFIG_STDOUT_MEM_CACHE_SIZE 64
#define DEBUG_CONFIG_DEFAULT_DEBUG_LEVEL DBG_DEBUG


#define DBG_FLAGS                            (DBG_FLAGS_DEBUG_TAG|DBG_FLAGS_MONOTONIC_CLOCK|DBG_FLAGS_WALL_CLOCK)

#if defined(CONFIG_DEBUG_ENTER_SIGN)
#define DEBUG_ENTER_SIGN CONFIG_DEBUG_ENTER_SIGN
#elif (defined CMAKE_CONFIG_DEBUG_ENTER_SIGN)
#define DEBUG_ENTER_SIGN CMAKE_CONFIG_DEBUG_ENTER_SIGN
#else
#define DEBUG_ENTER_SIGN "\r\n"
#endif

#define MACRO_DEBUG_STRINGIFY(name) #name
#define MACRO_DEBUG_LEVEL(name) ((uint32_t)(MACRO_DEBUG_STRINGIFY(name)[0] - '0'))

/* 避免clang警告 */
static inline int __dbg_feign_return(int ret){
    return ret;
}

#define dbg_mprintfl(name, tag, level, fmt, ...) ({\
        int n = 0; \
        if( MACRO_DEBUG_LEVEL(name) >= (uint32_t)level){ \
            n = dbg_printfl(level, "[" #tag "] ", fmt, ##__VA_ARGS__); \
        } \
        __dbg_feign_return(n); \
    })

#define dbg_mprintln(name, tag, level, fmt, ...) ({\
        int n = 0; \
        if( MACRO_DEBUG_LEVEL(name) >= level){ \
            n = dbg_println(level, "[" #tag "] ", fmt, ##__VA_ARGS__); \
        } \
        __dbg_feign_return(n); \
    })

#define dbg_mprintraw(name, level, fmt, ...) ({\
        int n = 0; \
        if( MACRO_DEBUG_LEVEL(name) >= level){ \
            n = dbg_printraw(level, fmt, ##__VA_ARGS__); \
        } \
        __dbg_feign_return(n); \
    })

#define dbg_mhex(name, level, buf, len) ({\
        int n = 0; \
        if( MACRO_DEBUG_LEVEL(name) >= level){ \
            n = dbg_hex(level, DBG_FLAGS, len, buf); \
        } \
        __dbg_feign_return(n); \
    })

/**
 * @brief   dbg模块初始化
 * @param  log_dir                  log文件目录,当目录不存在时自动创建，当log_dir为NULL时，不记录日志到文件
 * @param  max_log_file_num         log文件最大数量, 为小于等于0时不记录日志到文件
 * @param  max_log_file_size        单个log文件最大大小, 小于等于0时对记录大小不设限制
 * @param  max_log_interval_sec     单个log文件记录最大时间, 小于等于0时对记录时间不设限制
 * @return int 
 */
extern int dbg_init(const char* log_dir, int max_log_file_num, int max_log_file_size, int max_log_interval_sec);
extern void dbg_exit(void);
extern int dbg_set_level(enum dbg_level level);


extern int dbg_raw(enum dbg_level level, enum dbg_flags flags, const char *fmt, ...);
extern int vdbg_raw(enum dbg_level level, enum dbg_flags flags, const char *fmt, va_list args);
extern int dbg_hex(enum dbg_level level, enum dbg_flags flags, size_t len, const void *buf);
#define dbg_println(level, tag_str, fmt, ...)     dbg_raw((enum dbg_level)level, (enum dbg_flags)DBG_FLAGS, tag_str fmt DEBUG_ENTER_SIGN , ##__VA_ARGS__)
#define dbg_printfl(level, tag_str, fmt, ...)     dbg_raw((enum dbg_level)level, (enum dbg_flags)DBG_FLAGS, tag_str "[%s, %d]: " fmt DEBUG_ENTER_SIGN , __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define dbg_printraw(level, fmt, ...)             dbg_raw((enum dbg_level)level, 0, fmt, ##__VA_ARGS__)

/* ######################## 下面是简单写法 ####################### */
/* 带自动回车的版本 */
#define dbg_debugln(fmt, ...)               dbg_println(DBG_DEBUG, "", fmt, ##__VA_ARGS__)
#define dbg_infoln(fmt, ...)                dbg_println(DBG_INFO, "", fmt, ##__VA_ARGS__)
#define dbg_sysln(fmt, ...)                 dbg_println(DBG_SYS, "", fmt, ##__VA_ARGS__)
#define dbg_warnln(fmt, ...)                dbg_println(DBG_WARNING, "", fmt, ##__VA_ARGS__)
#define dbg_errln(fmt, ...)                 dbg_println(DBG_ERR, "", fmt, ##__VA_ARGS__)
/* 带自动回车，带函数定位 */
#define dbg_debugfl(fmt, ...)               dbg_printfl(DBG_DEBUG, "", fmt, ##__VA_ARGS__)
#define dbg_infofl(fmt, ...)                dbg_printfl(DBG_INFO, "", fmt, ##__VA_ARGS__)
#define dbg_sysfl(fmt, ...)                 dbg_printfl(DBG_SYS, "", fmt, ##__VA_ARGS__)
#define dbg_warnfl(fmt, ...)                dbg_printfl(DBG_WARNING, "", fmt, ##__VA_ARGS__)
#define dbg_errfl(fmt, ...)                 dbg_printfl(DBG_ERR, "", fmt, ##__VA_ARGS__)
/* 原始数据版本 */
#define dbg_debugraw(fmt, ...)              dbg_printraw(DBG_DEBUG, fmt, ##__VA_ARGS__)
#define dbg_inforaw(fmt, ...)               dbg_printraw(DBG_INFO, fmt, ##__VA_ARGS__)
#define dbg_sysraw(fmt, ...)                dbg_printraw(DBG_SYS, fmt, ##__VA_ARGS__)
#define dbg_warnraw(fmt, ...)               dbg_printraw(DBG_WARNING, fmt, ##__VA_ARGS__)
#define dbg_errraw(fmt, ...)                dbg_printraw(DBG_ERR, fmt, ##__VA_ARGS__)
/* 16进制数组打印 */
#define dbg_debughex(buf,len)               dbg_hex(DBG_DEBUG, DBG_FLAGS, len, buf)
#define dbg_infohex(buf,len)                dbg_hex(DBG_INFO, DBG_FLAGS, len, buf)
#define dbg_syshex(buf,len)                 dbg_hex(DBG_SYS, DBG_FLAGS, len, buf)
#define dbg_warnhex(buf,len)                dbg_hex(DBG_WARNING, DBG_FLAGS, len, buf)
#define dbg_errhex(buf,len)                 dbg_hex(DBG_ERR, DBG_FLAGS, len, buf)


/**
    * 可通过编译宏来精细的控制不同模块的打印等级
    * 假如有如下语句
    *   void func(void){
    *       dbg_mdebugln(FUNC_A, "1 hello world");
    *       dbg_minfoln(FUNC_A, "2 hello world");
    *       dbg_merrln(FUNC_A, "3 hello world");
    *   }
    * 只有输出语句的打印等级同时满足全局打印等级和模块打印等级才会输出
    * 全局打印等级可以通过dbg_set_level函数设置
    * 而模块打印等级则只能在编译器设置,设置宏为DBG_MODEULE_LEVEL_<module_name>来设置
    * 例如：
    *   #define DBG_MODEULE_LEVEL_FUNC_A  DBG_DEBUG
    *   或者由编译脚本来控制 -DDBG_MODEULE_LEVEL_FUNC_A=DBG_DEBUG
    *
    */
/* 模块可进行宏控制打印是否输出*/
#define dbg_mdebugln(name, fmt, ...)    dbg_mprintln(DBG_MODEULE_LEVEL_##name, name, DBG_DEBUG, fmt, ##__VA_ARGS__)
#define dbg_minfoln(name, fmt, ...)     dbg_mprintln(DBG_MODEULE_LEVEL_##name, name, DBG_INFO, fmt, ##__VA_ARGS__)
#define dbg_msysln(name, fmt, ...)      dbg_mprintln(DBG_MODEULE_LEVEL_##name, name, DBG_SYS, fmt, ##__VA_ARGS__)
#define dbg_mwarnln(name, fmt, ...)     dbg_mprintln(DBG_MODEULE_LEVEL_##name, name, DBG_WARNING, fmt, ##__VA_ARGS__)
#define dbg_merrln(name, fmt, ...)      dbg_mprintln(DBG_MODEULE_LEVEL_##name, name, DBG_ERR, fmt, ##__VA_ARGS__)
/* 模块可进行宏控制打印是否输出(带行号和函数名称) */
#define dbg_mdebugfl(name, fmt, ...)    dbg_mprintfl(DBG_MODEULE_LEVEL_##name, name, DBG_DEBUG, fmt, ##__VA_ARGS__)
#define dbg_minfofl(name, fmt, ...)     dbg_mprintfl(DBG_MODEULE_LEVEL_##name, name, DBG_INFO, fmt, ##__VA_ARGS__)
#define dbg_msysfl(name, fmt, ...)      dbg_mprintfl(DBG_MODEULE_LEVEL_##name, name, DBG_SYS, fmt, ##__VA_ARGS__)
#define dbg_mwarnfl(name, fmt, ...)     dbg_mprintfl(DBG_MODEULE_LEVEL_##name, name, DBG_WARNING, fmt, ##__VA_ARGS__)
#define dbg_merrfl(name, fmt, ...)      dbg_mprintfl(DBG_MODEULE_LEVEL_##name, name, DBG_ERR, fmt, ##__VA_ARGS__)
/* 模块可进行宏控制打印是否输出(原始数据) */
#define dbg_mdebugraw(name, fmt, ...)   dbg_mprintraw(DBG_MODEULE_LEVEL_##name, DBG_DEBUG, fmt, ##__VA_ARGS__)
#define dbg_minforaw(name, fmt, ...)    dbg_mprintraw(DBG_MODEULE_LEVEL_##name, DBG_INFO, fmt, ##__VA_ARGS__)
#define dbg_msysraw(name, fmt, ...)     dbg_mprintraw(DBG_MODEULE_LEVEL_##name, DBG_SYS, fmt, ##__VA_ARGS__)
#define dbg_mwarnraw(name, fmt, ...)    dbg_mprintraw(DBG_MODEULE_LEVEL_##name, DBG_WARNING, fmt, ##__VA_ARGS__)
#define dbg_merrraw(name, fmt, ...)     dbg_mprintraw(DBG_MODEULE_LEVEL_##name, DBG_ERR, fmt, ##__VA_ARGS__)
/* 模块可进行宏控制打印是否输出(16进制) */
#define dbg_mdebughex(name, buf, len)   dbg_mhex(DBG_MODEULE_LEVEL_##name, DBG_DEBUG, buf, len)
#define dbg_minfohex(name, buf, len)    dbg_mhex(DBG_MODEULE_LEVEL_##name, DBG_INFO, buf, len)
#define dbg_msyshex(name, buf, len)     dbg_mhex(DBG_MODEULE_LEVEL_##name, DBG_SYS, buf, len)
#define dbg_mwarnhex(name, buf, len)    dbg_mhex(DBG_MODEULE_LEVEL_##name, DBG_WARNING, buf, len)
#define dbg_merrhex(name, buf, len)     dbg_mhex(DBG_MODEULE_LEVEL_##name, DBG_ERR, buf, len)

/**
    * @brief 打印错误原因，且执行 action 语句
    */
#define DBG_ERROR_EXEC(expression, action)  do{                              \
    if(expression){                                                             \
        dbg_printfl(DBG_ERR, "(%s) execute {%s}", #expression, #action); \
        action;                                                                 \
    }                                                                           \
}while(0)



    





#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __DEBUG__H__ */



