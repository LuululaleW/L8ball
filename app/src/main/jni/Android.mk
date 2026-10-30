LOCAL_PATH := $(call my-dir)

# ---------------------------------------------------------------------------
# Native module
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := gdtqis

LOCAL_CFLAGS := -Wno-error=format-security -fvisibility=hidden -ffunction-sections -fdata-sections -w
LOCAL_CFLAGS += -fpermissive -rdynamic -funwind-tables

LOCAL_CPPFLAGS := -std=c++20 -frtti -fexceptions -fpermissive
LOCAL_CPPFLAGS += -fvisibility=hidden -ffunction-sections -fdata-sections -w
LOCAL_CPPFLAGS += -Wno-error=c++11-narrowing -fms-extensions
LOCAL_CPPFLAGS += -rdynamic -funwind-tables -fno-omit-frame-pointer

LOCAL_LDFLAGS += -Wl,--strip-all
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := main.cpp \
    include/external/dlfcn/dlfcn.cpp \
    include/external/imgui/imgui.cpp \
    include/external/Substrate/SubstrateHook.cpp \
    include/external/Substrate/SubstrateDebug.cpp \
    include/external/Substrate/SubstratePosixMemory.cpp \
    include/external/And64InlineHook/And64InlineHook.cpp \
    include/external/xhook/xhook.c \
    include/external/oxorany/oxorany.cpp

LOCAL_C_INCLUDES := $(NDK_ROOT)/sources/android
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/external

LOCAL_LDLIBS := -landroid -lGLESv3 -lEGL -ldl -llog -lz

include $(BUILD_SHARED_LIBRARY)
