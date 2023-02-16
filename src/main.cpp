#include "comm.h"
#include "postprocess.h"
#include "rknn_model.h"
#include "sample_common.h"
#include "rknn_funcs.h"
#include "yolo.h"

#define RTSP_INPUT_VI_WIDTH  1920
#define RTSP_INPUT_VI_HEIGHT 1080

#define MONITOR_CHN 0
#define RKNN_CHN 1
#define ONLY_ONE_VI_CHN 0

#define X_START ((RTSP_INPUT_VI_WIDTH - MODEL_INPUT_SIZE) / 2)
#define Y_START ((RTSP_INPUT_VI_HEIGHT - MODEL_INPUT_SIZE) / 2)
// (1080-640)= 440/2 = 220

// rtsp obj for streaming main_stream and sub_stream
static rtsp_demo_handle g_rtsplive = NULL;
// rknn result list to exchange data between two thread
// its just a simple chain list
static rknn_list *rknn_list_;
// iqfile
static RK_CHAR *pIqfilesPath = (RK_CHAR *) "/oem/etc/iqfiles/";
static RK_S32 s32CamId = 0;
static RK_BOOL bMultictx = RK_FALSE;
static rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
int g_flag_run = 1;

typedef struct g_box_info_t {
    int x;
    int y;
    int w;
    int h;
} box_info_t;

static pthread_t observer_thread_id;
static pthread_t get_rga_buffer_thread_id;
static pthread_t rknn_yolo_thread_id;

static pthread_t venc0_thread_id;
static pthread_t venc1_thread_id;

static struct demo_cfg cfg;
static char *yoloModelFilePath = NULL;
static int yoloModelSize = 0;
RK_BOOL ifDetecting = RK_FALSE;

rtsp_session_handle g_rtsp_session_c0;
rtsp_session_handle g_rtsp_session_c1;

// 直接在nv12的内存上画框
static int nv12_border(char *pic, int pic_w, int pic_h, int rect_x, int rect_y, int rect_w, int rect_h, int R, int G, int B) {
    /* Set up the rectangle border size */
    const int border = 5;

    /* RGB convert YUV */
    int Y, U, V;
    Y = 0.299 * R + 0.587 * G + 0.114 * B;
    U = -0.1687 * R + 0.3313 * G + 0.5 * B + 128;
    V = 0.5 * R - 0.4187 * G - 0.0813 * B + 128;
    /* Locking the scope of rectangle border range */
    int j, k;
    for (j = rect_y; j < rect_y + rect_h; j++) {
        for (k = rect_x; k < rect_x + rect_w; k++) {
            if (k < (rect_x + border) || k > (rect_x + rect_w - border) ||
                j < (rect_y + border) || j > (rect_y + rect_h - border)) {
                /* Components of YUV's storage address index */
                int y_index = j * pic_w + k;
                int u_index = (y_index / 2 - pic_w / 2 * ((j + 1) / 2)) * 2 + pic_w * pic_h;
                int v_index = u_index + 1;
                /* set up YUV's conponents value of rectangle border */
                pic[y_index] = Y;
                pic[u_index] = U;
                pic[v_index] = V;
            }
        }
    }

    return 0;
}

// 初始化isp
static void init_isp(void) {
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, pIqfilesPath);
    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, FPS);
}

static void sig_proc(int signo) {
    fprintf(stderr, "signal %d\n", signo);
    g_flag_run = 0;
}

static void destroy_all(void) {

    if (observer_thread_id) {
        pthread_join(observer_thread_id, NULL);
    }

    if (get_rga_buffer_thread_id) {
        pthread_join(get_rga_buffer_thread_id, NULL);
    }

    if (rknn_yolo_thread_id) {
        pthread_join(rknn_yolo_thread_id, NULL);
    }

    rtsp_del_demo(g_rtsplive);
    RK_MPI_SYS_UnBind(&cfg.session_cfg[0].stViChn, &cfg.session_cfg[0].stVenChn);
    RK_MPI_VENC_DestroyChn(cfg.session_cfg[0].stVenChn.s32ChnId);
    RK_MPI_VI_DisableChn(s32CamId, cfg.session_cfg[0].stViChn.s32ChnId);

    RK_MPI_SYS_UnBind(&cfg.session_cfg[1].stViChn, &cfg.session_cfg[1].stRgaChn);
    RK_MPI_SYS_UnBind(&cfg.session_cfg[1].stRgaChn, &cfg.session_cfg[1].stVenChn);
    RK_MPI_VENC_DestroyChn(cfg.session_cfg[1].stVenChn.s32ChnId);
    RK_MPI_RGA_DestroyChn(cfg.session_cfg[1].stRgaChn.s32ChnId);
    RK_MPI_VI_DisableChn(s32CamId, cfg.session_cfg[1].stViChn.s32ChnId);

    SAMPLE_COMM_ISP_Stop(s32CamId);

    destory_rknn_list(&rknn_list_);
}

void print_mb_info(MEDIA_BUFFER buffer) {
    int cnt = 0;
    printf("#%d Get Frame:ptr:%p, size:%zu, mode:%d, channel:%d, timestamp:%lld\n",
           cnt++, RK_MPI_MB_GetPtr(buffer), RK_MPI_MB_GetSize(buffer),
           RK_MPI_MB_GetModeID(buffer), RK_MPI_MB_GetChannelID(buffer),
           RK_MPI_MB_GetTimestamp(buffer));
}

box_info_t boxInfoList[10];
int boxInfoListNumber = 0;
int boxDisplayCounterDown = 0;

static void *venc0_thread(void *args) {
    MEDIA_BUFFER buffer;
    while (g_flag_run) {

        buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_VENC, 0, -1);
        if (!buffer) {
            usleep(1000);
            continue;
        }

        print_mb_info(buffer);

        RK_MPI_MB_ReleaseBuffer(buffer);
    }
    return nullptr;
}

static void *venc1_thread(void *args) {
    MEDIA_BUFFER buffer;
    while (g_flag_run) {

        buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_VENC, 1, -1);
        if (!buffer) {
            usleep(1000);
            continue;
        }

        print_mb_info(buffer);

        RK_MPI_MB_ReleaseBuffer(buffer);
    }
    return nullptr;
}

static void *observer_thread(void *args) {

    memset(boxInfoList, 0, sizeof(boxInfoList));

    MEDIA_BUFFER buffer;

    while (g_flag_run) {

        buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_VI, DRAW_RESULT_BOX_CHN_INDEX, -1);

        if (!buffer) {
            usleep(1000);
            continue;
        }
        if (rknn_list_size(rknn_list_)) {

            long time_before;
            detect_result_group_t detect_result_group;
            memset(&detect_result_group, 0, sizeof(detect_result_group));

            // pick up the first one
            rknn_list_pop(rknn_list_, &time_before, &detect_result_group);
            // printf("result count:%d \n", detect_result_group.count);

            for (int j = 0; j < detect_result_group.detect_count; j++) {
                int x = detect_result_group.results[j].box.left + X_START;
                int y = detect_result_group.results[j].box.top + Y_START;
                int w = (detect_result_group.results[j].box.right - detect_result_group.results[j].box.left);
                int h = (detect_result_group.results[j].box.bottom - detect_result_group.results[j].box.top);
#if 0
                while ((uint32_t) (x + w) >= RTSP_INPUT_VI_WIDTH) {
                    w -= 16;
                }
                while ((uint32_t) (y + h) >= RTSP_INPUT_VI_HEIGHT) {
                    h -= 16;
                }
#endif
                printf("border=(%d %d %d %d)\n", x, y, w, h);
                boxInfoList[j] = {x, y, w, h};
                boxInfoListNumber++;
            }

            boxDisplayCounterDown = 1;
        }
        for (int i = 0; i < boxInfoListNumber; i++) {
            nv12_border((char *) RK_MPI_MB_GetPtr(buffer), RTSP_INPUT_VI_WIDTH, RTSP_INPUT_VI_HEIGHT,
                        boxInfoList[i].x, boxInfoList[i].y, boxInfoList[i].w, boxInfoList[i].h, 0, 0, 255);
        }

#if 0
        // hold box for 3 frames
        if (boxInfoListNumber > 0) {
            for (int i = 0; i < boxInfoListNumber; i++) {
                nv12_border((char *) RK_MPI_MB_GetPtr(buffer), RTSP_INPUT_VI_WIDTH, RTSP_INPUT_VI_HEIGHT,
                            boxInfoList[i].x, boxInfoList[i].y, boxInfoList[i].w, boxInfoList[i].h, 0, 0, 255);
            }
            boxDisplayCounterDown--;
        }

        if (boxDisplayCounterDown == 0) {
            boxInfoListNumber = 0;
            memset(boxInfoList, 0, sizeof(boxInfoList));
        }
#endif

        boxInfoListNumber = 0;
        memset(boxInfoList, 0, sizeof(boxInfoList));

        nv12_border((char *) RK_MPI_MB_GetPtr(buffer), RTSP_INPUT_VI_WIDTH, RTSP_INPUT_VI_HEIGHT,
                    X_START, Y_START, MODEL_INPUT_SIZE, MODEL_INPUT_SIZE, 0, 200, 0);

        RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, DRAW_RESULT_BOX_CHN_INDEX, buffer);
        RK_MPI_MB_ReleaseBuffer(buffer);
    }
    return nullptr;
}

static int nv12_to_rgb24_640x640(void *yuvBuffer, void *rgbBuffer) {

    rga_buffer_t src, dst;
    memset(&src, 0, sizeof(rga_buffer_t));
    memset(&dst, 0, sizeof(rga_buffer_t));

    src = wrapbuffer_virtualaddr(yuvBuffer, 640, 640, RK_FORMAT_YCbCr_420_SP);
    dst = wrapbuffer_virtualaddr(rgbBuffer, 640, 640, RK_FORMAT_RGB_888);

    src.format = RK_FORMAT_YCbCr_420_SP;
    dst.format = RK_FORMAT_RGB_888;

    IM_STATUS status = imcvtcolor(src, dst, src.format, dst.format);

    if (status != IM_STATUS_SUCCESS) {
        printf("ERROR: imcvtcolor failed!\n");
        return -1;
    }

    return 0;
}

// this thread is for rknn
static void *rknn_yolo_thread(void *args) {

    int ret = 0;

    printf("Start get_media_buffer thread, \n");

    MEDIA_BUFFER buffer = NULL;

    // get data from vi
    while (g_flag_run) {
        ifDetecting = RK_TRUE;
        buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA, RK_NN_RGA_CHN_INDEX, -1);
        if (!buffer) {
            usleep(1000);
            continue;
        }
        // print_mb_info(buffer);

        // int rga_buffer_size = cfg.session_cfg[RK_NN_RGA_CHN_INDEX].u32Width * cfg.session_cfg[RK_NN_INDEX].u32Height * 3; // nv12 3/2, rgb 3
        // int rga_buffer_model_input_size = MODEL_INPUT_SIZE * MODEL_INPUT_SIZE * 3;
        // auto rga_buffer_model_input = (unsigned char*)malloc(rga_buffer_model_input_size);

        // trans_data_for_yolo_input(rga_buffer_model_input, cfg, buffer);
        void *pRknnInputData = malloc(YOLO_INPUT_SIZE);
        ret = nv12_to_rgb24_640x640(RK_MPI_MB_GetPtr(buffer), pRknnInputData);
        if (ret < 0) {
            printf("nv12_to_rgb24_640x640 failed\n");
        }

        if (ret == 0) {
            detect_result_group_t detect_result_group;
            memset(&detect_result_group, 0, sizeof(detect_result_group_t));

            // just for test
            // write_rgb_file(pRknnInputData);

            // Post Process
            // detect_by_buf(pRknnInputData, &detect_result_group);
            int pResult = predict(pRknnInputData, &detect_result_group);

            // put detect result to list
            if (detect_result_group.detect_count > 0) {
                rknn_list_push(rknn_list_, get_current_time_ms(), detect_result_group);
                int size = rknn_list_size(rknn_list_);
                if (size >= MAX_RKNN_LIST_NUM) {
                    rknn_list_drop(rknn_list_);
                }
                // printf("result size: %d\n", size);
            }
        }

        RK_MPI_MB_ReleaseBuffer(buffer);
        free(pRknnInputData);
    }

    return NULL;
}

static void *get_rga_buffer_thread(void *arg) {
    MEDIA_BUFFER buffer = NULL;

    while (g_flag_run) {

        if (RK_TRUE == ifDetecting) {
            buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA, RK_NN_RGA_CHN_INDEX, -1);
            if (!buffer) {
                usleep(1000);
                continue;
            }
            RK_MPI_MB_ReleaseBuffer(buffer);
        }
    }

    return NULL;
}

void venc0_packet_cb(MEDIA_BUFFER mb) {

    static RK_S32 packet_cnt = 0;
    if (g_flag_run == 0)
        return;

    // printf("#Get packet-%d, size %zu\n", packet_cnt, RK_MPI_MB_GetSize(mb));

    if (g_rtsplive && g_rtsp_session_c0) {
        rtsp_tx_video(g_rtsp_session_c0, (const uint8_t *) RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetTimestamp(mb));
        rtsp_do_event(g_rtsplive);
    }

    RK_MPI_MB_ReleaseBuffer(mb);
    packet_cnt++;
}

void monitor_venc_packet_cb(MEDIA_BUFFER mb) {

    static RK_S32 packet_cnt = 0;
    if (g_flag_run == 0)
        return;

    // printf("#Get packet-%d, size %zu\n", packet_cnt, RK_MPI_MB_GetSize(mb));

    if (g_rtsplive && g_rtsp_session_c0) {
        rtsp_tx_video(g_rtsp_session_c0, (const uint8_t *) RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetTimestamp(mb));
        rtsp_do_event(g_rtsplive);
    }

    RK_MPI_MB_ReleaseBuffer(mb);
    packet_cnt++;
}

void rknn_rga_packet_cb(MEDIA_BUFFER mb) {
    int ret = 0;
    if (g_flag_run == 0)
        return;

    print_mb_info(mb);

    void *pRknnInputData = malloc(YOLO_INPUT_SIZE);
    ret = nv12_to_rgb24_640x640(RK_MPI_MB_GetPtr(mb), pRknnInputData);
    if (ret < 0) {
        printf("nv12_to_rgb24_640x640 failed\n");
    }

    if (ret == 0) {
        detect_result_group_t detect_result_group;
        memset(&detect_result_group, 0, sizeof(detect_result_group_t));

        // just for test
        // write_rgb_file(pRknnInputData);

        // Post Process
        int pResult = predict(pRknnInputData, &detect_result_group);

        // put detect result to list
        if (detect_result_group.detect_count > 0) {
            rknn_list_push(rknn_list_, get_current_time_ms(), detect_result_group);
            int size = rknn_list_size(rknn_list_);
            if (size >= MAX_RKNN_LIST_NUM) {
                rknn_list_drop(rknn_list_);
            }
            // printf("result size: %d\n", size);
        }
    }

    RK_MPI_MB_ReleaseBuffer(mb);
    free(pRknnInputData);

}

