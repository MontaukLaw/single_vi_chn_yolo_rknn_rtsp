#include "mp4_process.h"

int recordingVideo = 0;
MP4FileHandle hMP4File = NULL;
// pthread_mutex_t mp4_mutex = PTHREAD_MUTEX_INITIALIZER;

int g_file_number = 200;
// extern int g_seconds_per_file;
// extern bool recordingAudio;

static int spsflag = 0;
static int ppsflag = 0;
static int mp4_file_inited = 0;
static MP4TrackId videoTrack = 0;

extern g_params gParams;

void create_mp4_file(void)
{
    char file_full_name[200] = {0};
    int total_files_so_far = count_my_mp4_file_number();

    printf("total_files_so_far = %d\n", total_files_so_far);

    char file_name_buf[100];

    while (total_files_so_far >= g_file_number)
    {

        find_oldest_file_name_prefix(file_name_buf);

        // 删除最老的那个文件
        printf("delete oldest file %s\n", file_name_buf);

        remove(file_name_buf);

        total_files_so_far = count_my_mp4_file_number();
    }

    // 获取文件存储路径为
    get_file_name_by_time(file_name_buf);

    // 创建mp4文件
    // sprintf(file_full_name, "/mnt/hisiv/test.mp4", total_files_so_far);
    hMP4File = MP4CreateEx(file_name_buf, 0, 1, 1, 0, 0, 0, 0);
    if (hMP4File == MP4_INVALID_FILE_HANDLE)
    {
        printf("open file fialed.\n");
        return;
    }

    MP4SetTimeScale(hMP4File, VIDEO_TRACK_TIME_SCALE);

    mp4_file_inited = 1;
}

void close_mp4_file_saving(void)
{
    recordingVideo = 0;
    printf("Close mp4 file\n");
    if (hMP4File != NULL)
    {
        MP4Close(hMP4File, 0);
    }
}

// 创建mp4文件
void reset_new_mp4_file(void)
{

    close_mp4_file_saving();

    // hMP4File = NULL;
    create_mp4_file();

    ppsflag = 0;
}

void init_video_audio_track(uint8_t *pData)
{

    // printf("init video audio track\n");
    // printf("Write sps =================\n");
    // 把视频的轨道写入文件, 分辨率为1920x1080
    // 第二个参数TimeScale的值是90000（也可以是别的值，在计算上下帧对应的ticks值时，需要使用这个刻度基准）。
    // 第三个参数值采用MP4_INVALID_DURATION，意思是我们使用变化的ticks，也就是函数MP4WriteSample里的第5个参数。
    // 如果帧与帧间隔值比较固定，也可以把第三个参数值写成固定的对应ticks，比如每秒固定有25帧数据，那么第三个参数值可以是： duration= (1/25)秒/(1/90000) = 36000;
    // duration = (timestamp-lasttimestamp)/(1/timescale)
    // sample_duration 就是sample_delta
    videoTrack = MP4AddH264VideoTrack(hMP4File, VIDEO_TRACK_TIME_SCALE, VIDEO_TRACK_TIME_SCALE / VIDEO_FPS,
                                      RTSP_INPUT_VI_WIDTH,
                                      RTSP_INPUT_VI_HEIGHT,
                                      pData[FRAME_START_FLAG_LEN + 1], // sps[1] AVCProfileIndication
                                      pData[FRAME_START_FLAG_LEN + 2], // sps[2] profile_compat
                                      pData[FRAME_START_FLAG_LEN + 3], // sps[3] AVCLevelIndication
                                      3);                              // 4 bytes length before each NAL unit

    printf("videoTrack = %d\n", videoTrack);
    MP4SetVideoProfileLevel(hMP4File, 0x7F);

    // 初始化音频音轨
    // init_audio_track();
}

