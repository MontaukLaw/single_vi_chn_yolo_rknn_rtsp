#ifndef __COMM_H__
#define __COMM_H__

#include <getopt.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <stdint.h>

#include <assert.h>
#include <fcntl.h>

// #include "CImg/CImg.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "user_define.h"
#include "user_struct.h"
#include "rga/rga.h"
#include "rkmedia_api.h"
#include "rkmedia_venc.h"
#include "rknn_api.h"
#include "rtsp_demo.h"
#include "im2d.h"
#include "mp4_process.h"
#include "rknn_model.h"
#include "sample_common.h"
#include "rknn_funcs.h"
#include "yolo.h"
#include "monitor.h"
#include "user_struct.h"
#include "rk_aiq_comm.h"
#include "rga_func.h"
#include "h264_venc.h"


    void common_vi_setup(struct Session *session, VI_CHN_WORK_MODE mode, RK_S32 vi_pipe);

    void common_venc_setup(struct Session *session, bool ifSubStream);
    // void common_venc_setup(struct Session* session);

    unsigned char *load_model(const char *filename, int *model_size);

    // void trans_data_for_yolo_input(unsigned char *rga_buffer_model_input, struct demo_cfg cfg, MEDIA_BUFFER buffer);

    long get_current_time_ms(void);

    int bind_rga_for_vi(struct Session session);

    int count_my_mp4_file_number(void);

    void find_oldest_file_name_prefix(char *file_name_buf);

    void get_file_name_by_time(char *file_name_buf);

    int64_t get_current_time_us(void);

    int bind_vi_rga(RK_S32 cameraId, RK_U32 viChnId, RK_U32 rgaChnId);

    void unbind_vi_rga(RK_S32 cameraId, RK_U32 viChnId, RK_U32 rgaChnId);

    int create_rga(RK_S32 rgaChn, RK_U32 u32Width, RK_U32 u32Height);

    int create_vi(RK_S32 s32CamId, RK_U32 u32Width, RK_U32 u32Height, const RK_CHAR *pVideoNode, RK_S32 viChn);

    void *rknn_yolo_thread(void *args);

    void print_mb_info(MEDIA_BUFFER buffer);

    long get_time_now_us(void);

    long int get_time_now_sec(void);

    void fill_image_data(void *inputData, void *modelData, int inputWidth, int inputHeight, int modelWidth, int modelHeight);

    // int init_isp_raw(void);
    void init_isp(void);

#ifdef __cplusplus
}
#endif

#endif