void monitor_rga_packet_cb_test(MEDIA_BUFFER mb) {
    if (g_flag_run == 0)
        return;

    nv12_border((char *) RK_MPI_MB_GetPtr(mb), RTSP_INPUT_VI_WIDTH, RTSP_INPUT_VI_HEIGHT,
                X_START, Y_START, MODEL_INPUT_SIZE, MODEL_INPUT_SIZE, 0, 200, 0);

    RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, MONITOR_CHN, mb);
    RK_MPI_MB_ReleaseBuffer(mb);

}

void monitor_rga_packet_cb(MEDIA_BUFFER mb) {

    if (g_flag_run == 0)
        return;

    if (rknn_list_size(rknn_list_)) {

        long time_before;
        detect_result_group_t detect_result_group;
        memset(&detect_result_group, 0, sizeof(detect_result_group));

        // pick up the first one
        rknn_list_pop(rknn_list_, &time_before, &detect_result_group);
        // printf("result count:%d \n", detect_result_group.count);

        for (int j = 0; j < detect_result_group.detect_count; j++) {
            int x = detect_result_group.results[j].box.left + X_START;
            int y = detect_result_group.results[j].box.top + Y_START;
            int w = (detect_result_group.results[j].box.right - detect_result_group.results[j].box.left);
            int h = (detect_result_group.results[j].box.bottom - detect_result_group.results[j].box.top);
            printf("border=(%d %d %d %d)\n", x, y, w, h);
            boxInfoList[j] = {x, y, w, h};
            boxInfoListNumber++;
        }

        boxDisplayCounterDown = 1;
    }
    for (int i = 0; i < boxInfoListNumber; i++) {
        nv12_border((char *) RK_MPI_MB_GetPtr(mb), RTSP_INPUT_VI_WIDTH, RTSP_INPUT_VI_HEIGHT,
                    boxInfoList[i].x, boxInfoList[i].y, boxInfoList[i].w, boxInfoList[i].h, 0, 0, 255);
    }

    boxInfoListNumber = 0;
    memset(boxInfoList, 0, sizeof(boxInfoList));

    nv12_border((char *) RK_MPI_MB_GetPtr(mb), RTSP_INPUT_VI_WIDTH, RTSP_INPUT_VI_HEIGHT,
                X_START, Y_START, MODEL_INPUT_SIZE, MODEL_INPUT_SIZE, 0, 200, 0);

    RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, MONITOR_CHN, mb);
    RK_MPI_MB_ReleaseBuffer(mb);
}

void venc1_packet_cb(MEDIA_BUFFER mb) {

    static RK_S32 packet_cnt = 0;
    if (g_flag_run == 0)
        return;

    // printf("#Get packet-%d, size %zu\n", packet_cnt, RK_MPI_MB_GetSize(mb));

    if (g_rtsplive && g_rtsp_session_c1) {
        rtsp_tx_video(g_rtsp_session_c1, (const uint8_t *) RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetTimestamp(mb));
        rtsp_do_event(g_rtsplive);
    }

    RK_MPI_MB_ReleaseBuffer(mb);
    packet_cnt++;
}

// H264 codec
int create_venc_chn(RK_U32 fps, RK_U32 u32Width, RK_U32 u32Height, RK_U32 vencChn) {

    VENC_CHN_ATTR_S venc_chn_attr;
    memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));

    int ret = 0;
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = fps;
    venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = u32Width * u32Height;

    // frame rate: in 30/1, out 30/1.
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = fps;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = fps;

    venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
    venc_chn_attr.stVencAttr.u32PicWidth = u32Width;
    venc_chn_attr.stVencAttr.u32PicHeight = u32Height;
    venc_chn_attr.stVencAttr.u32VirWidth = u32Width;
    venc_chn_attr.stVencAttr.u32VirHeight = u32Height;
    venc_chn_attr.stVencAttr.u32Profile = 77; // main profile

    ret = RK_MPI_VENC_CreateChn(vencChn, &venc_chn_attr);
    if (ret) {
        printf("ERROR: create VENC[%d] error! ret=%d\n", vencChn, ret);
        return -1;
    }
    return 0;
}

int create_vi(RK_U32 u32Width, RK_U32 u32Height, const RK_CHAR *pVideoNode, RK_S32 viChn) {
    RK_S32 ret = 0;

    printf("init mpi\n");

    VI_CHN_ATTR_S vi_chn_attr;
    vi_chn_attr.pcVideoNode = pVideoNode;
    vi_chn_attr.u32BufCnt = 3;
    vi_chn_attr.u32Width = u32Width;
    vi_chn_attr.u32Height = u32Height;
    vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
    vi_chn_attr.enBufType = VI_CHN_BUF_TYPE_MMAP;
    vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
    ret = RK_MPI_VI_SetChnAttr(0, viChn, &vi_chn_attr);
    ret |= RK_MPI_VI_EnableChn(s32CamId, viChn);
    if (ret) {
        printf("ERROR: create VI[%d] error! ret=%d\n", viChn, ret);
        return -1;
    }
    return 0;
}

