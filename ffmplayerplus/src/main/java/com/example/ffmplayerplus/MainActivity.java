package com.example.ffmplayerplus;

import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;

import java.io.File;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    FFSurfaceView surfaceViewLeft;
    SurfaceView surfaceViewRight;
//    String url = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";
//        String url = "rtsp://admin:admin@192.168.1.16:554/h264/ch1/main/av_stream";
    String url = "rtsp://192.168.1.168:554/sub";
//    String url = "rtsp://admin:admin222@192.168.1.64:554/h264/ch1/main/av_stream";
//    String url = "rtsp://192.168.1.10:554/user=admin&password=&channel=1&stream=0.sdp?";
    Button btnRecode;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initView();
    }

    private Handler handler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case 0x01:
                    //左
                    ESNetSdk.getInstance().ES_NET_setSurface(surfaceViewLeft.getHolder().getSurface());
                    Log.d("MonkeyDemo222","setSurfaceFinish");
                    ESNetSdk.getInstance().ES_NET_start(url);
                    break;
                case 0x02:
                    //右
                    ESNetSdk.getInstance().ES_NET_setSurface(surfaceViewRight.getHolder().getSurface());
                    Log.d("MonkeyDemo222","setSurfaceFinish");
                    ESNetSdk.getInstance().ES_NET_start(url);
                    break;
            }
        }
    };

    private void initView() {
        surfaceViewLeft = (FFSurfaceView) findViewById(R.id.surface_view_left);
        ESNetSdk.getInstance().ES_NET_setSurface(surfaceViewLeft.getHolder().getSurface());
        surfaceViewRight = (SurfaceView) findViewById(R.id.surface_view_right);
        findViewById(R.id.btn_start).setOnClickListener(this);
        findViewById(R.id.btn_pause).setOnClickListener(this);
        findViewById(R.id.btn_resume).setOnClickListener(this);
        findViewById(R.id.btn_stop).setOnClickListener(this);
        findViewById(R.id.btn_jietu).setOnClickListener(this);
        findViewById(R.id.btn_exchange).setOnClickListener(this);
        btnRecode = (Button) findViewById(R.id.btn_luzhi);
        btnRecode.setOnClickListener(this);
    }


    boolean isRecode = false;
    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.btn_start:
//                surfaceView.startVideo(url);
                ESNetSdk.getInstance().ES_NET_start(url);
                break;
            case R.id.btn_pause:
//                surfaceView.pauseVideo();
                ESNetSdk.getInstance().ES_NET_pause();
                break;
            case R.id.btn_resume:
//                surfaceView.resumeVideo();
                ESNetSdk.getInstance().ES_NET_resume();
                break;
            case R.id.btn_stop:
//                surfaceView.stopVideo();
                ESNetSdk.getInstance().ES_NET_stop();
                break;
            case R.id.btn_jietu:
                String dir = Environment.getExternalStorageDirectory().getAbsolutePath();
                File file = new File(dir+"/MonkeyDemo/testCapture");
                if(!file.exists()){
                    file.mkdirs();
                }
                String str = file.getAbsolutePath()+"/"+System.currentTimeMillis()+".jpg";
//                surfaceView.captureVideo(str);
                ESNetSdk.getInstance().ES_NET_capture(str);
                break;
            case R.id.btn_luzhi:
                String dir2= Environment.getExternalStorageDirectory().getAbsolutePath();
                File file2 = new File(dir2+"/MonkeyDemo/testRecode");
                if(!file2.exists()){
                    file2.mkdirs();
                }
                String str2 = file2.getAbsolutePath()+"/"+System.currentTimeMillis()+".mp4";
                if(!isRecode){
//                    surfaceView.recodeStart(str2);
//                    ESNetSdk.getInstance().ES_NET_startRecode(str2);
//                    ESNetSdkRecode.getInstance().ES_NET_Recode_start(url);
//                    try {
//                        Thread.sleep(100);
//                    } catch (InterruptedException e) {
//                        e.printStackTrace();
//                    }
                    ESNetSdkRecode.getInstance().ES_NET_Recode_startRecode(url,str2);
                    isRecode = true;
                }else{
//                    surfaceView.recodeStop();
//                    ESNetSdk.getInstance().ES_NET_stopRecode();
                    ESNetSdkRecode.getInstance().ES_NET_Recode_stopRecode();
                    isRecode = false;
                }
                break;
            case R.id.btn_exchange:
                exchangeSurfaceView();
                break;
        }
    }

    private int flag = 1; // 0-左边 1-右边
    private void exchangeSurfaceView() {
        if(flag ==0){
            //切换到左边
            ESNetSdk.getInstance().ES_NET_stop();
            Log.d("MonkeyDemo222","stopFinish()");
            handler.sendEmptyMessageDelayed(0x01,100);
            Log.d("MonkeyDemo222","startFinish");
            flag = 1;
        }else if(flag == 1){
            //切换到右边
            ESNetSdk.getInstance().ES_NET_stop();
            Log.d("MonkeyDemo222","stopFinish()");
            handler.sendEmptyMessageDelayed(0x02,100);
            Log.d("MonkeyDemo222","startFinish");
            flag = 0;
        }
    }
}
