package com.example.ffplayer;

import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

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