// 一共两种方式获取数据流
// 1. 是直接绑定， 然后使用回调
// 2. 是start stream，然后getbuffer
// 下面是第一种方式
int bind_vi_venc() {

    // 直接把VI的数据流绑定到VENC上， 通道号都是1
    MPP_CHN_S stSrcChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = s32CamId;
    stSrcChn.s32ChnId = DRAW_RESULT_BOX_CHN_INDEX;

    MPP_CHN_S stDestChn;
    stDestChn.enModId = RK_ID_VENC;
    stDestChn.s32ChnId = DRAW_RESULT_BOX_CHN_INDEX;

    RK_S32 ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
        printf("ERROR: Bind VI[0] and VENC[0] error! ret=%d\n", ret);
        return -1;
    }
    return 0;
}

// 注册编码的回调，当编码产生数据之后就会回调这个函数
// 与RK_MPI_SYS_GetMediaBuffer相比，无需缓存buffer等待用户索取，因此更节省内存。
int reg_venc_cb() {

    MPP_CHN_S stVenChn;
    memset(&stVenChn, 0, sizeof(MPP_CHN_S));
    stVenChn.enModId = RK_ID_VENC;
    stVenChn.s32ChnId = DRAW_RESULT_BOX_CHN_INDEX; // 使用monitor进程画框子的那个通道

    RK_S32 ret = RK_MPI_SYS_RegisterOutCb(&stVenChn, venc0_packet_cb);
    if (ret) {
        printf("ERROR: register output callback for VENC[0] error! ret=%d\n", ret);
        return -1;
    }

    return 0;
}

// 利用rtsp库， 将h264数据封装成rtsp流
void init_rtsp() {
    printf("init rtsp\n");
    g_rtsplive = create_rtsp_demo(554);                                                    // 554即rtsp端口号， 返回的是一个rtsp_demo_handle指针
    g_rtsp_session_c0 = rtsp_new_session(g_rtsplive, "/live/main_stream");           // 创建rtsp会话， 访问地址是rtsp://ip/live/main_stream
    // g_rtsp_session_c1 = rtsp_new_session(g_rtsplive, "/live/sub_stream");         // 创建rtsp会话， 访问地址是rtsp://ip/live/main_stream

    rtsp_set_video(g_rtsp_session_c0, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);      // 设置视频编码格式
    rtsp_sync_video_ts(g_rtsp_session_c0, rtsp_get_reltime(), rtsp_get_ntptime());          // 设置时间戳

#if 0
    rtsp_set_video(g_rtsp_session_c1, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);      // 设置视频编码格式
    rtsp_sync_video_ts(g_rtsp_session_c1, rtsp_get_reltime(), rtsp_get_ntptime());          // 设置时间戳
#endif

}

// 创建rga通道
int create_rga(RK_S32 rgaChn, RK_U32 u32Width, RK_U32 u32Height) {
    RGA_ATTR_S stRgaAttr;
    memset(&stRgaAttr, 0, sizeof(stRgaAttr));
    stRgaAttr.bEnBufPool = RK_TRUE;
    stRgaAttr.u16BufPoolCnt = 3;
    stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
    stRgaAttr.u16Rotaion = 0;
    stRgaAttr.stImgIn.u32X = 0;
    stRgaAttr.stImgIn.u32Y = 0;

    stRgaAttr.stImgIn.u32Width = u32Width;                     // 注意这里是输出的宽高
    stRgaAttr.stImgIn.u32Height = u32Height;
    stRgaAttr.stImgIn.u32HorStride = u32Width;                 // 所谓虚高
    stRgaAttr.stImgIn.u32VirStride = u32Height;

    stRgaAttr.stImgOut.u32X = 0;                               // 输出的起始位置
    stRgaAttr.stImgOut.u32Y = 0;
    stRgaAttr.stImgOut.imgType = IMAGE_TYPE_NV12;              // 输出的格式
    stRgaAttr.stImgOut.u32Width = u32Width;                    // 输出的宽高
    stRgaAttr.stImgOut.u32Height = u32Height;
    stRgaAttr.stImgOut.u32HorStride = u32Width;                // 输出的虚高;
    stRgaAttr.stImgOut.u32VirStride = u32Height;

    RK_S32 ret = RK_MPI_RGA_CreateChn(rgaChn, &stRgaAttr); // 创建rga通道

    if (ret) {
        printf("ERROR: RGA Set Attr: ImgIn:<%u,%u,%u,%u> "
               "ImgOut:<%u,%u,%u,%u>, rotation=%u failed! ret=%d\n",
               stRgaAttr.stImgIn.u32X, stRgaAttr.stImgIn.u32Y,
               stRgaAttr.stImgIn.u32Width, stRgaAttr.stImgIn.u32Height,
               stRgaAttr.stImgOut.u32X, stRgaAttr.stImgOut.u32Y,
               stRgaAttr.stImgOut.u32Width, stRgaAttr.stImgOut.u32Height,
               stRgaAttr.u16Rotaion, ret);
        return -1;
    }
    return 0;
}

