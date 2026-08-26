SDK_DEMO_PATH ?= .
BL_SDK_BASE ?= $(SDK_DEMO_PATH)/../../bflb_mcu_sdk

export BL_SDK_BASE

CHIP ?= bl808
BOARD ?= bl808_m1s_dock
CPU_ID ?= d0

include $(BL_SDK_BASE)/project.build
