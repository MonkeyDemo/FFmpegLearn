package com.example.ffmplayerplus;


/**
 * Created by Easysight on 2019/4/10.
 */

public class ESNetSdkRecode {
    static ESNetSdkRecode netSdk = null;

    private ESNetSdkRecode(){
        try{
            System.loadLibrary("recode2mp4");
        }catch (UnsatisfiedLinkError var1){

        }
    }

    public static ESNetSdkRecode getInstance(){
        if(null == netSdk){
            synchronized (ESNetSdk.class){
                if(null == netSdk){
                    netSdk = new ESNetSdkRecode();
                }
            }
        }
        return netSdk;
    }

    public native void ES_NET_Recode_start(String url);
    public native void ES_NET_Recode_startRecode(String url,String mp4FileName);
    public native void ES_NET_Recode_stopRecode();
}
