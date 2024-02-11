#ifndef __MY_RKMEDIA_RKNN_MODEL_H
#define __MY_RKMEDIA_RKNN_MODEL_H

#include "yolo.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // int init_model(int argc, char **argv);
    int init_model(void);
    // int predict(void *bufData);
    int predict(void *bufData, detect_result_group_t *detect_result_group);

#ifdef __cplusplus
}
#endif

#endif // __MY_RKMEDIA_RKNN_MODEL_H
