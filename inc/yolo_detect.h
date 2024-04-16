#ifndef __YOLO_DETECT_H__
#define __YOLO_DETECT_H__

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <vector>

#ifdef __cplusplus
extern "C"
{
#endif

#include "rknn_api.h"

#define PERF_WITH_POST 1

// 用于rknn推理的图像压缩后的大小
#define RKNN_INPUT_IMG_WIDTH 640
#define RKNN_INPUT_IMG_HEIGHT 360

#define MODEL_INPUT_SIZE 640
#define YOLO_INPUT_SIZE (MODEL_INPUT_SIZE * MODEL_INPUT_SIZE * 3)

#include <stdint.h>

#define OBJ_NAME_MAX_SIZE 16
#define OBJ_NUMB_MAX_SIZE 64
#define OBJ_CLASS_NUM 80
#define NMS_THRESH 0.6
#define BOX_THRESH 0.5
#define PROP_BOX_SIZE (5 + OBJ_CLASS_NUM)

#if 0
    typedef struct _BOX_RECT
    {
        int left;
        int right;
        int top;
        int bottom;
    } BOX_RECT;

    typedef struct __detect_result_t
    {
        char name[OBJ_NAME_MAX_SIZE];
        BOX_RECT box;
        float prop;
    } detect_result_t;

    typedef struct _detect_result_group_t
    {
        int id;
        int count;
        detect_result_t results[OBJ_NUMB_MAX_SIZE];
    } detect_result_group_t;
#endif

    int post_process(uint8_t *input0, uint8_t *input1, uint8_t *input2, int model_in_h, int model_in_w,
                     float conf_threshold, float nms_threshold, float scale_w, float scale_h,
                     std::vector<uint32_t> &qnt_zps, std::vector<float> &qnt_scales,
                     detect_result_group_t *group);

    void detect(void *img_data, detect_result_group_t *detect_result_group);

    void release_model();

    int init_model(const char *model_name);

#ifdef __cplusplus
}
#endif

#endif // __YOLO_DETECT_H__