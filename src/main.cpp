#include "comm.h"


int g_flag_run = 1;

static pthread_t observer_thread_id;
static pthread_t rknn_yolo_thread_id;

static pthread_t get_rga_buffer_thread_id;
rknn_list *rknn_list_;

RK_S32 s32CamId = 0;
rtsp_demo_handle g_rtsplive = NULL;
rtsp_session_handle g_rtsp_session_c0;

extern g_params gParams;

long int get_time_now_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

long int get_time_now_sec(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

static void save_file(char *fileName, void *data, int dataLen)
{

    FILE *save_file = fopen(fileName, "w");
    if (!save_file)
    {
        printf("ERROR: Open file failed!\n");
    }

    fwrite(data, 1, dataLen, save_file);
    fclose(save_file);
}

#if 0
// 初始化isp
static void init_isp(void)
{
    SAMPLE_COMM_ISP_Init(s32CamId, hdr_mode, bMultictx, pIqfilesPath);

    SAMPLE_COMM_ISP_Run(s32CamId);
    SAMPLE_COMM_ISP_SetFrameRate(s32CamId, FPS);

    if (gParams.defog_level > 0)
    {
        // 打开除雾 1表示打开手动, 2是自动模式
        SAMPLE_COMM_ISP_SET_DefogEnable(s32CamId, 1);
        // 1表示手动模式, 255是最大值
        SAMPLE_COMM_ISP_SET_DefogStrength(s32CamId, 1, gParams.defog_level);
    }
    else
    {
        SAMPLE_COMM_ISP_SET_DefogEnable(s32CamId, RK_FALSE);
    }

    if (gParams.fec_level)
    {
        rk_aiq_uapi_setFecCorrectLevel(s32CamId, gParams.fec_level);
    }
}
#endif


static void sig_proc(int signo)
{
    fprintf(stderr, "signal %d\n", signo);
    g_flag_run = 0;
}

static void destroy_all(void)
{

    close_mp4_file_saving();

    if (observer_thread_id)
    {
        pthread_join(observer_thread_id, nullptr);
    }

    if (get_rga_buffer_thread_id)
    {
        pthread_join(get_rga_buffer_thread_id, nullptr);
    }

    if (rknn_yolo_thread_id)
    {
        pthread_join(rknn_yolo_thread_id, nullptr);
    }

    rtsp_del_demo(g_rtsplive);

    unbind_vi_rga(s32CamId, DRAW_RESULT_BOX_CHN_INDEX, MONITOR_RGA_CHN);
    unbind_vi_rga(s32CamId, RK_NN_RGA_CHN_INDEX, RKNN_RGA_CHN);

    RK_MPI_VI_DisableChn(s32CamId, DRAW_RESULT_BOX_CHN_INDEX);
    RK_MPI_VI_DisableChn(s32CamId, RK_NN_RGA_CHN_INDEX);

    // RK_MPI_VENC_DestroyChn(cfg.session_cfg[1].stVenChn.s32ChnId);
    RK_MPI_RGA_DestroyChn(MONITOR_RGA_CHN);
    RK_MPI_RGA_DestroyChn(RKNN_RGA_CHN);

    SAMPLE_COMM_ISP_Stop(s32CamId);

    destory_rknn_list(&rknn_list_);
}

// H264 codec
static int create_venc_chn(RK_U32 fps, RK_U32 u32Width, RK_U32 u32Height, RK_U32 vencChn)
{

    VENC_CHN_ATTR_S venc_chn_attr;
    memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));

    int ret = 0;
    venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
    venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = fps;
    venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = u32Width * u32Height * 3;

    // frame rate: in 30/1, out 30/1.
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = fps;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
    venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = fps;

    venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_RGB888; // IMAGE_TYPE_RGB888; // IMAGE_TYPE_NV12;
    venc_chn_attr.stVencAttr.u32PicWidth = u32Width;
    venc_chn_attr.stVencAttr.u32PicHeight = u32Height;
    venc_chn_attr.stVencAttr.u32VirWidth = u32Width;
    venc_chn_attr.stVencAttr.u32VirHeight = u32Height;
    venc_chn_attr.stVencAttr.u32Profile = 77; // main profile

    ret = RK_MPI_VENC_CreateChn(vencChn, &venc_chn_attr);
    if (ret)
    {
        printf("ERROR: create VENC[%d] error! ret=%d\n", vencChn, ret);
        return -1;
    }
    return 0;
}

