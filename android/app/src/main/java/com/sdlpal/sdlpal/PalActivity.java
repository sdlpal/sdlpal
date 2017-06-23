package com.sdlpal.sdlpal;

import org.libsdl.app.SDLActivity;

import android.content.Intent;
import android.os.*;
import android.util.*;
import android.media.*;
import android.net.Uri;
import java.io.*;

public class PalActivity extends SDLActivity {
    private static final String TAG = "sdlpal-debug";
    private static MediaPlayer mediaPlayer;

    public static native void setScreenSize(int width, int height);

    public static boolean crashed = false;

    private static MediaPlayer JNI_mediaplayer_load(String filename){
        Log.v(TAG, "loading midi:" + filename);
        if (mediaPlayer == null) {
            mediaPlayer = new MediaPlayer();
        }
        mediaPlayer.reset();
        try {
            mediaPlayer.setDataSource(mSingleton.getApplicationContext(), Uri.fromFile(new File(filename)));
            mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mediaPlayer.prepare();
        } catch(IOException e) {
            Log.e(TAG, filename + " not available for playing, check");
        }
        return mediaPlayer;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {  
        super.onCreate(savedInstanceState);

        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        setScreenSize(metrics.widthPixels, metrics.heightPixels);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        if (!this.isFinishing() && mediaPlayer != null) {
            mediaPlayer.pause();
        }
        super.onPause();
    }

    @Override
    protected void onResume() {
        if (mediaPlayer != null) {
            mediaPlayer.start();
        }
        super.onResume();
    }
}
