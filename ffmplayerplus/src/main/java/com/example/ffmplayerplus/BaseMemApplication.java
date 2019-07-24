package com.example.ffmplayerplus;

import android.app.ActivityManager;
import android.app.Application;
import android.os.Debug;
import android.util.Log;

/**
 * Created by Easysight on 2019/5/27.
 */

public class BaseMemApplication extends Application {

    private int sleepTime = 10000;
    @Override
    public void onCreate() {
        super.onCreate();
        startMemCollect();
    }

    private void startMemCollect(){
        new Thread(new Runnable() {
            @Override
            public void run() {
                while (true){
                    getMemorySize();
                    getMemoryNativeSize();
                    try {
                        Thread.sleep(sleepTime);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }).start();
    }
    private void getMemorySize(){
        ActivityManager activityManager=(ActivityManager) getSystemService(this.ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo memInfo=new ActivityManager.MemoryInfo();
        activityManager.getMemoryInfo(memInfo);
        Log.v("memInfo", "availMem:"+memInfo.availMem/1024+" kb");
        Log.v("memInfo", "threshold:"+memInfo.threshold/1024+" kb");//low memory threshold
        Log.v("memInfo", "totalMem:"+memInfo.totalMem/1024+" kb");
        Log.v("memInfo", "lowMemory:"+memInfo.lowMemory);  //if current is in low memory
    }

    /**
     * 获取native内存
     */
    private void getMemoryNativeSize() {
        long sizeTotal = Debug.getNativeHeapSize();
        long sizeAllocate = Debug.getNativeHeapAllocatedSize();
        long sizeFree = Debug.getNativeHeapFreeSize();
        double sizeTotalD = (double) sizeTotal/1024;
        double sizeAllocateD = (double) sizeAllocate/1024;
        double sizeFreeD = (double)sizeFree/1024;
        Log.v("memInfoNative", "allocateMem:"+sizeAllocateD+" kb");//low memory threshold
        Log.v("memInfoNative", "totalMem:"+sizeTotalD+" kb");
        Log.v("memInfoNative", "freeMem:"+sizeFreeD+"kb");  //if current is in low memory
    }

    protected void setSleepTime(int time){
        this.sleepTime = time;
    }

    protected int getSleepTime(){
        return this.sleepTime;
    }
}
