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
        System.loadLibrary("native-lib");
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
    private native void setSurface(Surface holder);
    private native void start(String url);
    private native void pause();
    private native void stop();
    private native void resume();
    private native void capture(String picFileName);
    private native void startRecode();
    private native void stopRecode();

    private void init(){
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
        Log.d("MonkeyDemo","dir = "+picFileName);
        File picFile = new File(picFileName);
        File picDir = picFile.getParentFile();
        if(picDir != null && picDir.exists() && picDir.isDirectory()){
            this.capture(picFileName);
        }else {
            throw new IllegalArgumentException("传入的文件名不合法!");
        }
    }
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        setSurface(holder.getSurface());
        if(!TextUtils.isEmpty(mUrl)){
            startVideo(mUrl);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stopVideo();
    }
}
