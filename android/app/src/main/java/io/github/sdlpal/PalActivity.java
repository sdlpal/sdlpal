package io.github.sdlpal;

import org.libsdl.app.SDLActivity;
import android.os.*;
import android.util.*;
import android.media.*;
import android.net.Uri;
import java.io.*;
import java.util.*;

public class PalActivity extends SDLActivity {
    private static final String TAG = "sdlpal-debug";
    private static MediaPlayer mediaPlayer;
    private static int screenWidth, screenHeight;

    private static MediaPlayer JNI_mediaplayer_load(String filename){
        Log.v(TAG, "loading midi:" + filename);
        MediaPlayer mediaPlayer = new MediaPlayer();
        mediaPlayer.reset();
        try {
            mediaPlayer.setDataSource(mSingleton.getApplicationContext(), Uri.fromFile(new File(filename)));
            mediaPlayer.prepare();
        } catch(IOException e) {
            Log.e(TAG, filename + " not available for playing, check");
        }
        PalActivity.mediaPlayer = mediaPlayer;
        return mediaPlayer;
    }

    public static native void setExternalStorage(String str);
    public static native void setMIDIInterFile(String str);

    @Override
    public void onCreate(Bundle savedInstanceState) {  
        super.onCreate(savedInstanceState);
        String interFilePath = mSingleton.getApplicationContext().getCacheDir().getPath() + "/intermediates.mid";
        Log.v(TAG, "java interfile path " + interFilePath);
        setMIDIInterFile(interFilePath);
        String externalStorageState = Environment.getExternalStorageState();
        if (externalStorageState.equals(Environment.MEDIA_MOUNTED)){
            setExternalStorage(Environment.getExternalStorageDirectory().getPath());
            Log.v(TAG, "sdcard path " + Environment.getExternalStorageDirectory().getPath());
        }
        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        screenWidth = metrics.widthPixels;
        screenHeight = metrics.heightPixels;
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
