#include "monitor.h"
#include "mp4_process.h"

using namespace cv;

extern g_params gParams;
extern detect_result_group_t detect_result_group;
extern int g_flag_run;
extern rknn_list *rknn_list_;
extern int boxInfoListNumber;
extern int boxDisplayCounterDown;

box_info_t boxInfoList[OBJ_NUMB_MAX_SIZE];

// 画虚线
void drawDashedLine(cv::Mat &img, cv::Point pt1, cv::Point pt2,
                    const cv::Scalar &color, int thickness = 1, int lineType = cv::LINE_8,
                    int dashLength = 10, int gapLength = 10)
{
    double distance = sqrt(pow(pt2.x - pt1.x, 2) + pow(pt2.y - pt1.y, 2));
    double dashGapTotalLength = dashLength + gapLength;
    int dashes = distance / dashGapTotalLength;

    for (int i = 0; i < dashes; ++i)
    {
        cv::Point start = pt1 + (pt2 - pt1) * ((i * dashGapTotalLength) / distance);
        cv::Point end = pt1 + (pt2 - pt1) * (((i * dashGapTotalLength) + dashLength) / distance);
        cv::line(img, start, end, color, thickness, lineType);
    }
}

// 在图像上添加分类标签
void draw_label_text(Mat img, std::string text, Point org)
{
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 2;
    int thickness = 2;
    Scalar color(255, 0, 0); // BGR颜色值，蓝色
    // Point org(50, 50);       // 文本起始坐标

    // 在图像上添加文本
    // putText(img, text, org, fontFace, fontScale, color, thickness, cv::LINE_AA);
    putText(img, text, org, fontFace, fontScale, color, thickness, LINE_AA);
}