// 创建rga通道
int create_rga_rknn(RK_S32 rgaChn, RK_U32 u32WidthIn, RK_U32 u32HeightIn, RK_U32 u32WidthOut, RK_U32 u32HeightOut) {

    RGA_ATTR_S stRgaAttr;
    memset(&stRgaAttr, 0, sizeof(stRgaAttr));
    stRgaAttr.bEnBufPool = RK_TRUE;
    stRgaAttr.u16BufPoolCnt = 3;
    stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
    stRgaAttr.u16Rotaion = 0;
    stRgaAttr.stImgIn.u32X = (u32WidthIn - u32WidthOut) / 2; // 输入的起始位置， 例如输入如果是1920x1080， 输出是640x640， 那么输入的起始位置就是(1920-640)/2=640
    stRgaAttr.stImgIn.u32Y = (u32HeightIn - u32HeightOut) / 2;

    stRgaAttr.stImgIn.u32Width = u32WidthOut; // 注意这里是输出的宽高
    stRgaAttr.stImgIn.u32Height = u32HeightOut;
    stRgaAttr.stImgIn.u32HorStride = u32WidthIn; // 所谓虚高
    stRgaAttr.stImgIn.u32VirStride = u32HeightIn;

    stRgaAttr.stImgOut.u32X = 0; // 输出的起始位置
    stRgaAttr.stImgOut.u32Y = 0;
    stRgaAttr.stImgOut.imgType = IMAGE_TYPE_NV12; // 输出的格式
    stRgaAttr.stImgOut.u32Width = u32WidthOut;    // 输出的宽高
    stRgaAttr.stImgOut.u32Height = u32HeightOut;
    stRgaAttr.stImgOut.u32HorStride = u32WidthOut; // 输出的虚高;
    stRgaAttr.stImgOut.u32VirStride = u32HeightOut;
    RK_S32 ret = RK_MPI_RGA_CreateChn(rgaChn, &stRgaAttr); // 创建rga通道

    if (ret) {
        printf("ERROR: RGA Set Attr: ImgIn:<%u,%u,%u,%u> "
               "ImgOut:<%u,%u,%u,%u>, rotation=%u failed! ret=%d\n",
               stRgaAttr.stImgIn.u32X, stRgaAttr.stImgIn.u32Y,
               stRgaAttr.stImgIn.u32Width, stRgaAttr.stImgIn.u32Height,
               stRgaAttr.stImgOut.u32X, stRgaAttr.stImgOut.u32Y,
               stRgaAttr.stImgOut.u32Width, stRgaAttr.stImgOut.u32Height,
               stRgaAttr.u16Rotaion, ret);
        return -1;
    }
    return 0;
}

