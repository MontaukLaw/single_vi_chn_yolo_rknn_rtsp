#include "h264_venc.h"
#include "rtsp_demo.h"
#include "user_define.h"
#include "comm.h"

extern rtsp_demo_handle g_rtsplive;
extern rtsp_session_handle g_rtsp_session_c0;
rtsp_session_handle g_rtsp_session_c1;

static pthread_mutex_t mp4_mutex = PTHREAD_MUTEX_INITIALIZER;

#if 0
// wasted
void venc0_packet_cb(MEDIA_BUFFER mb)
{

    static RK_S32 packet_cnt = 0;
    // printf("#Get packet-%d, size %zu\n", packet_cnt, RK_MPI_MB_GetSize(mb));

    if (g_rtsplive && g_rtsp_session_c0)
    {

        // record_mp4(mb);

        rtsp_tx_video(g_rtsp_session_c0, (const uint8_t *)RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetTimestamp(mb));
        rtsp_do_event(g_rtsplive);
    }

    RK_MPI_MB_ReleaseBuffer(mb);
    packet_cnt++;
}

// wasted
// 注册编码的回调，当编码产生数据之后就会回调这个函数
// 与RK_MPI_SYS_GetMediaBuffer相比，无需缓存buffer等待用户索取，因此更节省内存。
int reg_venc_cb()
{

    MPP_CHN_S stVenChn;
    memset(&stVenChn, 0, sizeof(MPP_CHN_S));
    stVenChn.enModId = RK_ID_VENC;
    stVenChn.s32ChnId = DRAW_RESULT_BOX_CHN_INDEX; // 使用monitor进程画框子的那个通道

    RK_S32 ret = RK_MPI_SYS_RegisterOutCb(&stVenChn, venc0_packet_cb);
    if (ret)
    {
        printf("ERROR: register output callback for VENC[0] error! ret=%d\n", ret);
        return -1;
    }

    return 0;
}
#endif

// 利用rtsp库， 将h264数据封装成rtsp流
void init_rtsp()
{
    printf("init rtsp\n");
    g_rtsplive = create_rtsp_demo(8554);                                   // 554即rtsp端口号， 返回的是一个rtsp_demo_handle指针
    g_rtsp_session_c0 = rtsp_new_session(g_rtsplive, "/live/main_stream"); // 创建rtsp会话， 访问地址是rtsp://ip/live/main_stream
    // g_rtsp_session_c1 = rtsp_new_session(g_rtsplive, "/live/sub_stream");        // 创建rtsp会话， 访问地址是rtsp://ip/live/main_stream

    rtsp_set_video(g_rtsp_session_c0, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);          // 设置视频编码格式
    rtsp_sync_video_ts(g_rtsp_session_c0, rtsp_get_reltime(), rtsp_get_ntptime()); // 设置时间戳

#if 0
    rtsp_set_video(g_rtsp_session_c1, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);           // 设置视频编码格式
    rtsp_sync_video_ts(g_rtsp_session_c1, rtsp_get_reltime(), rtsp_get_ntptime());  // 设置时间戳
#endif
}

// 用于监控的编码的回调
// real thing
void monitor_venc_packet_cb(MEDIA_BUFFER mb)
{

    static RK_S32 packet_cnt = 0;
    // printf("#Get packet-%d, size %zu\n", packet_cnt, RK_MPI_MB_GetSize(mb));
    if (g_rtsplive && g_rtsp_session_c0)
    {

        rtsp_tx_video(g_rtsp_session_c0, (const uint8_t *)RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetTimestamp(mb));
        rtsp_do_event(g_rtsplive);
    }

    // 计算一下帧率
    // static int64_t last_time = 0;
    // int64_t cur_time = get_current_time_us();
    // printf("fps: %.2f\n", 1000000.0 / (cur_time - last_time));
    // last_time = cur_time;

    // pthread_mutex_lock(&mp4_mutex);
    // 录制mp4
    record_mp4(mb);
    // pthread_mutex_unlock(&mp4_mutex);

    RK_MPI_MB_ReleaseBuffer(mb);
    packet_cnt++;
}

// wasted
void venc1_packet_cb(MEDIA_BUFFER mb)
{

    static RK_S32 packet_cnt = 0;

    // printf("#Get packet-%d, size %zu\n", packet_cnt, RK_MPI_MB_GetSize(mb));

    if (g_rtsplive && g_rtsp_session_c1)
    {
        rtsp_tx_video(g_rtsp_session_c1, (const uint8_t *)RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetTimestamp(mb));
        rtsp_do_event(g_rtsplive);
    }

    RK_MPI_MB_ReleaseBuffer(mb);
    packet_cnt++;
}