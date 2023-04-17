/**
 * @file memctrl.h
 * @brief 非常方便的内存操作函数
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2021-03-16
 * 
 * @copyright Copyright (c) 2023  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef __MEMCTRL_H__
#define __MEMCTRL_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "bits.h"

/***********************************
 *	内存自增
 *	参数：	
 *		 p :	地址
 *		len:	长度
 ***********************************/
#define MEM_INC(p, len) ((typeof(p))((char *)(p) + (len)))

/***********************************
 *	对单个变量进行字节序的转化
 *	参数：	
 *		ptr：	存放变量的指针
 *		len:	变量的长度
 ***********************************/
#define BYTE_ORDER_CHANGE(ptr,len) \
	do{\
		int __i;\
		char* __p = (char*)(ptr), __tmp;\
		for(__i=0;__i<(len)/2;__i++)\
		{\
			__tmp = __p[__i];\
			__p[__i] =__p[(len)-__i-1];\
			__p[(len)-__i-1] = __tmp;\
		}\
	}while(0)


/***********************************
 *	用一个值，来设置一段内存
 *	参数：	
 *		dst：	目的地址
 *		val:	值
 ***********************************/
#define SET_MEM_VAL(pdst,val) \
		do{\
			typeof(val) __tmp = (val);\
			memcpy((pdst),&__tmp,sizeof(val));\
		}while(0)


/***********************************
 *	获取内存中某个类型的值
 *	参数：	
 *		psrc	某段内存
 *		type:	该值的类型
 ***********************************/
#define GET_MEM_VAL(psrc, type) ({\
				*((type *)(psrc));\
				})


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __MEMCTRL_H__ */
