package com.example.ffmplayerplus;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import java.io.File;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {
    FFSurfaceView surfaceView;
//    String url = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";
        String url = "rtsp://admin:admin222@192.168.1.64:554/h264/ch1/main/av_stream";
//    String url = "rtsp://192.168.1.10:554/user=admin&password=&channel=1&stream=0.sdp?";
    Button btnRecode;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        initView();
    }

    private void initView() {
        surfaceView = (FFSurfaceView) findViewById(R.id.surface_view);
        findViewById(R.id.btn_start).setOnClickListener(this);
        findViewById(R.id.btn_pause).setOnClickListener(this);
        findViewById(R.id.btn_resume).setOnClickListener(this);
        findViewById(R.id.btn_stop).setOnClickListener(this);
        findViewById(R.id.btn_jietu).setOnClickListener(this);
        btnRecode = (Button) findViewById(R.id.btn_luzhi);
        btnRecode.setOnClickListener(this);
    }


    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.btn_start:
                surfaceView.startVideo(url);
                break;
            case R.id.btn_pause:
                surfaceView.pauseVideo();
                break;
            case R.id.btn_resume:
                surfaceView.resumeVideo();
                break;
            case R.id.btn_stop:
                surfaceView.stopVideo();
                break;
            case R.id.btn_jietu:
                String dir = Environment.getExternalStorageDirectory().getAbsolutePath();
                File file = new File(dir+"/MonkeyDemo/testCapture");
                if(!file.exists()){
                    file.mkdirs();
                }
                String str = file.getAbsolutePath()+"/"+System.currentTimeMillis()+".jpg";
                surfaceView.captureVideo(str);
                break;
            case R.id.btn_luzhi:
                break;
        }
    }
}
