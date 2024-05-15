#include "comm.h"

// 将vi绑定到vga
int bind_vi_rga(RK_S32 cameraId, RK_U32 viChnId, RK_U32 rgaChnId)
{

    RK_S32 ret = 0;

    MPP_CHN_S stSrcChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = cameraId;
    stSrcChn.s32ChnId = viChnId;

    MPP_CHN_S stDestChn;
    stDestChn.enModId = RK_ID_RGA;
    stDestChn.s32ChnId = rgaChnId;

    ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret != 0)
    {
        printf("ERROR: Bind vi[%d] and rga[%d] failed! ret=%d\n", viChnId, rgaChnId, ret);
        return -1;
    }

    return 0;
}

// 解绑vi跟rga
void unbind_vi_rga(RK_S32 cameraId, RK_U32 viChnId, RK_U32 rgaChnId)
{
    RK_S32 ret = 0;

    MPP_CHN_S stSrcChn;
    stSrcChn.enModId = RK_ID_VI;
    stSrcChn.s32DevId = cameraId;
    stSrcChn.s32ChnId = viChnId;

    MPP_CHN_S stDestChn;
    stDestChn.enModId = RK_ID_RGA;
    stDestChn.s32ChnId = rgaChnId;

    ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret != 0)
    {
        printf("ERROR: UnBind vi[%d] and rga[%d] failed! ret=%d\n", viChnId, rgaChnId, ret);
    }
}

int create_vi(RK_S32 s32CamId, RK_U32 u32Width, RK_U32 u32Height, const RK_CHAR *pVideoNode, RK_S32 viChn)
{
    RK_S32 ret = 0;

    VI_CHN_ATTR_S vi_chn_attr;
    vi_chn_attr.pcVideoNode = pVideoNode;
    vi_chn_attr.u32BufCnt = 3;
    vi_chn_attr.u32Width = u32Width;
    vi_chn_attr.u32Height = u32Height;
    vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
    vi_chn_attr.enBufType = VI_CHN_BUF_TYPE_MMAP;
    vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
    ret = RK_MPI_VI_SetChnAttr(0, viChn, &vi_chn_attr);
    ret |= RK_MPI_VI_EnableChn(s32CamId, viChn);
    if (ret)
    {
        printf("ERROR: create VI[%d] error! ret=%d\n", viChn, ret);
        return -1;
    }
    return 0;
}

// 创建rga通道
int create_rga_(RK_S32 rgaChn)
{
    RGA_ATTR_S stRgaAttr;
    memset(&stRgaAttr, 0, sizeof(stRgaAttr));
    stRgaAttr.bEnBufPool = RK_TRUE;
    stRgaAttr.u16BufPoolCnt = 3;
    stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
    stRgaAttr.u16Rotaion = 0;
    stRgaAttr.stImgIn.u32X = 0;
    stRgaAttr.stImgIn.u32Y = 0;
}

// 创建rga通道
// 测试ok
int create_rga_nice(RK_S32 rgaChn, RK_U32 u32WidthInput, RK_U32 u32HeightInput, RK_U32 u32WidthOutput, RK_U32 u32HeightOutput)
{
    RGA_ATTR_S stRgaAttr;
    memset(&stRgaAttr, 0, sizeof(stRgaAttr));
    stRgaAttr.bEnBufPool = RK_TRUE;
    stRgaAttr.u16BufPoolCnt = 3;
    stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
    stRgaAttr.u16Rotaion = 90;
    stRgaAttr.stImgIn.u32X = 0;
    stRgaAttr.stImgIn.u32Y = 0;
    // 这里是旋转后的宽高
    stRgaAttr.stImgIn.u32Width = 1080; // u32WidthOutput; // 注意这里是输入的宽高
    stRgaAttr.stImgIn.u32Height = 1920;

    // 2592x1944
    // 输入的宽高
    stRgaAttr.stImgIn.u32HorStride = 2592;
    stRgaAttr.stImgIn.u32VirStride = 1944;

    stRgaAttr.stImgOut.u32X = 0; // 输出的起始位置
    stRgaAttr.stImgOut.u32Y = 0;
    stRgaAttr.stImgOut.imgType = IMAGE_TYPE_BGR888; // IMAGE_TYPE_RGB888; // IMAGE_TYPE_RGB888; // IMAGE_TYPE_NV12; // 输出的格式
    stRgaAttr.stImgOut.u32Width = 1920;             // 输出的宽高
    stRgaAttr.stImgOut.u32Height = 1080;
    stRgaAttr.stImgOut.u32HorStride = 1920; // 输出的虚高;
    stRgaAttr.stImgOut.u32VirStride = 1080;

    RK_S32 ret = RK_MPI_RGA_CreateChn(rgaChn, &stRgaAttr); // 创建rga通道

    if (ret)
    {
        printf("ERROR: RGA Set Attr: ImgIn:<%u,%u,%u,%u> "
               "ImgOut:<%u,%u,%u,%u>, rotation=%u failed! ret=%d\n",
               stRgaAttr.stImgIn.u32X, stRgaAttr.stImgIn.u32Y,
               stRgaAttr.stImgIn.u32Width, stRgaAttr.stImgIn.u32Height,
               stRgaAttr.stImgOut.u32X, stRgaAttr.stImgOut.u32Y,
               stRgaAttr.stImgOut.u32Width, stRgaAttr.stImgOut.u32Height,
               stRgaAttr.u16Rotaion, ret);
        return -1;
    }
}

