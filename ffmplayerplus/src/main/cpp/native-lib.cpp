#include <jni.h>
#include <string>
#include "android_log.h"
#include "com_example_ffmplayerplus_FFSurfaceView.h"
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
char captureDir[200] = {0};
bool mFlagStop = false;
bool mFlayPause = false;
bool mIsCapture = false;

//截图传参结构体
struct CAPTURE_STU{
    AVFrame *pFrame;
    uint8_t *dataY;
    uint8_t *dataU;
    uint8_t *dataV;
    int width;
    int height;
};

void *capture_thread(void *argv);
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
 * 渲染surface
 * @param pixel
 */
void render_surface2(uint8_t *pixel,int linesize) {
//    LOGV("MonkeyDemo:  renderSurface");
    if (mFlayPause) {
        return;
    }

    ANativeWindow_acquire(mANativeWindow);
//    LOGV("开始渲染");
    if (0 != ANativeWindow_lock(mANativeWindow, &nwBuffer, NULL)) {
        LOGV("ANativeWindow_lock() error");
        return;
    }
// 获取stride
    uint8_t *dst = (uint8_t *) nwBuffer.bits;
    int dstStride = nwBuffer.stride * 4;

    // 由于window的stride和帧的stride不同,因此需要逐行复制
    int h;
    for (h = 0; h < nwBuffer.height; h++) {
        memcpy(dst + h * dstStride, pixel + h * linesize, linesize);
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
    int32_t format = WINDOW_FORMAT_RGBA_8888;

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
    AVDictionary* options = NULL;
    av_dict_set(&options, "buffer_size", "1024000", 0);
    av_dict_set(&options, "max_delay", "500000", 0);
    av_dict_set(&options, "stimeout", "20000000", 0);  //设置超时断开连接时间
    av_dict_set(&options, "rtsp_transport", "tcp", 0);  //以udp方式打开，如果以tcp方式打开将udp替换为tcp

    ret = avformat_open_input(&m_iformatCtx,inputUrl,NULL,&options);
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
            if (mIsCapture) {
                int width = m_icondecCtx->width;
                int height = m_icondecCtx->height;
                AVFrame *pFrame420 = av_frame_alloc();
                int byteSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,width,height,1);
                uint8_t *buffer = (uint8_t*) av_malloc(byteSize);
                av_image_fill_arrays(pFrame420->data,pFrame420->linesize,buffer,AV_PIX_FMT_YUV420P,width,height,1);
                SwsContext *context = sws_getContext(width,
                                                    height,
                                                    m_icondecCtx->pix_fmt,
                                                    width,
                                                    height,
                                                    AV_PIX_FMT_YUV420P,
                                                    SWS_BICUBIC,NULL,NULL,NULL);
                sws_scale(context, (const uint8_t *const *) pFrame->data, pFrame->linesize, 0,height, pFrame420->data, pFrame420->linesize);
                sws_freeContext(context);
                int y_length = width * height;
                LOGV("width = %d :height = %d",width,height);
                int u_length = y_length / 4;
                int v_length = u_length;
                uint8_t *y = (uint8_t *) malloc(sizeof(uint8_t) * y_length);
                uint8_t *u = (uint8_t *) malloc(sizeof(uint8_t) * u_length);
                uint8_t *v = (uint8_t *) malloc(sizeof(uint8_t) * v_length);
                memcpy(y, pFrame420->data[0], y_length );
                memcpy(u, pFrame420->data[1], u_length );
                memcpy(v, pFrame420->data[2], v_length );
                //释放无用内存
                av_frame_free(&pFrame420);
                av_free(buffer);
                CAPTURE_STU capture_stu;
                capture_stu.dataY = y;
                capture_stu.dataU = u;
                capture_stu.dataV = v;
                capture_stu.width = m_icondecCtx->width;
                capture_stu.height = m_icondecCtx->height;
                pthread_t pthreadCapture;
                pthread_create(&pthreadCapture, NULL, capture_thread, (void *) &capture_stu);
                mIsCapture = false;
            }
            uint8_t *dst_data[8];
            int dst_linesize;
            av_image_alloc(dst_data,&dst_linesize,m_icondecCtx->width,m_icondecCtx->height,
                           AV_PIX_FMT_RGBA,16);
            img_convert(dst_data,&dst_linesize, AV_PIX_FMT_RGBA, (AVPicture *) pFrame,
                        m_icondecCtx->pix_fmt,
                        m_icondecCtx->width,
                        m_icondecCtx->height);
            render_surface2(dst_data[0],dst_linesize);
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
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_FFSurfaceView_setSurface
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

/**
 * 截屏
 * @param pFrame
 */
int capture(CAPTURE_STU *pCapture_stu) {
    AVFormatContext* pFormatCtx;
    AVOutputFormat* fmt;
    AVStream* video_st;
    AVCodecContext* pCodecCtx;
    AVCodec* pCodec;
    uint8_t* picture_buf;
    AVFrame* picture;
    AVPacket pkt;
    int y_size;
    int got_picture=0;
    int size;

    int ret=0;

//    FILE *in_file = NULL;                            //YUV source
    int in_w=pCapture_stu->width,in_h=pCapture_stu->height;                           //YUV's width and height
    LOGV("width = %d;height = %d",pCapture_stu->width,pCapture_stu->height);
    av_register_all();

    //Method 1
    pFormatCtx = avformat_alloc_context();
    //Guess format
    fmt = av_guess_format("mjpeg", NULL, NULL);
    pFormatCtx->oformat = fmt;
    //Output URL
    LOGV("outputFile = %s",captureDir);
    if (avio_open(&pFormatCtx->pb,captureDir, AVIO_FLAG_READ_WRITE) < 0){
        LOGV("Couldn't open output file.");
        return -1;
    }

    video_st = avformat_new_stream(pFormatCtx, 0);
    if (video_st==NULL){
        return -1;
    }
    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;

    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;

    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    //Output some information
    av_dump_format(pFormatCtx, 0, captureDir, 1);

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec){
//        printf("Codec not found.");
        LOGV("Codec not found.");
        return -1;
    }
    int ret1 = avcodec_open2(pCodecCtx,pCodec,NULL);
    if (ret1 < 0){
        printf("Could not open codec.");
        char temp[20]  = {0};
        av_make_error_string(temp,20,ret1);
        LOGV("Could not open codec.%s",temp);
        return -1;
    }
    picture = av_frame_alloc();
    size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t *)av_malloc(size);
    if (!picture_buf)
    {
        return -1;
    }
    avpicture_fill((AVPicture *)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

    //Write Header
    avformat_write_header(pFormatCtx,NULL);

    y_size = pCodecCtx->width * pCodecCtx->height;
    av_new_packet(&pkt,y_size*3);
    picture->data[0] = pCapture_stu->dataY;              // Y
    picture->data[1] = pCapture_stu->dataU;      // U
    picture->data[2] = pCapture_stu->dataV;  // V
    //Encode
    LOGV("111");
    ret = avcodec_encode_video2(pCodecCtx, &pkt,picture, &got_picture);
    LOGV("222");
    if(ret < 0){
        printf("Encode Error.\n");
        LOGV("Encode Error.\n");
        return -1;
    }
    if (got_picture==1){
        pkt.stream_index = video_st->index;
        ret = av_write_frame(pFormatCtx, &pkt);
    }
//    free(pCapture_stu->dataY);
//    free(pCapture_stu->dataU);
//    free(pCapture_stu->dataV);
    av_free_packet(&pkt);
    //Write Trailer
    av_write_trailer(pFormatCtx);

    printf("Encode Successful.\n");
    LOGV("Encode Successful.\n");

    if (video_st){
        avcodec_close(video_st->codec);
        av_free(picture);
        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    return 0;
}
/**
 * 截图线程
 * @param argv
 * @return
 */
void *capture_thread(void *argv){
    CAPTURE_STU *p = (CAPTURE_STU *)argv;
    if(p){
        capture(p);
    }
    return NULL;
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
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_FFSurfaceView_start
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
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_FFSurfaceView_pause
        (JNIEnv *, jobject){
    mFlayPause = true;
}

/*
 * Class:     com_example_ffplayer_FFSurfaceView
 * Method:    stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_FFSurfaceView_stop
        (JNIEnv *, jobject){
    mFlagStop = true;
}

/*
 * Class:     com_example_ffplayer_FFSurfaceView
 * Method:    resume
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_FFSurfaceView_resume
        (JNIEnv *, jobject){
    mFlayPause = false;
}

/*
 * Class:     com_example_ffmplayerplus_FFSurfaceView
 * Method:    capture
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_FFSurfaceView_capture
        (JNIEnv *env, jobject, jstring fileName){
    char *file = (char *) env->GetStringUTFChars(fileName, nullptr);
    sprintf(captureDir,"%s",file);
    mIsCapture = true;
    env->ReleaseStringUTFChars(fileName,file);
    env->DeleteLocalRef(fileName);
}
