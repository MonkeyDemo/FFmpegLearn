//
// Created by Easysight on 2018/12/18.
//

#ifndef LIVE555_ANDROID_LOG_H
#define LIVE555_ANDROID_LOG_H
#include <android/log.h>

#ifdef __cplusplus
extern "C"{
#endif

#define LOG_FLAG 0  //日志打印标志 (release版本中将1改为0即可不打印日志)
#define TAG "MonkeyDemo" //日志打印标签
#if LOG_FLAG
#define log_print_verbose(...) __android_log_print(ANDROID_LOG_VERBOSE,TAG,__VA_ARGS__)
#define log_print_debug(...) __android_log_print(ANDROID_LOG_DEBUG,TAG,__VA_ARGS__)
#define log_print_info(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#define log_print_warn(...) __android_log_print(ANDROID_LOG_WARN,TAG,__VA_ARGS__)
#define log_print_error(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)
#else
#define log_print_verbose(...)
#define log_print_debug(...)
#define log_print_info(...)
#define log_print_warn(...)
#define log_print_error(...)
#endif
#define LOGV(...) log_print_verbose(__VA_ARGS__)
#define LOGD(...) log_print_debug(__VA_ARGS__)
#define LOGI(...) log_print_info(__VA_ARGS__)
#define LOGW(...) log_print_warn(__VA_ARGS__)
#define LOGE(...) log_print_error(__VA_ARGS__)



#ifdef __cplusplus
}
#endif

#endif //LIVE555_ANDROID_LOG_H