RK_S32 record_mp4(MEDIA_BUFFER mb)
{

    // 如果不保存mp4, 直接退出
    if (gParams.save_mp4 == false)
    {
        return 0;
    }
    // printf("Record mp4\n");
    static int passFrameCounter = 0;
    if (passFrameCounter < PASS_FRAME_NUM)
    {
        passFrameCounter++;
        return 0;
    }

    // printf("passFrameCounter = %d\n", passFrameCounter);

    static int finishFileTimeCounter = 0;
    static int nRecordFlag = 0x00;

    static bool ifFoundSpsPps = false;
    static bool ifFoundSEI = false;
    static RK_S32 packet_cnt = 0;
    // static int64_t elapse_us = 0;

    int j = 0;
    int len = 0;
    uint8_t *pData = NULL;
    // char isSyncSample = 0;
    char isSyncSample = 1;

    RK_S32 frameType = RK_MPI_MB_GetFlag(mb);

    // printf("Get packet type: %d, size %zu\n", frameType, RK_MPI_MB_GetSize(mb));

    // elapse_us = get_current_time_us();

    // printf("1 frame create time: %.3fms\n", elapse_us / 1000.0);
    // len = stStream->pstPack[j].u32Len - stStream->pstPack[j].u32Offset;
    // pData = (stStream->pstPack[j].pu8Addr + stStream->pstPack[j].u32Offset);
    len = RK_MPI_MB_GetSize(mb);
    pData = (uint8_t *)RK_MPI_MB_GetPtr(mb);

    if (VENC_NALU_IDRSLICE == frameType)
    {
        printf("VENC_NALU_IDRSLICE len:%d\n", len);
        if (pData[0] == 0 && pData[1] == 0 && pData[2] == 0 && pData[3] == 0x01 && pData[4] == 0x67)
        {
            printf("SPS\n");
        }
        else
        {
            printf("IDR frame 0x%02x 0x%02x\n", pData[3], pData[4]);
        }
    }
    else if (frameType == VENC_NALU_PSLICE)
    {
        printf("VENC_NALU_PSLICE len:%d\n", len);
    }

    if (VENC_NALU_IDRSLICE == frameType)
    {
        // printf("IDR frame 0x%02x 0x%02x\n", pData[3], pData[4]);
        if (pData[0] == 0 && pData[1] == 0 && pData[2] == 0 && pData[3] == 0x01 && pData[4] == 0x67)
        {
            // printf("SPS\n");
            finishFileTimeCounter++;
            // 第一次找到sps, pps, sei, 就创建mp4文件
            // 或者是时间到了, 就创建mp4文件
            if (finishFileTimeCounter >= gParams.mp4_length_sec || spsflag == 0x00)
            {
                // printf("%d second time up, close old one, create a new mp4 file\n", gParams.mp4_length_sec);
                printf("Close old one, create a new mp4 file\n");

                // 重置mp4文件, 创建新的
                reset_new_mp4_file();

                // 用于同步音频写入
                recordingVideo = 1;

                spsflag = 0x1;

                // 初始化视频音轨
                init_video_audio_track(pData);

                pData = pData + FRAME_START_FLAG_LEN;

                // 把sps信息写入到文件中 SPS 26字节
                MP4AddH264SequenceParameterSet(hMP4File, videoTrack, pData, SPS_FRAME_LEN);

                pData = pData + SPS_FRAME_LEN + FRAME_START_FLAG_LEN;

                finishFileTimeCounter = 0;

                // 写pps信息写入到文件中 PPS 4字节
                MP4AddH264PictureParameterSet(hMP4File, videoTrack, pData, PPS_FRAME_LEN);

                // pData = pData + PPS_FRAME_LEN + FRAME_START_FLAG_LEN;
                pData = pData + PPS_FRAME_LEN;
                len = len - PPS_FRAME_LEN - FRAME_START_FLAG_LEN - SPS_FRAME_LEN;
            }

            pData[0] = (len - 4) >> 24;
            pData[1] = (len - 4) >> 16;
            pData[2] = (len - 4) >> 8;
            pData[3] = len - 4;

            // printf("MP4WriteSample len = %d\n", len);
            MP4WriteSample(hMP4File, videoTrack, (const uint8_t *)pData, len, MP4_INVALID_DURATION, 0, true);
        }
    }
    else if (frameType == VENC_NALU_PSLICE)
    {

        // printf("VENC_NALU_PSLICE \n");

        pData[0] = (len - 4) >> 24;
        pData[1] = (len - 4) >> 16;
        pData[2] = (len - 4) >> 8;
        pData[3] = len - 4;

        MP4WriteSample(hMP4File, videoTrack, (const uint8_t *)pData, len, MP4_INVALID_DURATION, 0, false);
    }
    else
    {
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Unknown frame type\n");
    }
}