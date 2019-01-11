#include <jni.h>
#include <string>
#include "com_example_ffplayer_FFSurfaceView.h"
#include "android_log.h"
#include <pthread.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
//引入头文件
extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavutil/imgutils.h"
#include <libavcodec/avcodec.h>
//#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
//引入时间
#include "libavutil/time.h"
//引入库
#pragma comment(lib,"avformat.lib")
//工具库，包括获取错误信息等
#pragma comment(lib,"avutil.lib")
//编解码的库
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avfilter.lib")
}


ANativeWindow *mANativeWindow;
ANativeWindow_Buffer nwBuffer;

AVFormatContext *m_iformatCtx;
AVCodecContext *m_icondecCtx;
AVCodec *m_icodec;
char inputUrl[200] = {0};
bool mFlagStop = false;
bool mFlayPause = false;



/**
 * 渲染surface
 * @param pixel
 */
void render_surface(uint8_t *pixel) {
    LOGV("MonkeyDemo:  renderSurface");
    if (mFlayPause) {
        return;
    }

    ANativeWindow_acquire(mANativeWindow);
//    LOGV("开始渲染");
    if (0 != ANativeWindow_lock(mANativeWindow, &nwBuffer, NULL)) {
        LOGV("ANativeWindow_lock() error");
        return;
    }
    //LOGV("renderSurface, %d, %d, %d", nwBuffer.width ,nwBuffer.height, nwBuffer.stride);
    if (nwBuffer.width >= nwBuffer.stride) {
        //srand(time(NULL));
        //memset(piexels, rand() % 100, nwBuffer.width * nwBuffer.height * 2);
        //memcpy(nwBuffer.bits, piexels, nwBuffer.width * nwBuffer.height * 2);
        memcpy(nwBuffer.bits, pixel, nwBuffer.width * nwBuffer.height * 2);
    } else {
//        LOGV("new buffer width is %d,height is %d ,stride is %d",
//             nwBuffer.width, nwBuffer.height, nwBuffer.stride);
        int i;
        for (i = 0; i < nwBuffer.height; ++i) {
            memcpy((void*) ((uintptr_t) nwBuffer.bits + nwBuffer.stride * i * 2),
                   (void*) ((uintptr_t) pixel + nwBuffer.width * i * 2),
                   nwBuffer.width * 2);
        }
    }

    if (0 != ANativeWindow_unlockAndPost(mANativeWindow)) {
        LOGV("ANativeWindow_unlockAndPost error");
        return;
    }

    ANativeWindow_release(mANativeWindow);
}
/**
 * 设置渲染宽高
 * @param width
 * @param height
 * @return
 */
int32_t set_BuffersGeometry(int32_t width, int32_t height) {
    int32_t format = WINDOW_FORMAT_RGB_565;

    if (NULL == mANativeWindow) {
        LOGV("mANativeWindow is NULL.");
        return -1;
    }

    return ANativeWindow_setBuffersGeometry(mANativeWindow, width, height,
                                            format);
}

/**
 * 格式转换
 * @param dst_data
 * @param dst_linesize
 * @param dst_pix_fmt
 * @param src
 * @param src_pix_fmt
 * @param src_width
 * @param src_height
 * @return
 */
