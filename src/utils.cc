#include "comm.h"
#include "dirent.h"

extern g_params gParams;

static int if_my_mp4_fle(char *filename, int file_name_len)
{
    if (file_name_len != MP4_FILE_NAME_LEN)
    {
        return 0;
    }
    // 比较文件名字符串是否以mp4结尾
    if ((filename[file_name_len - 4] == '.') && (filename[file_name_len - 3] == 'm') && (filename[file_name_len - 2] == 'p') && (filename[file_name_len - 1] == '4'))
    {
        return 1;
    }
    return 0;
}

void find_oldest_file_name_prefix(char *file_name_buf)
{
    struct dirent *ent = NULL;
    DIR *pDir;
    pDir = opendir(gParams.mp4_folder.c_str());
    int result = 0;
    char file_name_prefix_str[MP4_FILE_NAME_LEN - 4];
    // char file_name_buf[MP4_FILE_NAME_LEN];
    long long file_name_prefix = 0;
    long long oldest_file_name_prefix = 0;

    while (NULL != (ent = readdir(pDir)))
    {
        // printf("ent->d_reclen:%d d_type:%d\n", ent->d_reclen, ent->d_type);
        if (ent->d_type == IS_FILE_NOT_FOLDER)
        {
            // printf("ent->d_type:%d\n", ent->d_type);

            if (if_my_mp4_fle(ent->d_name, strlen(ent->d_name)))
            {
                // printf("media file name:%s len:%d\n", ent->d_name, strlen(ent->d_name));

                memcpy(file_name_prefix_str, ent->d_name, MP4_FILE_NAME_LEN - 4);

                file_name_prefix = atoll(file_name_prefix_str);

                if (oldest_file_name_prefix == 0)
                {
                    oldest_file_name_prefix = file_name_prefix;
                }
                else if (oldest_file_name_prefix >= file_name_prefix)
                {
                    oldest_file_name_prefix = file_name_prefix;
                }
            }
        }
    }

    printf("oldest_file_name_prefix:%lld\n", oldest_file_name_prefix);

    sprintf(file_name_buf, "%s%lld.mp4", gParams.mp4_folder, oldest_file_name_prefix);
}

void list_files(const char *path, int *total_number)
// void list_files(char *path, int *total_number)
{
    struct dirent *ent = NULL;
    DIR *pDir;
    pDir = opendir(path);
    int result = 0;
    while (NULL != (ent = readdir(pDir)))
    {
        // printf("ent->d_reclen:%d d_type:%d\n", ent->d_reclen, ent->d_type);
        if (ent->d_type == IS_FILE_NOT_FOLDER)
        {
            // printf("ent->d_type:%d\n", ent->d_type);
            // printf("media file name:%s len:%d\n", ent->d_name, strlen(ent->d_name));

            if (if_my_mp4_fle(ent->d_name, strlen(ent->d_name)))
            {
                result++;
            }
        }
    }

    *total_number = result;
}

int count_my_mp4_file_number(void)
{
    int total_number = 0;
    list_files(gParams.mp4_folder.c_str(), &total_number);
    // printf("total_number:%d\n", total_number);
    return total_number;
}

void get_file_name_by_time(char *file_name_buf)
{
    time_t timep;
    struct tm *pLocalTime;
    time(&timep);

    pLocalTime = localtime(&timep);

    // 20230802201900.mp4
    long long time_long = (long long)(pLocalTime->tm_year + 1900) * 10000000000 +
                          (long long)(pLocalTime->tm_mon + 1) * 100000000 +
                          (long long)(pLocalTime->tm_mday) * 1000000 +
                          (long long)(pLocalTime->tm_hour) * 10000 +
                          (long long)(pLocalTime->tm_min) * 100 +
                          (long long)(pLocalTime->tm_sec);

    sprintf(file_name_buf, "%s%lld.mp4", gParams.mp4_folder.c_str(), time_long);

    printf("new file name = %s\n", file_name_buf);

    // print_time(pLocalTime);
}

int64_t get_current_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}