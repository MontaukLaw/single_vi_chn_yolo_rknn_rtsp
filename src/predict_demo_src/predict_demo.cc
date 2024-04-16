#include "comm.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <model_name>\n", argv[0]);
        return -1;
    }
    char *model_name = argv[1];
    
    int ret = init_model(model_name);
    // int ret = init_model("yolov5s_relu_tk1_RV1109_1126_u8_precompile.rknn");

    if (ret < 0)
    {
        printf("init model failed\n");
        return -1;
    }

    // 读取文件640x360.rgb
    FILE *fp = fopen("640x360.rgb", "rb");
    if (!fp)
    {
        printf("open file failed\n");
        return -1;
    }

    // 读取文件大小
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    printf("file size: %d\n", file_size);
    fseek(fp, 0, SEEK_SET);

    // 读取文件内容
    void *file_data = malloc(file_size);
    fread(file_data, 1, file_size, fp);
    fclose(fp);

    // 申请内存
    void *modeInputData = malloc(YOLO_INPUT_SIZE);
    memset(modeInputData, 0x7F, YOLO_INPUT_SIZE);

    // 填充数据
    fill_image_data(file_data, modeInputData,
                    RKNN_INPUT_IMG_WIDTH, RKNN_INPUT_IMG_HEIGHT,
                    MODEL_INPUT_SIZE, MODEL_INPUT_SIZE);
    // 开始推理
    detect_result_group_t detect_result_group_;
    memset(&detect_result_group_, 0, sizeof(detect_result_group_t));

    // Post Process
    int pResult = predict(modeInputData, &detect_result_group_);

    return 0;
}