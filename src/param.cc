#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include "monitor.h"

g_params gParams;

std::map<std::string, std::string> readConfig(const std::string &filename)
{
    std::map<std::string, std::string> config;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line))
    {
        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '='))
        {
            std::string value;
            if (std::getline(is_line, value))
            {
                config[key] = value;
            }
        }
    }

    // 使用配置项
    gParams.save_mp4 = config["save_mp4"] == "true";
    gParams.mp4_length_sec = std::stoi(config["mp4_length"]);
    gParams.draw_distance = config["draw_distance"] == "true";
    gParams.point_x = std::stoi(config["point_x"]);
    gParams.point_y = std::stoi(config["point_y"]);
    gParams.water_mark_start_x = std::stoi(config["water_mark_start_x"]);
    gParams.water_mark_start_y = std::stoi(config["water_mark_start_y"]);
    gParams.water_mark_text=config["water_mark_text"];
    gParams.chinese_font_path=config["chinese_font_path"];
    gParams.mp4_folder=config["mp4_folder"];

    printf("Save MP4: %d\n", gParams.save_mp4);
    printf("MP4 Length: %d\n", gParams.mp4_length_sec);
    printf("Draw Distance: %d\n", gParams.draw_distance);
    printf("Point X: %d\n", gParams.point_x);
    printf("Point Y: %d\n", gParams.point_y);
    printf("Water Mark Start X: %d\n", gParams.water_mark_start_x);
    printf("Water Mark Start Y: %d\n", gParams.water_mark_start_y);
    printf("Water Mark Text: %s\n", gParams.water_mark_text.c_str());
    printf("Chinese Font Path: %s\n", gParams.chinese_font_path.c_str());
    printf("MP4 Folder: %s\n", gParams.mp4_folder.c_str());

    return config;
}