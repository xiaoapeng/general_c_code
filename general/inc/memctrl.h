/**
 * @file memctrl.h
 * @brief 非常方便的内存操作函数
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2021-03-16
 * 
 * @copyright Copyright (c) 2023  simon.xiaoapeng@gmail.com
 * 
 *
 */

#ifndef __MEMCTRL_H__
#define __MEMCTRL_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "bits.h"


#define _MEM_BYTE_ORDER_BIG 	4321			/* 大端定义 */
#define _MEM_BYTE_ORDER_LITTLE  1234			/* 小段定义 */

/* 用户可在这里使用 _MEM_USER_DEF_ORDER 定义字节序 */
//#define _MEM_USER_DEF_ORDER 	_MEM_BYTE_ORDER_LITTLE








#ifdef _MEM_USER_DEF_ORDER
#define _MEM_BYTE_ORDER 		(_MEM_USER_DEF_ORDER)
#endif

#if defined(_MEM_BYTE_ORDER) && _MEM_BYTE_ORDER != _MEM_BYTE_ORDER_BIG && _MEM_BYTE_ORDER != _MEM_BYTE_ORDER_LITTLE
#undef _MEM_BYTE_ORDER
#endif

#ifndef _MEM_BYTE_ORDER
	#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
		#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			/* 大端 */
			#define _MEM_BYTE_ORDER  _MEM_BYTE_ORDER_BIG
		#else
			#define _MEM_BYTE_ORDER  _MEM_BYTE_ORDER_LITTLE
			/* 小段 */
		#endif /* __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ */
	#else
		/* 若没有定义__BYTE_ORDER__ 一般在windows系统上编译 那么默认系统类型为小端 */
		#define _MEM_BYTE_ORDER  _MEM_BYTE_ORDER_LITTLE
	#endif
#endif

/* 设置内存时并将字节序翻转,不公开 */
#define _SET_TURN_MEM_VAL_TYPE(pdst, val, type) \
		do{\
			*((type *)(pdst)) = (type)(val);\
			byte_order_change((pdst), sizeof(type));\
		}while(0)








/**
 * @brief 内存字节序转换
 * @param ptr 			指针
 * @param len 			内容大小
 */
static inline void byte_order_change(void* ptr, uint32_t len){
	int i; char *p = (char*)(ptr);
	for(i=0;i<(len)/2;i++){
		p[(len)-i-1] ^= p[i];
		p[i] ^=p[(len)-i-1];
		p[(len)-i-1] ^= p[i];
	}
}


/**
 * @brief 指针以单字节自增
 * @param ptr 			指针
 * @param len 			自增数
 */
#define MEM_INC(ptr, len) ((typeof(ptr))((char *)(ptr) + (len)))

/**
 * @brief 内存字节序转换
 *		BYTE_ORDER_CHANGE: 					执行字节序转换
 *		BYTE_ORDER_LITTLE_TO_SYSTEM:		将内存中的小端字节序转换为系统字节序
 *		BYTE_ORDER_BIG_TO_SYSTEM:			将内存中的大端字节序转换为系统字节序
 * @param ptr 			指针
 * @param len 			内容大小
 */
#define BYTE_ORDER_CHANGE(ptr,len) byte_order_change((ptr), (len))

#if _MEM_BYTE_ORDER == _MEM_BYTE_ORDER_LITTLE
	#define BYTE_ORDER_LITTLE_TO_SYSTEM(ptr, len) 
	#define BYTE_ORDER_BIG_TO_SYSTEM(ptr, len) 		BYTE_ORDER_CHANGE(ptr,len)
#else
	#define BYTE_ORDER_TO_LITTLE(ptr, len) 		BYTE_ORDER_CHANGE(ptr,len)
	#define BYTE_ORDER_TO_BIG(ptr, len)
#endif



/** 
 * @brief 用一个值，来设置一段内存
 *		val只有为变量的时候才能正确编译
 *		注意：该宏在windows编译器用不了,使用宏SET_MEM_VAL_TYPE替代
 * @param pdst 			目的地址
 * @param val 			值
 */
#define SET_MEM_VAL(pdst,val) \
		do{\
			*((typeof(&val))(pdst)) = (val);\
		}while(0)

/** 
 * @brief 用一个值，来设置一段内存
 *		SET_MEM_VAL_TYPE:				设置时内存时原样拷贝，忽略字节序大小端
 *		SET_LITTLE_MEM_VAL_TYPE:		设置内存时将其改变为小端存储 将根据系统情况来决定是否进行字节序转换
 *		SET_BIG_MEM_VAL_TYPE:			设置内存时将其改变为大端存储 将根据系统情况来决定是否进行字节序转换
 * @param pdst 			目的地址
 * @param val 			值
 * @param type 			要设置值的类型
 */
#define SET_MEM_VAL_TYPE(pdst, val, type) \
		do{\
			*((type *)(pdst)) = (type)(val);\
		}while(0)
		
#if _MEM_BYTE_ORDER == _MEM_BYTE_ORDER_LITTLE
	#define SET_LITTLE_MEM_VAL_TYPE(pdst, val, type) 	SET_MEM_VAL_TYPE(pdst, val, type)
	#define SET_BIG_MEM_VAL_TYPE(pdst, val, type)    	_SET_TURN_MEM_VAL_TYPE(pdst, val, type)

#else
	#define SET_BIG_MEM_VAL_TYPE(pdst, val, type)  	  	SET_MEM_VAL_TYPE(pdst, val, type)
	#define SET_LITTLE_MEM_VAL_TYPE(pdst, val, type)  	_SET_TURN_MEM_VAL_TYPE(pdst, val, type)
#endif



/** 
 * @brief 获取内存中某个类型的值
 * @param pdst 			某段内存
 * @param type 			类型
 */
#define GET_MEM_VAL(psrc, type)  (*((type *)(psrc)))





#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __MEMCTRL_H__ */
