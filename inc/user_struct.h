#ifndef __USER_STRUCT_H_
#define __USER_STRUCT_H_

#include "user_define.h"
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "rga/rga.h"
#include "rkmedia_common.h"
#include "rknn_api.h"
#include "user_define.h"
#include "rkmedia_api.h"
#include "rtsp_demo.h"

typedef enum
{
    YOLOX = 0,
    YOLOV5,
    YOLOV7
} MODEL_TYPE;

typedef enum
{
    U8 = 0,
    FP = 1,
} POST_PROCESS_TYPE;

typedef enum
{
    SINGLE_IMG = 0,
    MULTI_IMG,
    VIDEO_STREAM
} INPUT_SOURCE;

typedef struct _MODEL_INFO
{
    MODEL_TYPE m_type;
    POST_PROCESS_TYPE post_type;
    INPUT_SOURCE in_source;

    char *m_path = nullptr;
    char *in_path = nullptr;

    int channel;
    int height;
    int width;
    RgaSURF_FORMAT color_expect;

    int anchors[18];
    int anchor_per_branch;

    int in_nodes;
    rknn_tensor_attr *in_attr = nullptr;

    int out_nodes = 3;
    rknn_tensor_attr *out_attr = nullptr;

    int strides[3] = {8, 16, 32};

} MODEL_INFO;

typedef struct _LETTER_BOX
{
    int in_width, in_height;
    int target_width, target_height;

    float img_wh_ratio, target_wh_ratio, resize_scale;
    int resize_width, resize_height;
    int h_pad, w_pad;
    bool add_extra_sz_h_pad = false;
    bool add_extra_sz_w_pad = false;
} LETTER_BOX;

typedef struct g_box_info_t
{
    int x;
    int y;
    int w;
    int h;

    int left;
    int right;
    int top;
    int bottom;

    int centerX;
    int centerY;

} box_info_t;

typedef struct _BOX_RECT
{
    int left;
    int right;
    int top;
    int bottom;
} BOX_RECT;

typedef struct _detect_result_t
{
    char name[OBJ_NAME_MAX_SIZE];
    int class_index;
    BOX_RECT box;
    float prop;
} detect_result_t;

typedef struct _detect_result_group_t
{
    int id;
    int detect_count;
    detect_result_t results[OBJ_NUMB_MAX_SIZE];
} detect_result_group_t;

struct Session
{
    char path[64];
    CODEC_TYPE_E video_type;
    RK_U32 u32Width;
    RK_U32 u32Height;
    IMAGE_TYPE_E enImageType;
    char videopath[120];

    rtsp_session_handle session;
    MPP_CHN_S stViChn;
    MPP_CHN_S stVenChn;
    MPP_CHN_S stRgaChn;
};

// rknn list to draw boxs asynchronously

struct demo_cfg
{
    int session_count;
    struct Session session_cfg[MAX_SESSION_NUM];
};

typedef struct node {
    long timeval;
    detect_result_group_t detect_result_group;
    struct node *next;
} Node;

typedef struct my_stack {
    int size;
    Node *top;
} rknn_list;

#endif
