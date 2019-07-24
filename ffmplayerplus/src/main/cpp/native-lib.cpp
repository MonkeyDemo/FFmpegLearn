#include <jni.h>
#include <string>
#include "android_log.h"
#include "com_example_ffmplayerplus_FFSurfaceView.h"
#include "com_example_ffmplayerplus_ESNetSdk.h"
#include <pthread.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <string>
#include <deque>
using namespace std;
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


char inputUrl[200] = {0};
char captureDir[200] = {0};
char recodeDir[200] = {0};
bool mFlagStop = false;
bool mFlayPause = false;
bool mIsCapture = false; //截图
bool mIsRecode = false; //录制
int m_width,m_height;
//截图传参类
class FrameData{
public:
    FrameData(int width,int height){
        this->width = width;
        this->height = height;
        int y_length = this->width*this->height;
        int uv_length = y_length/4;
        this->y_length = y_length;
        this->uv_length = uv_length;
        this->dataY = (uint8_t *) malloc(sizeof(uint8_t) * y_length);
        this->dataU = (uint8_t *)malloc(sizeof(uint8_t)*uv_length);
        this->dataV = (uint8_t *)malloc(sizeof(uint8_t)*uv_length);
    }

    ~FrameData(){
        if(this->dataY){
            free(this->dataY);
        }
        if(this->dataU){
            free(this->dataU);
        }
        if(this->dataV){
            free(this->dataV);
        }
    }
    int width;
    int height;
    int y_length;
    int uv_length;
    uint8_t *dataY;
    uint8_t *dataU;
    uint8_t *dataV;
    void copyFromAvFrame(AVFrame *avFrame){
        memcpy(this->dataY,avFrame->data[0], this->y_length);
        memcpy(this->dataU,avFrame->data[1], this->uv_length);
        memcpy(this->dataV,avFrame->data[2], this->uv_length);
    }
    void copyToAvFrame(AVFrame *avFrame){
        memcpy(avFrame->data[0],this->dataY, this->y_length);
        memcpy(avFrame->data[1],this->dataU, this->uv_length);
        memcpy(avFrame->data[2],this->dataV, this->uv_length);
    }
};
deque<FrameData*> mFrameQue;
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
    if (mFlayPause) {
        return;
    }
    if(mANativeWindow == NULL){
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
    LOGD("playstart");
    //-------------------------相关变量初始化-------------------//
    int ret = 0,i = 0;
    int videoStream = -1;
    AVFormatContext *m_iformatCtx;
    AVCodecContext *m_icondecCtx;
    AVCodec *m_icodec;
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
        goto error;
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
    m_width = m_icondecCtx->width;
    m_height = m_icondecCtx->height;
    pPacket = av_packet_alloc();
    av_init_packet(pPacket);
    pFrame = av_frame_alloc();
    while(!mFlagStop){
        ret = av_read_frame(m_iformatCtx,pPacket);
        if(ret <0){
            char temp[30] = {0};
            av_strerror(ret,temp,30);
            LOGE("read error!%s",temp);
            break;
        }
        if(pPacket->flags == AV_PKT_FLAG_DISCARD || pPacket->flags ==  AV_PKT_FLAG_CORRUPT){
            //丢弃或损坏的帧
            av_packet_unref(pPacket);
            continue;
        }
        ret = avcodec_decode_video2(m_icondecCtx,pFrame,&got_picture,pPacket);
        if(got_picture == 1){
            if (mIsCapture || mIsRecode) {
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
                LOGV("width = %d ------height = %d",width,height);
                if(mIsCapture){
                    FrameData *data = new FrameData(width,height);
                    data->copyFromAvFrame(pFrame420);
                    pthread_t pthreadCapture;
                    pthread_create(&pthreadCapture, NULL, capture_thread, (void *) data);
                    mIsCapture = false;
                }
                if(mIsRecode){
                    FrameData *data = new FrameData(width,height);
                    data->copyFromAvFrame(pFrame420);
                    mFrameQue.push_front(data);
                    long size = mFrameQue.size();
                    LOGV("queSize = %d",size);
                }
                //释放无用内存
                av_frame_free(&pFrame420);
                av_free(buffer);
            }
            uint8_t *dst_data[4];
            int dst_linesize;
            av_image_alloc(dst_data,&dst_linesize,m_icondecCtx->width,m_icondecCtx->height,
                           AV_PIX_FMT_RGBA,16);
            img_convert(dst_data,&dst_linesize, AV_PIX_FMT_RGBA, (AVPicture *) pFrame,
                        m_icondecCtx->pix_fmt,
                        m_icondecCtx->width,
                        m_icondecCtx->height);
            render_surface2(dst_data[0],dst_linesize);
            av_freep(&dst_data[0]);
        }
        av_packet_unref(pPacket);
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
    if(m_iformatCtx){
        avformat_free_context(m_iformatCtx);
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
int capture(FrameData *pCapture_stu) {
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
    pCodecCtx->qcompress = 1;
    pCodecCtx->qmin = 2;
    pCodecCtx->qmax = 31;
    pCodecCtx->max_qdiff = 3;
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
    pCapture_stu->copyToAvFrame(picture);
    //Encode
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
    delete pCapture_stu;
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

int recode(int width,int height){
    AVFormatContext *pFormatCtx;
    AVOutputFormat *fmt;
    AVStream *video_st;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame;
    uint8_t *buffer;
    AVPacket pkt;

    int ret =0;
    int in_w,in_h;
    in_w = width,in_h = height;
    av_register_all();
    pFormatCtx = avformat_alloc_context();
    fmt = av_guess_format(NULL,recodeDir,NULL);
    pFormatCtx->oformat = fmt;
    if(avio_open(&pFormatCtx->pb,recodeDir,AVIO_FLAG_READ_WRITE)<0){
        LOGV("Couldn't open outputFile!");
        return -1;
    }

    pCodec = avcodec_find_encoder(pFormatCtx->oformat->video_codec);
    if (!pCodec){
//        printf("Codec not found.");
        LOGV("Codec not found.");
        return -1;
    }
    video_st = avformat_new_stream(pFormatCtx,pCodec);
    if(video_st == NULL){
        LOGV("Stream new fail!");
        return -1;
    }
    video_st->codec->codec_tag = 0;
    if(pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER){
        video_st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->gop_size = 10;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;
    pCodecCtx->bit_rate = pCodecCtx->width* pCodecCtx->height;
    pCodecCtx->qcompress = 1;
    pCodecCtx->qmin = 2;
    pCodecCtx->qmax = 31;
    pCodecCtx->max_b_frames = 3;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    AVDictionary* param = 0;
    if(pCodecCtx->codec_id == AV_CODEC_ID_H264){
        av_dict_set(&param, "preset", "superfast", 0);
        av_dict_set(&param,"tune","zerolatency",0);
    }
    //Output some information
    av_dump_format(pFormatCtx, 0, captureDir, 1);
    int ret1 = avcodec_open2(pCodecCtx,pCodec,&param);
    if (ret1 < 0){
        char temp[100]  = {0};
        av_make_error_string(temp,100,ret1);
        LOGV("Could not open codec.%s",temp);
        return -1;
    }
    //Write Header
    avformat_write_header(pFormatCtx,NULL);

//    int y_size = pCodecCtx->width * pCodecCtx->height;
//    av_new_packet(&pkt,y_size*3);
    av_init_packet(&pkt);
    int dpsCount = 0;
    int got_picture;
    pFrame = av_frame_alloc();
    int size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(size);
    if (!buffer)
    {
        return -1;
    }
    avpicture_fill((AVPicture *)pFrame, buffer, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    LOGV("111");
    while(mIsRecode){
        if(mFrameQue.size()>3){
            FrameData *data = mFrameQue.back();
            data->copyToAvFrame(pFrame);
            delete data;
            mFrameQue.pop_back();
            //Encode
            pFrame->pts = dpsCount;
//            pFrame->pkt_duration = 1;
            ret = avcodec_encode_video2(pCodecCtx, &pkt,pFrame, &got_picture);
            if(ret < 0){
                char temp[100]  = {0};
                av_make_error_string(temp,100,ret);
                LOGV("Encode Error. %s\n",temp);
                return -1;
            }
            if (got_picture==1){
                LOGV("保存一帧");
                pkt.stream_index = video_st->index;
                LOGV("videoStream time_base = {%d,%d}",video_st->time_base.num,video_st->time_base.den);
                pkt.pts = dpsCount*((video_st->time_base.den/video_st->time_base.num)/pCodecCtx->time_base.den);
                pkt.dts = pkt.pts;
                pkt.duration = ((video_st->time_base.den/video_st->time_base.num)/pCodecCtx->time_base.den);
                ret = av_write_frame(pFormatCtx, &pkt);
                dpsCount++;
            }
            av_free_packet(&pkt);
        }
    }

    //Write Trailer
    av_write_trailer(pFormatCtx);
    while (mFrameQue.size()>0){
        delete mFrameQue.back();
        mFrameQue.pop_back();
    }
    printf("Encode Successful.\n");
    LOGV("Encode Successful.\n");

    if (video_st){
        avcodec_close(video_st->codec);
        av_free(pFormatCtx);
        av_free(buffer);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
    LOGV("Encode finished ");

    return 0;
}
/**
 * 截图线程
 * @param argv
 * @return
 */
void *capture_thread(void *argv){
//    CAPTURE_STU *p = (CAPTURE_STU *)argv;
    FrameData *p = (FrameData *)argv;
    if(p){
        capture(p);
    }
    return NULL;
}

void *recode_thread(void *argv){
    FrameData *p = (FrameData *)argv;
    int width = p->width,height = p->height;
    LOGV("thread width = %d ,height = %d ",width,height);
    delete p;
    if(p){
        recode(width,height);
    }
//    return 0;
    pthread_exit(NULL);
}

void *play_thread(void *argv){
    play();
    return 0;
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

/*
 * Class:     com_example_ffmplayerplus_FFSurfaceView
 * Method:    startRecode
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_FFSurfaceView_startRecode
        (JNIEnv *env, jobject ,jstring fileName){
    char *file = (char *) env->GetStringUTFChars(fileName, nullptr);
    sprintf(recodeDir,"%s",file);
    mIsRecode = true;
    pthread_t pthreadRecode;
    FrameData *p = new FrameData(m_width,m_height);
    pthread_create(&pthreadRecode,NULL,recode_thread,(void *)p);
    env->ReleaseStringUTFChars(fileName,file);
    env->DeleteLocalRef(fileName);
}

/*
 * Class:     com_example_ffmplayerplus_FFSurfaceView
 * Method:    stopRecode
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_FFSurfaceView_stopRecode
        (JNIEnv *, jobject){
    mIsRecode = false;
}

/*
 * Class:     com_example_ffmplayerplus_ESNetSdk
 * Method:    ES_NET_setSurface
 * Signature: (Landroid/view/Surface;)V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_ESNetSdk_ES_1NET_1setSurface
        (JNIEnv *env, jobject, jobject surface){
    if(NULL == surface){
        LOGE("surface is null, destory?");
        mANativeWindow = NULL;
        return;
    }
    mANativeWindow = ANativeWindow_fromSurface(env,surface);
    LOGV("mANativeWindow ok");
    return;
}

/*
 * Class:     com_example_ffmplayerplus_ESNetSdk
 * Method:    ES_NET_start
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_ESNetSdk_ES_1NET_1start
        (JNIEnv *env, jobject, jstring url){
    const char *inputU = env->GetStringUTFChars(url, nullptr);
    sprintf(inputUrl,"%s",inputU);
    pthread_t pthread;
    pthread_create(&pthread,NULL,play_thread,NULL);
    env->ReleaseStringUTFChars(url,inputU);
    env->DeleteLocalRef(url);
}

/*
 * Class:     com_example_ffmplayerplus_ESNetSdk
 * Method:    ES_NET_pause
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_ESNetSdk_ES_1NET_1pause
        (JNIEnv *, jobject){
    mFlayPause = true;
}

/*
 * Class:     com_example_ffmplayerplus_ESNetSdk
 * Method:    ES_NET_stop
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_ESNetSdk_ES_1NET_1stop
        (JNIEnv *, jobject){
    mFlagStop = true;
}

/*
 * Class:     com_example_ffmplayerplus_ESNetSdk
 * Method:    ES_NET_resume
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_ESNetSdk_ES_1NET_1resume
        (JNIEnv *, jobject){
    mFlayPause = false;
}

/*
 * Class:     com_example_ffmplayerplus_ESNetSdk
 * Method:    ES_NET_capture
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_ESNetSdk_ES_1NET_1capture
        (JNIEnv *env, jobject, jstring fileName){
    char *file = (char *) env->GetStringUTFChars(fileName, nullptr);
    sprintf(captureDir,"%s",file);
    mIsCapture = true;
    env->ReleaseStringUTFChars(fileName,file);
    env->DeleteLocalRef(fileName);
}

/*
 * Class:     com_example_ffmplayerplus_ESNetSdk
 * Method:    ES_NET_startRecode
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_ESNetSdk_ES_1NET_1startRecode
        (JNIEnv *env, jobject, jstring fileName){
    {
        char *file = (char *) env->GetStringUTFChars(fileName, nullptr);
        sprintf(recodeDir,"%s",file);
        mIsRecode = true;
        pthread_t pthreadRecode;
        FrameData *p = new FrameData(m_width,m_height);
        pthread_create(&pthreadRecode,NULL,recode_thread,(void *)p);
        env->ReleaseStringUTFChars(fileName,file);
        env->DeleteLocalRef(fileName);
    }
}

/*
 * Class:     com_example_ffmplayerplus_ESNetSdk
 * Method:    ES_NET_stopRecode
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_example_ffmplayerplus_ESNetSdk_ES_1NET_1stopRecode
        (JNIEnv *, jobject){
    mIsRecode = false;
}