// 将vi绑定到vga
int bind_vi_rga(RK_S32 cameraId, RK_U32 viChnId, RK_U32 rgaChnId) {

    RK_S32 ret = 0;

    MPP_CHN_S stSrcChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = cameraId;
    stSrcChn.s32ChnId = viChnId;

    MPP_CHN_S stDestChn;
    stDestChn.enModId = RK_ID_RGA;
    stDestChn.s32ChnId = rgaChnId;

    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret != 0) {
        printf("ERROR: Bind vi[%d] and rga[%d] failed! ret=%d\n", viChnId, rgaChnId, ret);
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {

    RK_U32 u32Width = 1920;
    RK_U32 u32Height = 1080;

    signal(SIGINT, sig_proc);

    int ret = 0;
    if (argc < 3) {
        printf("please input model name and conf\n");
        return -1;
    }

    float f = atof(argv[2]);
    set_conf(f);
    printf("conf:%f\n", f);

    // yoloModelFilePath = argv[1];
    // 初始化模型
    ret = init_model(argc, argv);
    if (ret < 0) {
        printf("init model failed\n");
        return -1;
    }

    printf("xml dirpath: %s\n\n", pIqfilesPath);
    printf("#bMultictx: %d\n\n", bMultictx);

    // 创建rknn的队列，用于线程间保存rknn的推理结果
    create_rknn_list(&rknn_list_);

    // 初始化isp
    init_isp();

    // 初始化rtsp
    init_rtsp();

    // 系统初始化
    RK_MPI_SYS_Init();

    VI_CHN_ATTR_S vi_chn_attr;
    vi_chn_attr.pcVideoNode = "rkispp_scale0";
    vi_chn_attr.u32BufCnt = 3;
    vi_chn_attr.u32Width = u32Width;
    vi_chn_attr.u32Height = u32Height;
    vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
    vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
    vi_chn_attr.enBufType = VI_CHN_BUF_TYPE_MMAP;
    ret = RK_MPI_VI_SetChnAttr(s32CamId, ONLY_ONE_VI_CHN, &vi_chn_attr);
    ret |= RK_MPI_VI_EnableChn(s32CamId, ONLY_ONE_VI_CHN);
    if (ret) {
        printf("Create VI[0] failed! ret=%d\n", ret);
        return -1;
    }

#if 0
    // vi chn 0, node is rkispp_scale0
    ret = create_vi(u32Width, u32Height, "rkispp_scale0", 0);
    if (ret < 0) {
        printf("create_vi %d failed\n", 0);
        return -1;
    }
#endif

    // 创建rga通道，通道号为1
    ret = create_rga(MONITOR_CHN, u32Width, u32Height);
    if (ret < 0) {
        printf("create_rga_rknn failed ret:%d\n", ret);
        return -1;
    }

    // 创建rga通道，通道号为1
    ret = create_rga_rknn(RKNN_CHN, u32Width, u32Height, MODEL_INPUT_SIZE, MODEL_INPUT_SIZE);
    if (ret < 0) {
        printf("create_rga_rknn failed ret:%d\n", ret);
        return -1;
    }

    // 把rga chn1绑定到vi chn1
    ret = bind_vi_rga(s32CamId, ONLY_ONE_VI_CHN, MONITOR_CHN);
    if (ret < 0) {
        printf("ERROR: bind vi 0 to rga 0 failed\n");
        return -1;
    }

    ret = bind_vi_rga(s32CamId, ONLY_ONE_VI_CHN, RKNN_CHN);
    if (ret < 0) {
        printf("ERROR: bind vi 0 to rga 1 failed\n");
        return -1;
    }

    MPP_CHN_S stRgaChnInfo;
    memset(&stRgaChnInfo, 0, sizeof(MPP_CHN_S));
    stRgaChnInfo.enModId = RK_ID_RGA;
    stRgaChnInfo.s32ChnId = RKNN_CHN;

    ret = RK_MPI_SYS_RegisterOutCb(&stRgaChnInfo, rknn_rga_packet_cb);
    if (ret) {
        printf("ERROR: register output callback for VENC[0] error! ret=%d\n", ret);
        return -1;
    }

    stRgaChnInfo.s32ChnId = MONITOR_CHN;
    ret = RK_MPI_SYS_RegisterOutCb(&stRgaChnInfo, monitor_rga_packet_cb);
    if (ret) {
        printf("ERROR: register output callback for VENC[0] error! ret=%d\n", ret);
        return -1;
    }

    // 创建venc通道，通道号为0
    ret = create_venc_chn(30, u32Width, u32Height, MONITOR_CHN);
    if (ret < 0) {
        printf("ERROR: create_venc_chn error%d\n", 0);
        return -1;
    }

    MPP_CHN_S stVenChn;
    memset(&stVenChn, 0, sizeof(MPP_CHN_S));
    stVenChn.enModId = RK_ID_VENC;
    stVenChn.s32ChnId = MONITOR_CHN;

    ret = RK_MPI_SYS_RegisterOutCb(&stVenChn, monitor_venc_packet_cb);
    if (ret) {
        printf("ERROR: register output callback for VENC[0] error! ret=%d\n", ret);
        return -1;
    }

#if 0
    // 创建venc通道，通道号为1
    ret = create_venc_chn(30, u32Width, u32Height, 1);
    if (ret < 0) {
        printf("ERROR: create_venc_chn %d error\n", 1);
        return -1;
    }


    MPP_CHN_S stSrcChn;
    stSrcChn.enModId = RK_ID_RGA;
    stSrcChn.s32ChnId = 0;

    MPP_CHN_S stDestChn;
    stDestChn.enModId = RK_ID_VENC;
    stDestChn.s32ChnId = 0;

    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret != 0) {
        printf("ERROR: Bind rga[%d] and venc[%d] failed! ret=%d\n", 0, 0, ret);
        return -1;
    }

    stSrcChn.s32ChnId = 1;
    stDestChn.s32ChnId = 1;

    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret != 0) {
        printf("ERROR: Bind rga[%d] and venc[%d] failed! ret=%d\n", 0, 1, ret);
        return -1;
    }

    // bind_vi_venc();
    // 打开vi通道０的输出，即开始从isp获取数据
    ret = RK_MPI_VI_StartStream(s32CamId, 0);
    if (ret < 0) {
        printf("RK_MPI_VI_StartStream 0 failed\n");
        return -1;
    }

    ret = RK_MPI_VI_StartStream(s32CamId, 1);
    if (ret < 0) {
        printf("RK_MPI_VI_StartStream 1 failed\n");
        return -1;
    }


    MPP_CHN_S stVenChn;
    memset(&stVenChn, 0, sizeof(MPP_CHN_S));
    stVenChn.enModId = RK_ID_VENC;
    stVenChn.s32ChnId = 0;

    ret = RK_MPI_SYS_RegisterOutCb(&stVenChn, venc0_packet_cb);
    if (ret) {
        printf("ERROR: register output callback for VENC[0] error! ret=%d\n", ret);
        return -1;
    }

    stVenChn.s32ChnId = 1;
    ret = RK_MPI_SYS_RegisterOutCb(&stVenChn, venc1_packet_cb);
    if (ret) {
        printf("ERROR: register output callback for VENC[1] error! ret=%d\n", ret);
        return -1;
    }

    // 从vi中获取数据，然后那推理结果，画框子，然后送进编码器
    // pthread_create(&venc0_thread_id, NULL, venc0_thread, NULL);
    // pthread_create(&venc1_thread_id, NULL, venc1_thread, NULL);