int img_convert(uint8_t *dst_data[8],int dst_linesize[8], int dst_pix_fmt, const AVPicture *src,
                int src_pix_fmt, int src_width, int src_height) {
    int w;
    int h;
    struct SwsContext *pSwsCtx;

    w = src_width;
    h = src_height;

    pSwsCtx = sws_getContext(w, h, (AVPixelFormat) src_pix_fmt, w, h,
                             (AVPixelFormat) dst_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
    sws_scale(pSwsCtx, (const uint8_t *const *) src->data, src->linesize, 0, h,
              dst_data, dst_linesize);
    sws_freeContext(pSwsCtx);
    return 0;
}


void play(){

    //-------------------------相关变量初始化-------------------//
    int ret = 0,i = 0;
    int videoStream = -1;
    AVPacket *pPacket;
    AVFrame *pFrame;
    int got_picture;
    mFlagStop = false;
    mFlayPause = false;
    //-----------------------------必须有，初始化工作-------------//
    av_register_all();
    avformat_network_init();

    LOGD("inputUrl = %s",inputUrl);
    m_iformatCtx = avformat_alloc_context();
    ret = avformat_open_input(&m_iformatCtx,inputUrl,NULL,NULL);
    if(ret <0){
        char temp[30] = {0};
        av_strerror(ret,temp,30);
        LOGE("open inpput fail! %d - %s",ret,temp);
        return;
    }
    ret = avformat_find_stream_info(m_iformatCtx,NULL);
    if(ret<0){
        LOGE("Couldn't find stream info !");
        goto error;
    }
    videoStream = -1;
    for(i = 0;i<m_iformatCtx->nb_streams;i++){
        if(m_iformatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStream = i;
            break;
        }
    }
    if(videoStream == -1){
        LOGE("Didn't find video stream !");
        goto error;
    }
    m_icondecCtx = m_iformatCtx->streams[videoStream]->codec;
    //-------------------------------查找解码器--------------------------//
    m_icodec = avcodec_find_decoder(m_icondecCtx->codec_id);
    if(!m_icodec){
        LOGE("Codec not find !");
        goto error;
    }
    //-------------------------------打开解码器--------------------------//
    ret = avcodec_open2(m_icondecCtx,m_icodec,NULL);
    if(ret <0){
        LOGE("codec open fail!");
        goto error;
    }

    if ((m_icondecCtx->width > 0) && (m_icondecCtx->height > 0)) {
        set_BuffersGeometry(m_icondecCtx->width,
                            m_icondecCtx->height);
    }
    pPacket = av_packet_alloc();
    av_init_packet(pPacket);
    pFrame = av_frame_alloc();
    while(!mFlagStop){
        ret = av_read_frame(m_iformatCtx,pPacket);
        if(ret <0){
            LOGE("read error!");
            break;
        }
        ret = avcodec_decode_video2(m_icondecCtx,pFrame,&got_picture,pPacket);
        if(got_picture == 1){
            uint8_t *dst_data[8];
            int dst_linesize[8];
            av_image_alloc(dst_data,dst_linesize,m_icondecCtx->width,m_icondecCtx->height,
                           AV_PIX_FMT_RGB565,16);
            img_convert(dst_data,dst_linesize, AV_PIX_FMT_RGB565, (AVPicture *) pFrame,
                        m_icondecCtx->pix_fmt,
                        m_icondecCtx->width,
                        m_icondecCtx->height);
            render_surface(dst_data[0]);
            av_freep(dst_data);
        }
    }
    error:
    if(m_icondecCtx){
        avcodec_close(m_icondecCtx);
        m_icondecCtx = NULL;
    }
    if(m_iformatCtx){
        avformat_close_input(&m_iformatCtx);
        m_iformatCtx = NULL;
    }
    avformat_network_deinit();
}
/*
 * Class:     com_example_ffplayer_FFSurfaceView
 * Method:    setSurface
 * Signature: (Landroid/view/Surface;)V
 */
JNIEXPORT void JNICALL Java_com_example_ffplayer_FFSurfaceView_setSurface
        (JNIEnv *env, jobject obj, jobject surface){
    if(NULL == surface){
        LOGE("surface is null, destory?");
        mANativeWindow = NULL;
        return;
    }
    mANativeWindow = ANativeWindow_fromSurface(env,surface);
    LOGV("mANativeWindow ok");
    return;
}

void *play_thread(void *argv){
    play();
    return NULL;
}

/*
 * Class:     com_example_ffplayer_FFSurfaceView
 * Method:    start
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_example_ffplayer_FFSurfaceView_start
        (JNIEnv *env, jobject obj, jstring url){
    const char *inputU = env->GetStringUTFChars(url, nullptr);
    sprintf(inputUrl,"%s",inputU);
    pthread_t pthread;
    pthread_create(&pthread,NULL,play_thread,NULL);
    env->ReleaseStringUTFChars(url,inputU);
    env->DeleteLocalRef(url);
}

/*
 * Class:     com_example_ffplayer_FFSurfaceView
 * Method:    pause
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffplayer_FFSurfaceView_pause
        (JNIEnv *, jobject){
    mFlayPause = true;
}

/*
 * Class:     com_example_ffplayer_FFSurfaceView
 * Method:    stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffplayer_FFSurfaceView_stop
        (JNIEnv *, jobject){
    mFlagStop = true;
}

/*
 * Class:     com_example_ffplayer_FFSurfaceView
 * Method:    resume
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffplayer_FFSurfaceView_resume
        (JNIEnv *, jobject){
    mFlayPause = false;
}
