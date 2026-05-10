CROSS_COMPILE ?= arm-himix200-linux-
CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld

SDK_PATH ?= $(HOME)/hisi/Hi3516CV500_SDK_V2.0.2.0
MPP_PATH = $(SDK_PATH)/smp/a7_linux/mpp
SAMPLE_PATH ?= $(HOME)/test/sample

TARGET = sample_vio_nnie
TARGET_PATH = .

SMP_SRCS := src/main.c
SMP_SRCS += $(MPP_PATH)/sample/common/sample_comm_sys.c
SMP_SRCS += $(MPP_PATH)/sample/common/sample_comm_vi.c
SMP_SRCS += $(MPP_PATH)/sample/common/sample_comm_vpss.c
SMP_SRCS += $(MPP_PATH)/sample/common/sample_comm_vo.c
SMP_SRCS += $(MPP_PATH)/sample/common/sample_comm_region.c
SMP_SRCS += $(MPP_PATH)/sample/common/sample_comm_isp.c
SMP_SRCS += $(MPP_PATH)/sample/common/sample_comm_venc.c
SMP_SRCS += $(MPP_PATH)/sample/common/loadbmp.c
SMP_SRCS += $(MPP_PATH)/sample/svp/common/sample_comm_svp.c
SMP_SRCS += $(MPP_PATH)/sample/svp/common/sample_comm_nnie.c
SMP_SRCS += $(MPP_PATH)/sample/svp/common/sample_comm_ive.c
SMP_SRCS += $(SAMPLE_PATH)/nnie/sample/sample_nnie.c
SMP_SRCS += $(SAMPLE_PATH)/nnie/sample_nnie_software/sample_svp_nnie_software.c

CFLAGS += -Wall -O3
CFLAGS += -mcpu=cortex-a7 -mfloat-abi=softfp -mfpu=neon-vfpv4
CFLAGS += -fno-aggressive-loop-optimizations -fPIC
CFLAGS += -DHI_RELEASE -DKERNEL_BIT_32 -DUSER_BIT_32
CFLAGS += -DSENSOR0_TYPE=GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT
CFLAGS += -DSENSOR1_TYPE=SONY_IMX327_MIPI_2M_30FPS_12BIT
CFLAGS += -DHI_AUDIO_STATIC_REGISTER_SUPPORT=y

CFLAGS += -I$(MPP_PATH)/include
CFLAGS += -I$(MPP_PATH)/include/adapt
CFLAGS += -I$(MPP_PATH)/component/isp/include
CFLAGS += -I$(MPP_PATH)/component/isp/ext_inc
CFLAGS += -I$(MPP_PATH)/sample/common
CFLAGS += -I$(MPP_PATH)/sample/svp/common
CFLAGS += -I$(SAMPLE_PATH)/nnie/sample
CFLAGS += -I$(SAMPLE_PATH)/nnie/sample_nnie_software
CFLAGS += -I$(TARGET_PATH)/src
CFLAGS += -I$(MPP_PATH)/cbb/audio/include

LDFLAGS += -L$(MPP_PATH)/lib
LDFLAGS += -lmpi -lnnie -live -lisp -lmd -lhdmi -ltde
LDFLAGS += -lsns_gc2053 -lsns_imx327 -lsns_imx327_2l -lsns_imx307 -lsns_imx307_2l
LDFLAGS += -lsns_imx335 -lsns_imx458 -lsns_sc4210 -lsns_mn34220
LDFLAGS += -lsns_os04b10 -lsns_os05a -lsns_os08a10 -lsns_ov12870 -lsns_imx415
LDFLAGS += -lsecurec -ldnvqe -lupvqe -lVoiceEngine
LDFLAGS += -l_hiae -l_hidrc -l_hildci -l_hidehaze -l_hiawb -l_hicalcflicker
LDFLAGS += -lm -lpthread -ldl

all: $(TARGET)

$(TARGET): $(SMP_SRCS)
	@echo "Compiling $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET_PATH)/$@ $^ $(LDFLAGS)
	@echo "Build completed: $(TARGET_PATH)/$@"

clean:
	rm -f $(TARGET_PATH)/$(TARGET)
	rm -rf $(TARGET_PATH)/build

install:
	cp $(TARGET_PATH)/$(TARGET) /home/woio/hi3516_nfs/
	cp -rf $(TARGET_PATH)/data /home/woio/hi3516_nfs/

.PHONY: all clean install
