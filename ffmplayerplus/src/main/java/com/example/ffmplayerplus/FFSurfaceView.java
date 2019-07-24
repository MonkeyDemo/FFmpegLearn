package com.example.ffmplayerplus;


import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.io.File;

/**
 * Created by Easysight on 2019/1/9.
 */

public class FFSurfaceView extends SurfaceView implements SurfaceHolder.Callback{
    String mUrl;
    static {
        System.loadLibrary("ffbackSdk");
    }
    public FFSurfaceView(Context context) {
        super(context);
        init();
    }

    public FFSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public FFSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }
    protected native void setSurface(Surface holder);
    protected native void start(String url);
    protected native void pause();
    protected native void stop();
    protected native void resume();
    protected native void capture(String picFileName);
    protected native void startRecode(String mp4FileName);
    protected native void stopRecode();

    protected void init(){
        this.getHolder().addCallback(this);
    }
    public void startVideo(String url){
        this.mUrl = url;
        this.start(url);
    }
    public void pauseVideo(){
        this.pause();
    }
    public void stopVideo(){
        this.stop();
    }
    public void resumeVideo(){
        this.resume();
    }

    public void captureVideo(String picFileName){
        Log.d("MonkeyDemo","pictureDir = "+picFileName);
        File picFile = new File(picFileName);
        File picDir = picFile.getParentFile();
        if(picDir != null && picDir.exists() && picDir.isDirectory()){
            this.capture(picFileName);
        }else {
            throw new IllegalArgumentException("传入的图片文件名不合法!");
        }
    }
    public void recodeStart(String mp4FileName){
        Log.d("MonkeyDemo","mp4Dir = "+mp4FileName);
        File mp4File = new File(mp4FileName);
        File mp4Dir = mp4File.getParentFile();
        if(mp4Dir != null && mp4Dir.exists() && mp4Dir.isDirectory()){
            this.startRecode(mp4FileName);
        }else {
            throw new IllegalArgumentException("传入的mp4文件名不合法!");
        }
    }
    public void recodeStop(){
        this.stopRecode();
    }
    public void setSurfaceTo(Surface surface){
        this.setSurface(surface);
    }
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        setSurface(holder.getSurface());
//        if(!TextUtils.isEmpty(mUrl)){
//            startVideo(mUrl);
//        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stopVideo();
    }
}
