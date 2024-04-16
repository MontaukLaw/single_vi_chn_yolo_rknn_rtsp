
#include "yolo_detect.h"

static void fill_image_data(void *inputData, void *modelData, int inputWidth, int inputHeight, int modelWidth, int modelHeight)
{
    unsigned char *src = static_cast<unsigned char *>(inputData);
    unsigned char *dest = static_cast<unsigned char *>(modelData);

    // 确保宽度相同
    if (inputWidth != modelWidth)
    {
        printf("Width not match\n");
        return;
    }

    // 计算垂直居中时的起始行（在目标图像中）
    int verticalPadding = (modelHeight - inputHeight) / 2;

    // 每个像素3个字节（RGB）
    int bytesPerPixel = 3;

    // 计算每行数据的字节数
    int rowBytes = inputWidth * bytesPerPixel;

    // 计算目标开始位置的指针
    unsigned char *destRowStart = dest + (verticalPadding * rowBytes);

    // 一次性复制整个图像数据块
    memcpy(destRowStart, src, inputHeight * rowBytes);
}

int main(int argc, char **argv)
{
    int ret = 0;
    char *model_name = NULL;

    if (argc < 2)
    {
        printf("Usage: %s <rknn model> \n", argv[0]);
        return -1;
    }

    model_name = (char *)argv[1];
    ret = init_model(model_name);
    if (ret < 0)
    {
        printf("init model failed\n");
        return -1;
    }

    printf("mode is ready");
    // 读取文件640x360.rgb
    FILE *fp = fopen("640x360_2.rgb", "rb");
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
    void *input_data = malloc(YOLO_INPUT_SIZE);
    memset(input_data, 0, YOLO_INPUT_SIZE);

    // 填充数据
    fill_image_data(file_data, input_data,
                    RKNN_INPUT_IMG_WIDTH, RKNN_INPUT_IMG_HEIGHT,
                    MODEL_INPUT_SIZE, MODEL_INPUT_SIZE);
    detect_result_group_t detect_result_group;

    detect(input_data, &detect_result_group);

    release_model();

    if (file_data)
    {
        free(file_data);
    }

    if (input_data)
    {
        free(input_data);
    }

    return 0;
}