int main(void)
{
    int ret = 0;

    readConfig("config.ini");

    RK_U32 u32Width = 1920;
    RK_U32 u32Height = 1080;

    // GC2053 只能用 1920*1080
    // RK_U32 u32VC1Width = 1920;
    // RK_U32 u32VC1Height = 1080;

    // imx415 可以使用 1280*720
    RK_U32 u32RknnWidth = 640;  // RTSP_INPUT_VI_WIDTH;
    RK_U32 u32RknnHeight = 360; // RTSP_INPUT_VI_HEIGHT;

    signal(SIGINT, sig_proc);

    // 初始化模型
    ret = init_model();
    if (ret < 0)
    {
        printf("init model failed\n");
        return -1;
    }

    // 创建rknn的队列，用于线程间保存rknn的推理结果
    create_rknn_list(&rknn_list_);

    // 初始化isp
    init_isp();

    // 初始化rtsp
    init_rtsp();

    // 系统初始化
    RK_MPI_SYS_Init();

    // 分别为rknn跟monitor创建vi通道，通道号分别为0跟1
    ret = create_vi(s32CamId, u32Width, u32Height, "rkispp_scale0", DRAW_RESULT_BOX_CHN_INDEX);
    if (ret < 0)
    {
        printf("create_vi %d failed\n", DRAW_RESULT_BOX_CHN_INDEX);
        return -1;
    }

    ret = create_vi(s32CamId, u32RknnWidth, u32RknnHeight, "rkispp_scale1", RK_NN_RGA_CHN_INDEX);
    if (ret < 0)
    {
        printf("create_vi %d cb\n", RK_NN_RGA_CHN_INDEX);
        return -1;
    }

    // 创建rga通道，通道号为0
    ret = create_rga(MONITOR_RGA_CHN, u32Width, u32Height);
    if (ret < 0)
    {
        printf("create_rga_rknn failed ret:%d\n", ret);
        return -1;
    }

    // 把rga chn0绑定到vi chn0, 用作画框
    ret = bind_vi_rga(s32CamId, DRAW_RESULT_BOX_CHN_INDEX, MONITOR_RGA_CHN);
    if (ret < 0)
    {
        printf("ERROR: bind vi 0 to rga 0 failed\n");
        return -1;
    }

    // 创建rga通道，通道号为1
    ret = create_rga(RKNN_RGA_CHN, u32RknnWidth, u32RknnHeight);
    if (ret < 0)
    {
        printf("create_rga_rknn failed ret:%d\n", ret);
        return -1;
    }

    // 把rga通道RK_NN_RGA_CHN_INDEX绑定到RKNN_CHN
    ret = bind_vi_rga(s32CamId, RK_NN_RGA_CHN_INDEX, RKNN_RGA_CHN);
    if (ret < 0)
    {
        printf("ERROR: bind vi 0 to rga 0 failed\n");
        return -1;
    }

    ret = RK_MPI_VI_StartStream(s32CamId, RK_NN_RGA_CHN_INDEX);
    if (ret < 0)
    {
        printf("RK_MPI_VI_StartStream 0 failed\n");
        return -1;
    }

    ret = RK_MPI_VI_StartStream(s32CamId, DRAW_RESULT_BOX_CHN_INDEX);
    if (ret < 0)
    {
        printf("RK_MPI_VI_StartStream failed\n");
        return -1;
    }

    // 创建venc通道，通道号为0
    ret = create_venc_chn(VIDEO_FPS, u32Width, u32Height, 0);
    if (ret < 0)
    {
        printf("ERROR: create_venc_chn error%d\n", 0);
        return -1;
    }

    // 创建venc通道，通道号为1
    MPP_CHN_S stVenChn;
    memset(&stVenChn, 0, sizeof(MPP_CHN_S));
    stVenChn.enModId = RK_ID_VENC;
    stVenChn.s32ChnId = MONITOR_VENC_CHN;

    ret = RK_MPI_SYS_RegisterOutCb(&stVenChn, monitor_venc_packet_cb);
    if (ret)
    {
        printf("ERROR: register output callback for VENC[0] error! ret=%d\n", ret);
        return -1;
    }

    // 创建推理线程
    pthread_create(&rknn_yolo_thread_id, NULL, rknn_yolo_thread, NULL);

    // 从vi中获取数据，然后那推理结果，画框子，然后送进编码器
    pthread_create(&observer_thread_id, NULL, observer_thread, NULL);

    getchar();
    getchar();
    g_flag_run = 0;

    printf("start to clean\n");
    // 清洁工作
    destroy_all();

    return 0;
}
