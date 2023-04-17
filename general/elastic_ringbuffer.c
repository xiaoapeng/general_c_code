/**
 * @file elastic_ringbuffer.c
 * @brief 
 *		环形缓冲器特殊实现，可直接获取数组进行数据解析，不需要绕回
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2021-03-01
 * 
 * @copyright Copyright (c) 2021  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */
#include <string.h>

#include "typedef.h"

#include "elastic_ringbuffer.h"

#define _er_malloc MALLOC
#define _er_free   FREE


#define erb_fix(ptr, size)	((ptr)%(size))

static inline bool erb_full(Erb *fifo)
{
	if(erb_fix(fifo->write+1, fifo->mem_size) == fifo->read)
		return true;
	return false;
}

static inline bool erb_empty(Erb *fifo)
{
	if(fifo->write == fifo->read)
		return true;
	return false;
}
#if 0
typedef enum _FIFOSTUAT{
	STUAT_WRITE_LOOP,		/* 写需要环回 */
	STUAT_READ_LOOP,		/* 读需要环回 */
	STUAT_NO_LOOP			/* 不需要环回 */
}FIFOSTUAT;

static inline FIFOSTUAT erb_stuat(Erb *fifo)
{
	if(fifo->write >= fifo->read)
		return STUAT_WRITE_LOOP;
	else
		return STUAT_READ_LOOP;
}
#endif


/*****************************************************************************
 函 数 名  : erb_Size
 功能描述  : 计算已经使用的缓冲区大小
 输入参数  : fifo
 输出参数  : 
 返 回 值  : 
 	返回缓冲区内字节流的大
 修改历史      :
  1.日    期   : 2021年3月3日
    作    者   : 彭洋明
    修改内容   : 新生成函数
*****************************************************************************/
uint32_t erb_Size(Erb *fifo)
{
	uint32_t write;
	if(fifo->read > fifo->write)
		write = fifo->write + fifo->mem_size;
	else
		write = fifo->write;
	return write - fifo->read;
}

/*****************************************************************************
 函 数 名  : erb_FreeSize
 功能描述  : 计算可用缓冲区的大小
 输入参数  : fifo
 输出参数  : 
 返 回 值  : 
 	返回可用缓冲区的大小
 修改历史      :
  1.日    期   : 2021年3月3日
    作    者   : 彭洋明
    修改内容   : 新生成函数
*****************************************************************************/
uint32_t erb_FreeSize(Erb* fifo)
{
	return fifo->mem_size - erb_Size(fifo) - 1;
}

/*****************************************************************************
 函 数 名  : erb_ReadAir
 功能描述  : 读了个寂寞，只对读指针产生偏移，不拿出数据
 输入参数  : 
 		fifo
 		buf_size:	想读寂寞的数量
 输出参数  : 
 返 回 值  : 
 	返回可读寂寞的字节数量
 修改历史      :
  1.日    期   : 2021年3月3日
    作    者   : 彭洋明
    修改内容   : 新生成函数
*****************************************************************************/
uint32_t erb_ReadAir(Erb* fifo, uint32_t buf_size)
{
	uint32_t br=0;
	br = erb_Size(fifo);
	br = br > buf_size ? buf_size : br ;
	fifo->read = erb_fix(fifo->read + br, fifo->mem_size);
	return br;
}

/*****************************************************************************
 函 数 名  : erb_Clear
 功能描述  : 完全清空环形缓冲区
 输入参数  : 
 		fifo
 修改历史      :
  1.日    期   : 2021年3月3日
    作    者   : 彭洋明
    修改内容   : 新生成函数
*****************************************************************************/
void erb_Clear(Erb* fifo)
{
	fifo->read = fifo->write;
}



/*****************************************************************************
 函 数 名  : erb_Peep
 功能描述  : 偷偷观看，观看缓冲区可观看的数据,
 输入参数  : 
 		fifo
 		expect_peep_size:		想偷看数据的大小
 输出参数  : 
 		reality_peep_size:		实际能偷看数据大大小
 返 回 值  : 
 		返回观看字节流数据的指针，注意该指针是只读的
 修改历史      :
  1.日    期   : 2021年3月3日
    作    者   : 彭洋明
    修改内容   : 新生成函数
*****************************************************************************/
const uint8_t* erb_Peep(Erb* fifo, uint32_t expect_peep_size, uint32_t* reality_peep_size)
{
	uint32_t br=0;
	br = erb_Size(fifo);
	br = br > expect_peep_size ? expect_peep_size : br ;
	if(reality_peep_size)
		*reality_peep_size = br;
	return fifo->mem+fifo->read;
}