// 计算两点的距离
int get_distance(Point p1, Point p2)
{
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

// 找到4个数中最小的值
int find_smallest_value(int v1, int v2, int v3, int v4)
{
    int min = v1;
    if (v2 < min)
    {
        min = v2;
    }
    if (v3 < min)
    {
        min = v3;
    }
    if (v4 < min)
    {
        min = v4;
    }
    return min;
}

// 画连接的虚线
int draw_connect_line_to_point(Mat img, Point startPoint, box_info_t boxInfo)
{
    if (gParams.draw_distance == false)
    {
        return 0;
    }
    // 找到4个顶点中, 离 startPoint 最近的点
    int minIdx = 0;
    int distance1 = get_distance(startPoint, Point(boxInfo.left, boxInfo.top));
    int distance2 = get_distance(startPoint, Point(boxInfo.right, boxInfo.top));
    int distance3 = get_distance(startPoint, Point(boxInfo.left, boxInfo.bottom));
    int distance4 = get_distance(startPoint, Point(boxInfo.right, boxInfo.bottom));

    // printf("distance1:%d, distance2:%d, distance3:%d, distance4:%d\n", distance1, distance2, distance3, distance4);

    // 获取最小的距离
    int minDistance = find_smallest_value(distance1, distance2, distance3, distance4);
    if (minDistance == distance1)
    {
        drawDashedLine(img, startPoint, Point(boxInfo.left, boxInfo.top), Scalar(0, 255, 0), 2, LINE_AA);
        // line(img, startPoint, Point(boxInfo.left, boxInfo.top), Scalar(0, 255, 0), 2, LINE_AA);
    }
    else if (minDistance == distance2)
    {
        drawDashedLine(img, startPoint, Point(boxInfo.right, boxInfo.top), Scalar(0, 255, 0), 2, LINE_AA);
        // line(img, startPoint, Point(boxInfo.right, boxInfo.top), Scalar(0, 255, 0), 2, LINE_AA);
    }
    else if (minDistance == distance3)
    {
        drawDashedLine(img, startPoint, Point(boxInfo.left, boxInfo.bottom), Scalar(0, 255, 0), 2, LINE_AA);
        // line(img, startPoint, Point(boxInfo.left, boxInfo.bottom), Scalar(0, 255, 0), 2, LINE_AA);
    }
    else if (minDistance == distance4)
    {
        drawDashedLine(img, startPoint, Point(boxInfo.right, boxInfo.bottom), Scalar(0, 255, 0), 2, LINE_AA);
        // line(img, startPoint, Point(boxInfo.right, boxInfo.bottom), Scalar(0, 255, 0), 2, LINE_AA);
    }

    return minDistance;
}

// 加水印
void draw_water_mark(Mat img)
{

    cv::Ptr<cv::freetype::FreeType2> ft2;
    ft2 = cv::freetype::createFreeType2();
    ft2->loadFontData(gParams.chinese_font_path, 0);
    int fontHeight = gParams.water_mark_font_size; // 字体高度
    cv::Scalar textColor(0, 0, 0);                 // 文本颜色（红色）

    int i = 0;
    for (i = 0; i < 3; i++)
    {
        // 设置文本属性
        std::string waterMarkText = gParams.wn_water_mark[i].text; // 需要显示的中文文本
        cv::Point textOrg(gParams.wn_water_mark[i].start_x,
                          gParams.wn_water_mark[i].start_y); // 文本在图像中的位置

        ft2->putText(img, waterMarkText, textOrg, fontHeight, textColor, -1, cv::LINE_AA, true);
    }

    for (i = 0; i < 3; i++)
    {
        // 设置文本属性
        std::string waterMarkText = gParams.ws_water_mark[i].text; // 需要显示的中文文本
        cv::Point textOrg(gParams.ws_water_mark[i].start_x,
                          gParams.ws_water_mark[i].start_y); // 文本在图像中的位置

        ft2->putText(img, waterMarkText, textOrg, fontHeight, textColor, -1, cv::LINE_AA, true);
    }

    for (i = 0; i < 3; i++)
    {
        // 设置文本属性
        std::string waterMarkText = gParams.en_water_mark[i].text; // 需要显示的中文文本
        cv::Point textOrg(gParams.en_water_mark[i].start_x,
                          gParams.en_water_mark[i].start_y); // 文本在图像中的位置

        ft2->putText(img, waterMarkText, textOrg, fontHeight, textColor, -1, cv::LINE_AA, true);
    }

    for (i = 0; i < 3; i++)
    {
        // 设置文本属性
        std::string waterMarkText = gParams.es_water_mark[i].text; // 需要显示的中文文本
        cv::Point textOrg(gParams.es_water_mark[i].start_x,
                          gParams.es_water_mark[i].start_y); // 文本在图像中的位置

        ft2->putText(img, waterMarkText, textOrg, fontHeight, textColor, -1, cv::LINE_AA, true);
    }
}

// 监控回调函数
void monitor_rga_packet_cb(MEDIA_BUFFER mb)
{

    // 计算帧率
    static int frameCounter = 0;
    static int64_t lastTime = 0;
    int64_t curTime = get_current_time_us();
    frameCounter++;
    if (curTime - lastTime > 1000000)
    {
        printf("monitor_rga_packet_cb fps:%d\n", frameCounter);
        frameCounter = 0;
        lastTime = curTime;
    }

#if 0
    Mat orig_img = Mat(RTSP_INPUT_VI_HEIGHT, RTSP_INPUT_VI_WIDTH, CV_8UC3, RK_MPI_MB_GetPtr(mb));

    if (rknn_list_size(rknn_list_))
    {

        long time_before;
        detect_result_group_t detect_result_group;
        memset(&detect_result_group, 0, sizeof(detect_result_group));

        // pick up the first one
        rknn_list_pop(rknn_list_, &time_before, &detect_result_group);
        // printf("result count:%d \n", detect_result_group.count);
        int scale = RTSP_INPUT_VI_WIDTH / MODEL_INPUT_SIZE;
        for (int j = 0; j < detect_result_group.detect_count; j++)
        {
            int x = detect_result_group.results[j].box.left * scale;
            int startY = (detect_result_group.results[j].box.top - (MODEL_INPUT_SIZE - RKNN_INPUT_IMG_HEIGHT) / 2);
            if (startY < 0)
            {
                startY = 0;
            }
            int y = startY * scale;
            int w = (detect_result_group.results[j].box.right - detect_result_group.results[j].box.left) * scale;
            if (x + w > RTSP_INPUT_VI_WIDTH)
            {
                w = RTSP_INPUT_VI_WIDTH - x;
            }

            int h = (detect_result_group.results[j].box.bottom - detect_result_group.results[j].box.top) * scale;
            if (y + h > RTSP_INPUT_VI_HEIGHT)
            {
                h = RTSP_INPUT_VI_HEIGHT - y;
            }

            // printf("border=(%d %d %d %d)\n", x, y, w, h);
            boxInfoList[j] = {x, y, w, h, x, x + w, y, y + h, x + w / 2, y + h / 2};
            // boxInfoList.results[j] = detect_result_group.results[j];
            boxInfoListNumber++;
        }

        boxDisplayCounterDown = 1;

        // 黑白灰图案
        // cv::rectangle(orig_img, cv::Point(100, 100),
        // cv::Point(300, 500), cv::Scalar(0, 255, 0), 5, 8, 0);
        // cv::rectangle(orig_img, cv::Point(100, 100), cv::Point(250, 250), cv::Scalar(0, 255, 0), RKNN_BOX_BORDER);
        // cv::rectangle(orig_img, cv::Point(100, 100), cv::Point(250, 250), cv::Scalar(0, 255, 0), 2);
        // 画框
        // 取三个随机数

        for (int i = 0; i < boxInfoListNumber; i++)
        {
            int r = rand() % 256;
            int g = rand() % 256;
            int b = rand() % 256;
            // nv12_border((char *)RK_MPI_MB_GetPtr(mb), RTSP_INPUT_VI_WIDTH, RTSP_INPUT_VI_HEIGHT,
            // boxInfoList[i].x, boxInfoList[i].y, boxInfoList[i].w, boxInfoList[i].h, 0, 0, 255);

            rectangle(orig_img, cv::Point(boxInfoList[i].left, boxInfoList[i].top),
                      cv::Point(boxInfoList[i].right, boxInfoList[i].bottom), cv::Scalar(r, g, b), RKNN_BOX_BORDER);

            int distance = draw_connect_line_to_point(orig_img, Point(gParams.point_x, gParams.point_y), boxInfoList[i]);

            // 画框上的文字距离框10个像素
            char labelStr[100];
            sprintf(labelStr, "%s: %d", detect_result_group.results[i].name, distance);
            draw_label_text(orig_img, std::string(labelStr), Point(boxInfoList[i].left, boxInfoList[i].top - 10));
            // draw_text(orig_img, std::string str(detect_result_group.results[i].name), Point(boxInfoList[i].left, boxInfoList[i].top - 10));
            // Point(boxInfoList[i].centerX, boxInfoList[i].centerY));
        }

        boxInfoListNumber = 0;
        memset(boxInfoList, 0, sizeof(box_info_t) * OBJ_NUMB_MAX_SIZE);
    }

    draw_water_mark(orig_img);

    // cv转颜色
    cvtColor(orig_img, orig_img, COLOR_BGR2RGB);

#endif

    RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, MONITOR_RGA_CHN, mb);
    RK_MPI_MB_ReleaseBuffer(mb);
}

