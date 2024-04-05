#ifndef __DEEP_FANCE_H__
#define __DEEP_FANCE_H__

#include <iostream>
#include "opencv2/opencv.hpp"
#include "opencv2/freetype.hpp"
#include "user_define.h"
#include "user_struct.h"
#include "monitor.h"

#ifdef __cplusplus
extern "C"
{
#endif

    using namespace cv;

    void draw_deep_fence_frames(Mat img);

    void draw_connection(detect_result_t detect_result, Mat img, box_info_t boxInfo);

    int draw_connect_line_to_point(Mat img, Point startPoint, box_info_t boxInfo);

#ifdef __cplusplus
}
#endif

#endif // __DEEP_FANCE_H__