/*****************************************************************************
 函 数 名  : erb_Peep
 功能描述  : 偷偷观看所有数据
 输入参数  : 
 		fifo
 输出参数  : 
 		peep_size:		偷看数据大大小
 返 回 值  : 
 		返回观看字节流数据的指针，注意该指针是只读的
 修改历史      :
  1.日    期   : 2021年3月3日
    作    者   : 彭洋明
    修改内容   : 新生成函数
*****************************************************************************/
const uint8_t* erb_PeepAll(Erb* fifo, uint32_t* peep_size)
{
	uint32_t br=0;
	br = erb_Size(fifo);
	if(peep_size)
		*peep_size = br;
	return fifo->mem+fifo->read;
}

/*****************************************************************************
 函 数 名  : erb_Read
 功能描述  : 读缓冲区的数据,放入buf缓冲区中
 输入参数  : 
 		fifo
 		buf:		存放字节流的指针
 		buf_size：	缓冲区大小
 返 回 值  : 
 		返回读到的缓冲区大小
 修改历史      :
  1.日    期   : 2021年3月3日
    作    者   : 彭洋明
    修改内容   : 新生成函数
*****************************************************************************/
uint32_t erb_Read(Erb* fifo, uint8_t *buf, uint32_t buf_size)
{
	uint32_t br=0;
	/* 为空 */
	if(erb_empty(fifo)) return 0;
	
	br = erb_Size(fifo);
	br = br > buf_size ? buf_size : br ;
	memcpy(buf, fifo->mem+fifo->read, br);
	/* 移动 */
	fifo->read = erb_fix(fifo->read + br, fifo->mem_size);
	return br;
}

/*****************************************************************************
 函 数 名  : erb_Write
 功能描述  : 写环形缓冲区
 输入参数  : 
 		fifo
 		buf:		要写的数据
 		buf_size：	要写的数据大小
 返 回 值  : 
 		写成功的数量
 修改历史      :
  1.日    期   : 2021年3月3日
    作    者   : 彭洋明
    修改内容   : 新生成函数
*****************************************************************************/
uint32_t erb_Write(Erb* fifo, uint8_t *buf, uint32_t buf_size)
{
	uint32_t wr = 0;

	if(buf == NULL || buf_size == 0) return 0;
	if(erb_full(fifo))	return 0;
	
	wr = erb_FreeSize(fifo);
	if(wr == 0) return 0;
	wr = wr > buf_size ? buf_size : wr;
	memcpy(fifo->mem+fifo->write, buf, wr);

	/* 进行环回 */
	if(fifo->write + wr > fifo->mem_size)
		memcpy(fifo->mem, fifo->mem_tmp, (fifo->write + wr) - fifo->mem_size);
	else
		memcpy(fifo->mem_tmp + fifo->write, buf, wr);
	/* 移动 */
	fifo->write = erb_fix(fifo->write + wr, fifo->mem_size);
	return wr;
}

/*****************************************************************************
 函 数 名  : erb_New
 功能描述  : 新建一个环形缓冲区
 输入参数  : 
 		size:	环形缓冲区的容量
 返 回 值  : 
 		返回环形缓冲区句柄
 修改历史      :
  1.日    期   : 2021年3月3日
    作    者   : 彭洋明
    修改内容   : 新生成函数
*****************************************************************************/
Erb* erb_New(uint32_t size)
{
	Erb *erb_new;
	erb_new = (Erb *)_er_malloc(sizeof(Erb)+size * 2);
	if( erb_new == NULL )
	{
		return NULL;
	}
	erb_new->mem 	  	= (uint8_t *)(erb_new+1);	
	erb_new->mem_tmp 	= erb_new->mem + size;
	erb_new->mem_size 	= size;
	erb_new->read 		= erb_new->write = 0;	
	return erb_new;
}

/*****************************************************************************
 函 数 名  : erb_Del
 功能描述  : 删除环形缓冲区
 输入参数  : 
 		fifo  环形缓冲区句柄
 修改历史      :
  1.日    期   : 2021年3月3日
    作    者   : 彭洋明
    修改内容   : 新生成函数
*****************************************************************************/
void erb_Del(Erb* fifo)
{
	_er_free(fifo);
}



#include "debug.h"
void erb_test(void)
{
	Erb *fifo;
	fifo = erb_New(20);
	char bbufer[21] = {0};

	erb_Write(fifo, (uint8_t *)"123456789", 10);
	erb_Read(fifo,(uint8_t *)bbufer,10);
	DBG_print(DBG_DEBUG, "读出%s", bbufer);

	erb_Write(fifo, (uint8_t *)"123456789", 9);
	erb_Write(fifo, (uint8_t *)"abcdefghij8", 11);
	erb_Read(fifo,(uint8_t *)bbufer,20);
	DBG_print(DBG_DEBUG, "读出%s", bbufer);

	
	erb_Del(fifo);
}








