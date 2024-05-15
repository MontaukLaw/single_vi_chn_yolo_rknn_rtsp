#include "comm.h"

extern int g_flag_run;
extern rknn_list *rknn_list_;
extern g_params gParams;

// int detectResultNumber = 0;
detect_result_group_t detect_result_group;

void print_fps(void)
{
    static int frameCounter = 0;
    static long int lastTime = 0;
    long int curTime = get_time_now_sec();
    frameCounter++;
    // printf("cur sec:%ld\n", curTime);
    // printf("curTime:%lld lastTime:%lld\n", curTime, lastTime);
    if ((curTime != lastTime))
    {
        printf("rknn_yolo_thread fps:%d\n", frameCounter);
        frameCounter = 0;
        lastTime = curTime;
    }
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

// size:691200
void *rknn_yolo_thread(void *args)
{

    int ret = 0;

    printf("Start get_media_buffer thread \n");
    int fileSaveCounter = 0;
    int printCounter = 0;

    MEDIA_BUFFER buffer = NULL;

    int noBoxCounter = 0;

    // get data from vi
    while (g_flag_run)
    {

        // 计算帧率
        print_fps();

        // 获取rga的数据
        buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA, RKNN_RGA_CHN, -1);
        if (!buffer)
        {
            usleep(1000);
            continue;
        }

        // size:691200
        printCounter++;
        if (printCounter % 30 == 1)
        {
            print_mb_info(buffer);
        }

        if (printCounter == 30)
        {
            printf("Saving buffer to file\n");
            save_file("640x360.rgb", RK_MPI_MB_GetPtr(buffer), RK_MPI_MB_GetSize(buffer));
        }

        // 开始detect
        void *modeInputData = malloc(YOLO_INPUT_SIZE);
        memset(modeInputData, 0x7F, YOLO_INPUT_SIZE);
        
        // 填充数据
        fill_image_data(RK_MPI_MB_GetPtr(buffer), modeInputData, RKNN_INPUT_IMG_WIDTH, RKNN_INPUT_IMG_HEIGHT, MODEL_INPUT_SIZE, MODEL_INPUT_SIZE);

        // detect_result_group_t detect_result_group_;
        // memset(&detect_result_group_, 0, sizeof(detect_result_group_t));

        detect_results_t detect_result_group_;
        memset(&detect_result_group_, 0, sizeof(detect_results_t));

        detect(modeInputData, &detect_result_group_);
        // int pResult = predict(modeInputData, &detect_result_group_);
        // printf("dd= %d,sizeN=%d\n", outMindist.size(), sizeN);

        if (detect_result_group_.count > 0)
        {

            printf("detect_result_group_.count:%d\n", detect_result_group_.count);
            // detect_result_group.detect_count = detect_result_group_.detect_count;
            // detect_result_group.count = detect_result_group_.count;
            for (int i = 0; i < detect_result_group_.count; i++)
            {

                printf("results[%d].name:%s\n", i, detect_result_group_.results[i].name);
                printf("results[%d].prop:%f\n", i, detect_result_group_.results[i].prop);
                printf("results[%d].box.left:%d\n", i, detect_result_group_.results[i].box.left);
                printf("results[%d].box.right:%d\n", i, detect_result_group_.results[i].box.right);
                printf("results[%d].box.top:%d\n", i, detect_result_group_.results[i].box.top);
                printf("results[%d].box.bottom:%d\n", i, detect_result_group_.results[i].box.bottom);
                printf("results[%d].class_index:%d\n", i, detect_result_group_.results[i].class_index);

                // detect_result_group.results[i].class_index = detect_result_group_.results[i].class_index;
                // detect_result_group.results[i].prop = detect_result_group_.results[i].prop;
                // detect_result_group.results[i].box.left = detect_result_group_.results[i].box.left;
                // detect_result_group.results[i].box.right = detect_result_group_.results[i].box.right;
                // detect_result_group.results[i].box.top = detect_result_group_.results[i].box.top;
                // detect_result_group.results[i].box.bottom = detect_result_group_.results[i].box.bottom;
            }

            // 数据搬运过去
            memset(&detect_result_group, 0, sizeof(detect_result_group_t));
            detect_result_group.count = detect_result_group_.count;
            for (int i = 0; i < detect_result_group_.count; i++)
            {
                // int name_len =  sizeof(detect_result_group_.results[i].name);
                // printf("size of name:%d\n", name_len);

                // strcpy(detect_result_group.results[i].name, detect_result_group_.results[i].name);
                memcpy(detect_result_group.results[i].name, detect_result_group_.results[i].name, OBJ_NAME_MAX_SIZE);
                detect_result_group.results[i].class_index = detect_result_group_.results[i].class_index;
                detect_result_group.results[i].prop = detect_result_group_.results[i].prop;
                detect_result_group.results[i].box.left = detect_result_group_.results[i].box.left;
                detect_result_group.results[i].box.right = detect_result_group_.results[i].box.right;
                detect_result_group.results[i].box.top = detect_result_group_.results[i].box.top;
                detect_result_group.results[i].box.bottom = detect_result_group_.results[i].box.bottom;
            }

            std::vector<_outMinDitsReuslt> outMindist;
            outMindist.resize(0);

            int sizeN = 0;
            sizeN = calDistance(&detect_result_group, outMindist);

            // memcpy(&detect_result_group, &detect_result_group_, sizeof(detect_result_group_t));
            noBoxCounter = 0;
        }
        else
        {
            noBoxCounter++;
            if (noBoxCounter >= gParams.clean_old_box_frames)
            {
                memset(&detect_result_group, 0, sizeof(detect_result_group_t));
                noBoxCounter = 0;
            }
        }

        // put detect result to list
        // detectResultNumber = detect_result_group.detect_count;
        // if (detect_result_group.detect_count > 0)
        // {
        // rknn_list_push(rknn_list_, get_current_time_ms(), detect_result_group);
        // int size = rknn_list_size(rknn_list_);
        // if (size >= MAX_RKNN_LIST_NUM)
        // {
        // rknn_list_drop(rknn_list_);
        // }
        // printf("result size: %d\n", size);
        // }

        free(modeInputData);
        // print_mb_info(buffer);
        // usleep(100000);
        RK_MPI_MB_ReleaseBuffer(buffer);
    }

    return nullptr;
}

void print_mb_info(MEDIA_BUFFER buffer)
{
    int cnt = 0;
    printf("#%d Get Frame:ptr:%p, size:%zu, mode:%d, channel:%d, timestamp:%lld\n",
           cnt++, RK_MPI_MB_GetPtr(buffer), RK_MPI_MB_GetSize(buffer),
           RK_MPI_MB_GetModeID(buffer), RK_MPI_MB_GetChannelID(buffer),
           RK_MPI_MB_GetTimestamp(buffer));
}