// 创建rga通道
int create_rga_rknn(RK_S32 rgaChn, RK_U32 u32WidthInput, RK_U32 u32HeightInput, RK_U32 u32WidthOutput, RK_U32 u32HeightOutput)
{
}

// 创建rga通道
int create_rga(RK_S32 rgaChn, RK_U32 u32WidthInput, RK_U32 u32HeightInput, RK_U32 u32WidthOutput, RK_U32 u32HeightOutput)
{
    RGA_ATTR_S stRgaAttr;
    memset(&stRgaAttr, 0, sizeof(stRgaAttr));
    stRgaAttr.bEnBufPool = RK_TRUE;
    stRgaAttr.u16BufPoolCnt = 3;
    stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
    // 旋转
    stRgaAttr.u16Rotaion = 90;

    // stRgaAttr.stImgIn.u32X = (2592 - 1080) / 2;
    // stRgaAttr.stImgIn.u32Y = (1944 - 1920) / 2;
    stRgaAttr.stImgIn.u32X = (u32WidthInput - u32HeightOutput) / 2;
    stRgaAttr.stImgIn.u32Y = (u32HeightInput - u32WidthOutput) / 2;

    // 这里是旋转后的用于输入的宽高, 即裁剪出的部分, 与上面的x,y组成了一个rect
    stRgaAttr.stImgIn.u32Width = u32HeightOutput;
    stRgaAttr.stImgIn.u32Height = u32WidthOutput;

    // 2592x1944
    // 输入的宽高
    stRgaAttr.stImgIn.u32HorStride = u32WidthInput;
    stRgaAttr.stImgIn.u32VirStride = u32HeightInput;

    stRgaAttr.stImgOut.u32X = 0; // 输出的起始位置
    stRgaAttr.stImgOut.u32Y = 0;
    stRgaAttr.stImgOut.imgType = IMAGE_TYPE_BGR888; // IMAGE_TYPE_RGB888; // IMAGE_TYPE_RGB888; // IMAGE_TYPE_NV12; // 输出的格式
    stRgaAttr.stImgOut.u32Width = u32WidthOutput;   // 输出的宽高
    stRgaAttr.stImgOut.u32Height = u32HeightOutput;
    stRgaAttr.stImgOut.u32HorStride = u32WidthOutput; // 输出的虚高;
    stRgaAttr.stImgOut.u32VirStride = u32HeightOutput;

    RK_S32 ret = RK_MPI_RGA_CreateChn(rgaChn, &stRgaAttr); // 创建rga通道

    if (ret)
    {
        printf("ERROR: RGA Set Attr: ImgIn:<%u,%u,%u,%u> "
               "ImgOut:<%u,%u,%u,%u>, rotation=%u failed! ret=%d\n",
               stRgaAttr.stImgIn.u32X, stRgaAttr.stImgIn.u32Y,
               stRgaAttr.stImgIn.u32Width, stRgaAttr.stImgIn.u32Height,
               stRgaAttr.stImgOut.u32X, stRgaAttr.stImgOut.u32Y,
               stRgaAttr.stImgOut.u32Width, stRgaAttr.stImgOut.u32Height,
               stRgaAttr.u16Rotaion, ret);
        return -1;
    }
}