// 6220800
void *observer_thread_good(void *arg)
{
    MEDIA_BUFFER buffer = NULL;
    int fileSaveCounter = 0;
    int frameCounter = 0;
    long int lastTime = 0;
    long int curTime = get_time_now_sec();
    int boxInfoListNumber = 0;
    int boxDisplayCounterDown = 0;

    long int startTimeMs = 0;
    long int endTimeMs = 0;

    struct timeval tv;

    while (g_flag_run)
    {
        buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA, MONITOR_RGA_CHN, -1);
        if (!buffer)
        {
            usleep(1000);
            continue;
        }

        frameCounter++;
        curTime = get_time_now_sec();
        // printf("cur sec:%ld\n", curTime);
        // printf("curTime:%lld lastTime:%lld\n", curTime, lastTime);
        if ((curTime != lastTime))
        {
            printf("monitor fps:%d\n", frameCounter);
            frameCounter = 0;
            lastTime = curTime;
        }
        RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, MONITOR_VENC_CHN, buffer);
        RK_MPI_MB_ReleaseBuffer(buffer);
    }

    return nullptr;
}

static void save_jpeg(Mat orig_img)
{
    static int saveJpegCounter = 0;
    saveJpegCounter++;
    static int jpegNameCounter = 0;
    if (saveJpegCounter % gParams.jpeg_interval == 0)
    {
        jpegNameCounter++;
        Mat jpegImg;
        std::vector<int> param(2);
        param[0] = cv::IMWRITE_JPEG_QUALITY;
        param[1] = gParams.jpeg_quality; // default(95) 0-100
        cvtColor(orig_img, jpegImg, COLOR_BGR2RGB);
        resize(jpegImg, jpegImg, Size(gParams.jpeg_width, gParams.jpeg_height));
        char fileName[100];
        sprintf(fileName, "%s%d.jpg", gParams.jpeg_folder.c_str(), jpegNameCounter);
        imwrite(fileName, jpegImg, param);
    }
}

