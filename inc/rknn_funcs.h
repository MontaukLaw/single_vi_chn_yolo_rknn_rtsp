#ifndef __RKNN_FUNCS_H__
#define __RKNN_FUNCS_H__


#ifdef __cplusplus
extern "C"
{
#endif
#include "user_struct.h"
// #include "postprocess.h"


void rknn_list_push(rknn_list *s, long timeval, detect_result_group_t detect_result_group);

int detect_by_buf(void *data, detect_result_group_t *detect_result_group);

void rknn_list_drop(rknn_list *s);

int rknn_list_size(rknn_list *s);

void create_rknn_list(rknn_list **s);

void destory_rknn_list(rknn_list **s);

// int init_model(const char* modelPath);

void rknn_list_pop(rknn_list *s, long *timeval, detect_result_group_t *detect_result_group);

#ifdef __cplusplus
}
#endif

#endif
