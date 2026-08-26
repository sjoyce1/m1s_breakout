SDK_DEMO_PATH ?= .
BL_SDK_BASE ?= $(SDK_DEMO_PATH)/bouffalo_sdk

export BL_SDK_BASE

CHIP ?= bl808
BOARD ?= bl808dk
CPU_ID ?= m0

include $(BL_SDK_BASE)/project.build

