#ifndef __TRAN_FUNC_H__
#define __TRAN_FUNC_H__

#include <vector>

#include "comm.h"

#ifdef _WIN32
#define CROSS_PLATFORM_HIDDEN_API
#ifdef EXPORT_LIB
#define API_DECLSPEC __declspec(dllexport)
#else
#define API_DECLSPEC __declspec(dllimport)
#endif
#else                                                           // 说明是 OSX 或者 Linux
#define API_DECLSPEC __attribute((visibility("default")))       // 明确指示，这个函数在动态库中可见
#define API_DECLSPEC_HIDDEN __attribute((visibility("hidden"))) // 明确指示，这个函数在动态库中不可见
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define RTSP_INPUT_VI_WIDTH 1280
#define RTSP_INPUT_VI_HEIGHT 720

// #define RTSP_INPUT_VI_WIDTH 1920
// #define RTSP_INPUT_VI_HEIGHT 1080

#define OBJ_NAME_MAX_SIZE 16
#define OBJ_NUMB_MAX_SIZE 64

  typedef struct
  {
    float x1;
    float y1;
    float x2;
    float y2;
    float mindist;
  } _outMinDitsReuslt;

  int calDistance(detect_result_group_t *detect_result_group, std::vector<_outMinDitsReuslt> &outMindist);

#ifdef __cplusplus
}
#endif
#endif