// 6220800
void *observer_thread(void *arg)
{
    MEDIA_BUFFER buffer = NULL;
    int fileSaveCounter = 0;
    int frameCounter = 0;
    long int lastTime = 0;
    long int curTime = get_time_now_sec();
    int boxInfoListNumber = 0;
    int boxDisplayCounterDown = 0;

    long int startTimeMs = 0;
    long int endTimeMs = 0;

    struct timeval tv;

    while (g_flag_run)
    {

        buffer = RK_MPI_SYS_GetMediaBuffer(RK_ID_RGA, MONITOR_RGA_CHN, -1);
        if (!buffer)
        {
            usleep(1000);
            continue;
        }

        gettimeofday(&tv, NULL);
        startTimeMs = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        Mat orig_img = Mat(RTSP_INPUT_VI_HEIGHT, RTSP_INPUT_VI_WIDTH, CV_8UC3, RK_MPI_MB_GetPtr(buffer));

        if (detect_result_group.detect_count > 0)
        {
            // printf("rknn_list_size:%d\n", rknn_list_size(rknn_list_));
            int scale = RTSP_INPUT_VI_WIDTH / MODEL_INPUT_SIZE;
            for (int j = 0; j < detect_result_group.detect_count; j++)
            {
                int x = detect_result_group.results[j].box.left * scale;
                int startY = (detect_result_group.results[j].box.top - (MODEL_INPUT_SIZE - RKNN_INPUT_IMG_HEIGHT) / 2);
                if (startY < 0)
                {
                    startY = 0;
                }
                int y = startY * scale;
                int w = (detect_result_group.results[j].box.right - detect_result_group.results[j].box.left) * scale;
                if (x + w > RTSP_INPUT_VI_WIDTH)
                {
                    w = RTSP_INPUT_VI_WIDTH - x;
                }

                int h = (detect_result_group.results[j].box.bottom - detect_result_group.results[j].box.top) * scale;
                if (y + h > RTSP_INPUT_VI_HEIGHT)
                {
                    h = RTSP_INPUT_VI_HEIGHT - y;
                }

                // printf("border=(%d %d %d %d)\n", x, y, w, h);
                boxInfoList[j] = {x, y, w, h, x, x + w, y, y + h, x + w / 2, y + h / 2};
                // boxInfoList.results[j] = detect_result_group.results[j];
                boxInfoListNumber++;
            }

            // 黑白灰图案
            // cv::rectangle(orig_img, cv::Point(100, 100),
            // cv::Point(300, 500), cv::Scalar(0, 255, 0), 5, 8, 0);
            // cv::rectangle(orig_img, cv::Point(100, 100), cv::Point(250, 250), cv::Scalar(0, 255, 0), RKNN_BOX_BORDER);
            // cv::rectangle(orig_img, cv::Point(100, 100), cv::Point(250, 250), cv::Scalar(0, 255, 0), 2);
            // 画框
            for (int i = 0; i < boxInfoListNumber; i++)
            {
                // 取三个随机数
                int r = 0;   // rand() % 256;
                int g = 255; // rand() % 256;
                int b = 0;   // rand() % 256;
                // nv12_border((char *)RK_MPI_MB_GetPtr(mb), RTSP_INPUT_VI_WIDTH, RTSP_INPUT_VI_HEIGHT,
                // boxInfoList[i].x, boxInfoList[i].y, boxInfoList[i].w, boxInfoList[i].h, 0, 0, 255);

                rectangle(orig_img, cv::Point(boxInfoList[i].left, boxInfoList[i].top),
                          cv::Point(boxInfoList[i].right, boxInfoList[i].bottom), cv::Scalar(r, g, b), RKNN_BOX_BORDER);

                int distance = draw_connect_line_to_point(orig_img, Point(gParams.point_x, gParams.point_y), boxInfoList[i]);

                // 画框上的文字距离框10个像素
                char labelStr[100];
                sprintf(labelStr, "%s: %d", detect_result_group.results[i].name, distance);
                draw_label_text(orig_img, std::string(labelStr), Point(boxInfoList[i].left, boxInfoList[i].top - 10));
                // draw_text(orig_img, std::string str(detect_result_group.results[i].name), Point(boxInfoList[i].left, boxInfoList[i].top - 10));
                // Point(boxInfoList[i].centerX, boxInfoList[i].centerY));
            }

            boxInfoListNumber = 0;
            memset(boxInfoList, 0, sizeof(box_info_t) * OBJ_NUMB_MAX_SIZE);
        }

        draw_water_mark(orig_img);

        // cv转颜色
        // cvtColor(orig_img, orig_img, COLOR_BGR2RGB);

        gettimeofday(&tv, NULL);
        endTimeMs = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        // printf("Predict spent : %ld ms\n", endTimeMs - startTimeMs);

        frameCounter++;
        curTime = get_time_now_sec();
        // printf("cur sec:%ld\n", curTime);
        // printf("curTime:%lld lastTime:%lld\n", curTime, lastTime);
        if ((curTime != lastTime))
        {
            printf("monitor fps:%d\n", frameCounter);
            frameCounter = 0;
            lastTime = curTime;
        }

        save_jpeg(orig_img);

        RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, MONITOR_VENC_CHN, buffer);
        RK_MPI_MB_ReleaseBuffer(buffer);
    }

    return nullptr;
}
