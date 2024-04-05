
#include "deep_fence.h"

#define DEEP_FRAME_NUM 10
#define FENCE_WIDTH 4

extern g_params gParams;
extern detect_result_group_t detect_result_group;
extern box_info_t boxInfoList[];

typedef struct deep_frame_t
{
    bool is_valid;

    int deep_point_x;
    int deep_point_y;

    int deep_frame_left;
    int deep_frame_top;
    int deep_frame_right;
    int deep_frame_bottom;

} deep_frame;

deep_frame deep_frames[DEEP_FRAME_NUM] = {
    {true, 50, 50, 100, 100, 400, 400},
    {true, 200, 50, 500, 100, 1200, 900},
    {false, 0, 0, 0, 0, 0, 0},
    {false, 0, 0, 0, 0, 0, 0},
    {false, 0, 0, 0, 0, 0, 0},
    {false, 0, 0, 0, 0, 0, 0},
    {false, 0, 0, 0, 0, 0, 0},
    {false, 0, 0, 0, 0, 0, 0},
    {false, 0, 0, 0, 0, 0, 0},
    {false, 0, 0, 0, 0, 0, 0},
};

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

void draw_deep_fence_frames(Mat img)
{
    int i = 0;
    for (i = 0; i < DEEP_FRAME_NUM; i++)
    {
        if (deep_frames[i].is_valid == false)
        {
            continue;
        }

        rectangle(img, Point(deep_frames[i].deep_frame_left, deep_frames[i].deep_frame_top), Point(deep_frames[i].deep_frame_right, deep_frames[i].deep_frame_bottom), Scalar(0, 255, 0), FENCE_WIDTH);
        circle(img, Point(deep_frames[i].deep_point_x, deep_frames[i].deep_point_y), 5, Scalar(0, 255, 0), -1);
    }
}

void draw_connection(detect_result_t detect_result, Mat img, box_info_t boxInfo)
{
    // 1. 判断是否在围栏内
    int i = 0;
    for (i = 0; i < DEEP_FRAME_NUM; i++)
    {
        if (deep_frames[i].is_valid == false)
        {
            continue;
        }

        int check_point_x = boxInfo.left + (boxInfo.right - boxInfo.left) / 2;
        int check_point_y = boxInfo.bottom;
        // 判断依据是, detect_result的下边缘的中心点是否在围栏内
        // if (detect_result.box.left > deep_frames[i].deep_frame_left && detect_result.box.right < deep_frames[i].deep_frame_right &&
        // detect_result.box.top > deep_frames[i].deep_frame_top && detect_result.box.bottom < deep_frames[i].deep_frame_bottom)
        if (check_point_x > deep_frames[i].deep_frame_left && check_point_x < deep_frames[i].deep_frame_right &&
            check_point_y > deep_frames[i].deep_frame_top && check_point_y < deep_frames[i].deep_frame_bottom)
        {
            printf("inside the fence %d detect_result.classname:%s\n", i, detect_result.name);

            // 2. 如果在围栏内， 则画线
            int distance = draw_connect_line_to_point(img, Point(deep_frames[i].deep_point_x, deep_frames[i].deep_point_y), boxInfo);

            // 画框上的文字距离框10个像素
            // char labelStr[100];
            // sprintf(labelStr, "%s: %d", detect_result_group.results[i].name, distance);
            // draw_label_text(img, std::string(labelStr), Point(boxInfo.left, boxInfo.top - 10));

            // line(img, Point(detect_result.box.left, detect_result.box.top), Point(deep_frames[i].deep_point_x, deep_frames[i].deep_point_y), Scalar(0, 255, 0), FENCE_WIDTH);
        }
        else
        {
            // printf("detect_result.box.left %d\n", detect_result.box.left);
            // printf("detect_result.box.right %d\n", detect_result.box.right);
            // printf("detect_result.box.top %d\n", detect_result.box.top);
            // printf("detect_result.box.bottom %d\n", detect_result.box.bottom);
        }
    }
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
