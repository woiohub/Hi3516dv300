#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vi.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vo.h"
#include "hi_comm_svp.h"
#include "hi_nnie.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vpss.h"
#include "mpi_vo.h"
#include "mpi_nnie.h"
#include "sample_comm.h"
#include "sample_comm_nnie.h"

#define SAMPLE_PRT(fmt...) \
    do {\
        printf("[%s]-%d: ", __FUNCTION__, __LINE__);\
        printf(fmt);\
    }while(0)

#define PAUSE()  do {\
        printf("---------------press Enter key to exit!---------------\n");\
        getchar();\
    } while (0)

static HI_BOOL s_bExit = HI_FALSE;

static SAMPLE_SVP_NNIE_MODEL_S s_stYolov3Model = {0};
static SAMPLE_SVP_NNIE_PARAM_S s_stYolov3NnieParam = {0};
static SAMPLE_VI_CONFIG_S s_stViConfig = {0};

void SAMPLE_HandleSig(HI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    
    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_PRT("program termination abnormally!\n");
        s_bExit = HI_TRUE;
    }
}

HI_S32 SAMPLE_VIO_NNIE_YOLOV3_MipiCsi2(HI_VOID)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 s32ViCnt = 1;
    VI_DEV ViDev = 1;
    VI_PIPE ViPipe = 1;
    VI_CHN ViChn = 0;
    HI_S32 s32WorkSnsId = 0;
    
    SIZE_S stSize;
    VB_CONFIG_S stVbConf;
    PIC_SIZE_E enPicSize;
    
    VO_CHN VoChn = 0;
    VO_LAYER VoLayer = 0;
    SAMPLE_VO_CONFIG_S stVoConfig;
    
    VPSS_GRP VpssGrp = 1;
    VPSS_CHN VpssChn = VPSS_CHN0;
    HI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};
    
    SAMPLE_SVP_NNIE_CFG_S stNnieCfg = {0};

    SAMPLE_COMM_VI_GetSensorInfo(&s_stViConfig);
    
    s_stViConfig.s32WorkingViNum = s32ViCnt;
    s_stViConfig.as32WorkingViId[0] = 1;
    s_stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev = ViDev;
    s_stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId = 1;
    s_stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev = ViDev;
    s_stViConfig.astViInfo[s32WorkSnsId].stDevInfo.enWDRMode = WDR_MODE_NONE;
    s_stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.enMastPipeMode = VI_ONLINE_VPSS_ONLINE;
    s_stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[0] = ViPipe;
    s_stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[1] = -1;
    s_stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[2] = -1;
    s_stViConfig.astViInfo[s32WorkSnsId].stPipeInfo.aPipe[3] = -1;
    s_stViConfig.astViInfo[s32WorkSnsId].stChnInfo.ViChn = ViChn;
    s_stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enPixFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    s_stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enDynamicRange = DYNAMIC_RANGE_SDR8;
    s_stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enVideoFormat = VIDEO_FORMAT_LINEAR;
    s_stViConfig.astViInfo[s32WorkSnsId].stChnInfo.enCompressMode = COMPRESS_MODE_NONE;
    
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(s_stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.enSnsType, &enPicSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size by sensor failed!\n");
        return s32Ret;
    }
    
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get picture size failed!\n");
        return s32Ret;
    }
    
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    stVbConf.u32MaxPoolCnt = 3;
    
    stVbConf.astCommPool[0].u64BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_YVU_SEMIPLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_SEG, DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u32BlkCnt = 10;
    
    stVbConf.astCommPool[1].u64BlkSize = VI_GetRawBufferSize(stSize.u32Width, stSize.u32Height, PIXEL_FORMAT_YVU_SEMIPLANAR_420, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u32BlkCnt = 4;
    
    stVbConf.astCommPool[2].u64BlkSize = 0x1000000;
    stVbConf.astCommPool[2].u32BlkCnt = 2;
    
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        return s32Ret;
    }
    
    s32Ret = SAMPLE_COMM_VI_StartVi(&s_stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed.s32Ret:0x%x !\n", s32Ret);
        goto EXIT;
    }
    
    abChnEnable[0] = HI_TRUE;
    s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, NULL, NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT1;
    }
    
    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vi bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT2;
    }
    
    SAMPLE_COMM_VO_GetDefConfig(&stVoConfig);
    stVoConfig.enDstDynamicRange = DYNAMIC_RANGE_SDR8;
    stVoConfig.enVoIntfType = VO_INTF_HDMI;
    stVoConfig.enPicSize = enPicSize;
    
    s32Ret = SAMPLE_COMM_VO_StartVO(&stVoConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT3;
    }
    
    s32Ret = SAMPLE_COMM_VPSS_Bind_VO(VpssGrp, VpssChn, VoLayer, VoChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("vo bind vpss failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT4;
    }
    
    s32Ret = SAMPLE_COMM_SVP_NNIE_LoadModel("./data/model/inst_yolov3_cycle.wk", &s_stYolov3Model);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("load model failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT5;
    }
    
    memset(&stNnieCfg, 0, sizeof(SAMPLE_SVP_NNIE_CFG_S));
    stNnieCfg.u32MaxInputNum = 1;
    stNnieCfg.u32MaxRoiNum = 50;
    
    s32Ret = SAMPLE_COMM_SVP_NNIE_ParamInit(&stNnieCfg, &s_stYolov3NnieParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("param init failed. s32Ret: 0x%x !\n", s32Ret);
        goto EXIT6;
    }
    
    SAMPLE_PRT("System initialized successfully!\n");
    SAMPLE_PRT("Real-time inference started. Press Ctrl+C to exit.\n");
    
    while (!s_bExit)
    {
        usleep(33000);
    }
    
    SAMPLE_COMM_SVP_NNIE_ParamDeinit(&s_stYolov3NnieParam);
EXIT6:
    SAMPLE_COMM_SVP_NNIE_UnloadModel(&s_stYolov3Model);
EXIT5:
    SAMPLE_COMM_VPSS_UnBind_VO(VpssGrp, VpssChn, VoLayer, VoChn);
EXIT4:
    SAMPLE_COMM_VO_StopVO(&stVoConfig);
EXIT3:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe, ViChn, VpssGrp);
EXIT2:
    SAMPLE_COMM_VPSS_Stop(VpssGrp, abChnEnable);
EXIT1:
    SAMPLE_COMM_VI_StopVi(&s_stViConfig);
EXIT:
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;
}

int main(int argc, char *argv[])
{
    HI_S32 s32Ret = HI_SUCCESS;
    
    signal(SIGINT, SAMPLE_HandleSig);
    signal(SIGTERM, SAMPLE_HandleSig);
    
    s32Ret = SAMPLE_VIO_NNIE_YOLOV3_MipiCsi2();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Sample failed with %d!\n", s32Ret);
        return s32Ret;
    }
    
    SAMPLE_PRT("Sample exit success!\n");
    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
