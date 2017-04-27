package com.codeplex.sdlpal;

import org.libsdl.app.SDLActivity;
import android.os.*;
import android.util.*;
import android.media.*;
import android.net.Uri;
import java.io.*;

public class PalActivity extends SDLActivity {
    private static final String TAG = "sdlpal-debug";
    private static final MediaPlayer mediaPlayer = new MediaPlayer();
    private static Uri interFileURI;

    private static void JNI_mediaplayer_load(){
        Log.v(TAG, "loading midi:" + interFileURI);
        mediaPlayer.reset();
        mediaPlayer.setLooping(true);
        try{
            mediaPlayer.setDataSource(mSingleton.getApplicationContext(), interFileURI);
            mediaPlayer.prepare();
        }catch(IOException e) {
            Log.e(TAG, interFileURI.toString() + " not available for playing, check");
        }
    }

    private static void JNI_mediaplayer_play() {
        mediaPlayer.start();
    }

    private static void JNI_mediaplayer_stop() {
        mediaPlayer.stop();
    }

    private static int JNI_mediaplayer_playing() {
        return mediaPlayer.isPlaying() ? 1 : 0;
    }

    private static void JNI_mediaplayer_setvolume(int volume) {
        mediaPlayer.setVolume((float)volume/256, (float)volume/256);
    }

    public static native void setExternalStorage(String str);
    public static native void setMIDIInterFile(String str);
    @Override  
    public void onCreate(Bundle savedInstanceState) {  
        super.onCreate(savedInstanceState);
        String appDataPath = mSingleton.getApplicationContext().getFilesDir().getPath();
        String interFilePath = appDataPath+"/intermediates.midi";
        Log.v(TAG, "java interfile path " + interFilePath);
        interFileURI = Uri.fromFile(new File(interFilePath));
        setMIDIInterFile(interFilePath);
        String externalStorageState = Environment.getExternalStorageState();
        if(externalStorageState.equals(Environment.MEDIA_MOUNTED)){
            setExternalStorage(Environment.getExternalStorageDirectory().getPath());
            Log.v(TAG, "sdcard path " + Environment.getExternalStorageDirectory().getPath());
        }
    }
}
