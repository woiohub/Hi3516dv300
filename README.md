# Hi3516DV300 GC2053 NNIE 本地私有化部署项目

## 项目概述

本项目基于易百纳 EB-Hi3516DV300-DC-182 开发板和格科微 GC2053 1080p CMOS 图像传感器，实现官方训练好的 YOLOv3 模型本地私有化部署，支持图像传感器输入、本地模型实时推理，并通过 HDMI 输出推理结果。

## 硬件平台

- **开发板**: 易百纳 EB-Hi3516DV300-DC-182
- **图像传感器**: 格科微 GC2053 (2MP, 1080p@30fps)
- **接口**: MIPI CSI 接口 2
- **输出**: HDMI

## 软件环境

- **SDK**: Hi3516CV500_SDK_V2.0.2.0
- **交叉编译器**: arm-himix200-linux-gcc
- **推理引擎**: HiSilicon NNIE (Neural Network Inference Engine)
- **模型**: YOLOv3 (inst_yolov3_cycle.wk)

## 项目结构

```
project/
├── src/
│   └── main.c              # 主程序文件
├── data/
│   └── model/
│       └── inst_yolov3_cycle.wk  # YOLOv3 模型文件
├── build/                  # 编译输出目录
├── Makefile                # 编译脚本
└── README.md               # 项目文档
```

## 编译步骤

### 1. 环境准备

确保交叉编译器已安装并配置好环境变量：

```bash
export PATH=/path/to/arm-himix200-linux/bin:$PATH
```

### 2. 编译项目

进入项目目录，执行 make 命令：

```bash
cd /home/woio/test/project
make
```

编译成功后，可执行文件 `sample_vio_nnie` 将生成在项目根目录下。

### 3. 清理编译

```bash
make clean
```

## 部署与运行

### 1. 开发板 NFS 挂载

开发板已通过 NFS 挂载到 Ubuntu 主机的 `/home/woio/hi3516_nfs` 目录。

### 2. 安装到开发板

```bash
make install
```

此命令将：
- 复制可执行文件 `sample_vio_nnie` 到 `/home/woio/hi3516_nfs/`
- 复制数据目录 `data/` 到 `/home/woio/hi3516_nfs/`

### 3. 硬件连接

| 设备 | 连接方式 |
|------|----------|
| GC2053 图像传感器 | MIPI CSI 接口 2 |
| HDMI 显示屏 | HDMI 接口 |

### 4. 在开发板上运行

```bash
cd /home/woio/hi3516_nfs
chmod +x sample_vio_nnie
./sample_vio_nnie
```

### 5. 退出程序

按 `Ctrl+C` 退出程序。

## 核心配置说明

### MIPI CSI 接口 2 配置

在 `src/main.c` 中，MIPI CSI 接口 2 的关键配置：

```c
VI_DEV ViDev = 1;           // MIPI CSI 接口 2 (接口1为0，接口2为1)
VI_PIPE ViPipe = 1;
s_stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.MipiDev = ViDev;
s_stViConfig.astViInfo[s32WorkSnsId].stSnsInfo.s32BusId = 1;
s_stViConfig.astViInfo[s32WorkSnsId].stDevInfo.ViDev = ViDev;
```

### 传感器配置

传感器类型通过编译宏定义：

```makefile
CFLAGS += -DSENSOR0_TYPE=GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT
```

### NNIE 模型加载

```c
s32Ret = SAMPLE_COMM_SVP_NNIE_LoadModel("./data/model/inst_yolov3_cycle.wk", &s_stYolov3Model);
```

## 功能说明

1. **VIO 初始化**: 配置 MIPI CSI 接口 2，启动视频输入
2. **VPSS 配置**: 启动视频处理子系统
3. **VO 配置**: 配置 HDMI 输出显示
4. **NNIE 模型加载**: 加载 YOLOv3 目标检测模型
5. **实时推理**: 持续运行，支持通过信号退出

## 技术架构

```
GC2053 Sensor → MIPI CSI 2 → VI → VPSS → VO (HDMI)
                               ↓
                          NNIE Inference
                               ↓
                          推理结果输出
```

## 常见问题

### 1. 编译错误 - 找不到头文件

确保 SDK 路径正确配置在 Makefile 中：

```makefile
SDK_PATH = /home/woio/hisi/Hi3516CV500_SDK_V2.0.2.0
```

### 2. 链接错误 - 找不到库文件

检查 MPP 库路径是否正确：

```makefile
MPP_PATH = $(SDK_PATH)/smp/a7_linux/mpp
LDFLAGS += -L$(MPP_PATH)/lib
```

### 3. 运行时错误 - 模型加载失败

确保模型文件路径正确：

```bash
ls -la /home/woio/hi3516_nfs/data/model/
```

### 4. 视频输出异常

检查 HDMI 连接线是否正确连接，显示器是否支持 1080p 分辨率。

## 参考文档

- Hi3516CV500 SDK 用户手册
- HiMPP V4.0 媒体处理软件开发参考
- NNIE 开发指南

## 许可证

本项目基于海思 SDK 示例代码开发，遵循海思相关许可协议。
