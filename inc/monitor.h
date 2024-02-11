#ifndef __MONITOR_H_
#define __MONITOR_H_
#include <map>
#include "opencv2/opencv.hpp"
#include "opencv2/freetype.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

#include "comm.h"

    typedef struct g_params_t
    {
        bool save_mp4;
        int mp4_length_sec;
        bool draw_distance;
        int point_x;
        int point_y;
        int water_mark_start_x;
        int water_mark_start_y;
        std::string chinese_font_path;
        std::string water_mark_text;
        std::string mp4_folder;
    } g_params;

    std::map<std::string, std::string> readConfig(const std::string &filename);

    void monitor_rga_packet_cb(MEDIA_BUFFER mb);

    void *observer_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif
