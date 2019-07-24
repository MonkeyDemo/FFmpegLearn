package com.example.ffmplayerplus;

import android.view.Surface;

/**
 * Created by Easysight on 2019/3/15.
 */

public class ESNetSdk {
    static ESNetSdk netSdk = null;

    private ESNetSdk(){
        try{
            System.loadLibrary("ffbackSdk");
        }catch (UnsatisfiedLinkError var1){

        }
    }

    public static ESNetSdk getInstance(){
        if(null == netSdk){
            synchronized (ESNetSdk.class){
                if(null == netSdk){
                    netSdk = new ESNetSdk();
                }
            }
        }
        return netSdk;
    }

    public native void ES_NET_setSurface(Surface holder);
    public native void ES_NET_start(String url);
    public native void ES_NET_pause();
    public native void ES_NET_stop();
    public native void ES_NET_resume();
    public native void ES_NET_capture(String picFileName);
    public native void ES_NET_startRecode(String mp4FileName);
    public native void ES_NET_stopRecode();
}