#endif

    getchar();
    getchar();

#if 0
    while (g_flag_run) {
        usleep(1000);
    }
#endif

    // 清洁工作
    destroy_all();

    return 0;

}

// rtsp pipe: scale0 -> vi chn 0 -> RK_MPI_SYS_GetMediaBuffer(RK_ID_VI）from vi chn0 -> 获取rknn的推测结果队列 -> 画框子 -> RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC) to venc chn 0
// venc 的回调 -> rtsp
// rknn pipe: scale1 -> vi chn 1 1920x1080 -> rga chn 1 640x640 -> RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA）转成RGB，推理之后，放入队列
int main_bak(int argc, char **argv) {

    RK_U32 u32Width = RTSP_INPUT_VI_WIDTH;
    RK_U32 u32Height = RTSP_INPUT_VI_HEIGHT;

    // GC2053 只能用 1920*1080
    // RK_U32 u32VC1Width = 1920;
    // RK_U32 u32VC1Height = 1080;

    // imx415 可以使用 1280*720
    RK_U32 u32VC1Width = RTSP_INPUT_VI_WIDTH;    // RTSP_INPUT_VI_WIDTH;
    RK_U32 u32VC1Height = RTSP_INPUT_VI_HEIGHT;  // RTSP_INPUT_VI_HEIGHT;

    // cannot use 1920*1080 on both vi input node: rkispp_scale0, rkispp_scale1

    signal(SIGINT, sig_proc);

    int ret = 0;
    if (argc < 3) {
        printf("please input model name and conf\n");
        return -1;
    }

    float f = atof(argv[2]);
    set_conf(f);
    printf("conf:%f\n", f);

    // yoloModelFilePath = argv[1];
    // 初始化模型
    ret = init_model(argc, argv);
    if (ret < 0) {
        printf("init model failed\n");
        return -1;
    }

    printf("xml dirpath: %s\n\n", pIqfilesPath);
    printf("#bMultictx: %d\n\n", bMultictx);

    // 初始化isp
    init_isp();

    // 初始化rtsp
    init_rtsp();

    // 系统初始化
    RK_MPI_SYS_Init();

    // 分别为rknn跟monitor创建vi通道，通道号分别为0跟1
    ret = create_vi(u32Width, u32Height, "rkispp_scale0", DRAW_RESULT_BOX_CHN_INDEX);
    if (ret < 0) {
        printf("create_vi %d failed\n", DRAW_RESULT_BOX_CHN_INDEX);
        return -1;
    }

    ret = create_vi(u32VC1Width, u32VC1Height, "rkispp_scale1", RK_NN_RGA_CHN_INDEX);
    if (ret < 0) {
        printf("create_vi %d cb\n", RK_NN_RGA_CHN_INDEX);
        return -1;
    }

    // 创建rga通道，通道号为1
    ret = create_rga_rknn(RK_NN_RGA_CHN_INDEX, u32VC1Width, u32VC1Height,
                          MODEL_INPUT_SIZE, MODEL_INPUT_SIZE);
    if (ret < 0) {
        printf("create_rga_rknn failed ret:%d\n", ret);
        return -1;
    }

    // 把rga chn1绑定到vi chn1
    bind_vi_rga(s32CamId, RK_NN_RGA_CHN_INDEX, RK_NN_RGA_CHN_INDEX);

    // 创建venc通道，通道号为0
    ret = create_venc_chn(30, u32Width, u32Height, DRAW_RESULT_BOX_CHN_INDEX);
    if (ret < 0) {
        printf("create_venc_chn %d\n", DRAW_RESULT_BOX_CHN_INDEX);
        return -1;
    }

    // 注册venc的回调函数，即产生了编码好的数据，就会回调这个函数
    ret = reg_venc_cb();
    if (ret < 0) {
        printf("reg venc cb\n");
        return -1;
    }

    // bind_vi_venc();
    // 打开vi通道０的输出，即开始从isp获取数据
    ret = RK_MPI_VI_StartStream(s32CamId, DRAW_RESULT_BOX_CHN_INDEX);
    if (ret < 0) {
        printf("RK_MPI_VI_StartStream failed\n");
        return -1;
    }

    // 创建rknn的队列，用于线程间保存rknn的推理结果
    create_rknn_list(&rknn_list_);

    // 创建推理线程
    pthread_create(&rknn_yolo_thread_id, NULL, rknn_yolo_thread, NULL);

    // 从vi中获取数据，然后那推理结果，画框子，然后送进编码器
    pthread_create(&observer_thread_id, NULL, observer_thread, NULL);

    while (g_flag_run) {
        usleep(1000);
    }

    // 清洁工作
    destroy_all();

    return 0;
}
