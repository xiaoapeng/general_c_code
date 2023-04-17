/**
 * @file hash_kv.h
 * @brief 
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2020-01-20
 * 
 * @copyright Copyright (c) 2020  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
 */

#ifndef __HASH_KV_H__
#define __HASH_KV_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */


#ifndef NULL
#define NULL ((void*)0)
#endif


enum HashIterState
{
	ITER_EXIT,	/* 退出迭代 */
	ITER_KEEP,	/* 保持迭代 */
};


typedef void* HashKv_t;
typedef struct{void*a;void*b;} HashBlob_t;

/* Blob 操作 性能高 */
extern int		hash_ReleaseBlob(HashBlob_t *b);
extern int		hash_AcquireBlob(HashKv_t ht, HashBlob_t *b,char* k);
extern int		hash_DelKeyFormBlob(HashBlob_t *b);
extern int		hash_BlobGetData(HashBlob_t * b, void * buf, uint32_t read_len);
extern int		hash_BlobSetData(HashBlob_t *b,const void* data,uint32_t data_len);

static inline int  hash_BlobSetString(HashBlob_t *b, char* string)
{
	return hash_BlobSetData(b, string, strlen(string)+1);
}

extern const char* hash_BlobGetKey(HashBlob_t *b);

/* key 操作 */
extern void 	hash_SetLock(HashKv_t ht,int (*lock)(void),void (*unlock)(void));
extern int		hash_SetData(HashKv_t ht,const char* k,const void* data,uint32_t data_len);
extern int		hash_GetData(HashKv_t ht,const char* k,void* buf,uint32_t read_size);
extern int		hash_NewData(HashKv_t ht,const char* k,uint32_t data_len);
extern int		hash_DelKey(HashKv_t ht,const char* k);
static inline int  hash_SetString(HashKv_t ht,const char* k, char* string)
{
	return hash_SetData(ht, k, string, strlen(string)+1);
}
/* 迭代器 */
extern int hash_Iteration(HashKv_t ht, enum HashIterState (*processor)(void* param,HashBlob_t* ),void *param);


/* 哈希表创建与销毁 */
extern HashKv_t hash_New(uint32_t tab_len);
extern void 	hash_Del(HashKv_t ht);








#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __HASH_KV_H__ */
