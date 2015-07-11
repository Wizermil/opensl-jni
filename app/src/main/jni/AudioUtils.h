#ifndef __AudioUtils__
#define __AudioUtils__

#include <android/log.h>

#define  LOG_TAG    "libaudio"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGEX(msg) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "fun:%s,line:%d,msg:%s", __func__, __LINE__, #msg)

#endif



