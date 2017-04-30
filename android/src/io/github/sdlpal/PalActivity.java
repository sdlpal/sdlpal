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
    private static final MediaPlayer mediaPlayer = new MediaPlayer();

    private static void JNI_mediaplayer_load(String filename){
        Log.v(TAG, "loading midi:" + filename);
        mediaPlayer.reset();
        mediaPlayer.setLooping(true);
        try {
            mediaPlayer.setDataSource(mSingleton.getApplicationContext(), Uri.fromFile(new File(filename)));
            mediaPlayer.prepare();
        } catch(IOException e) {
            Log.e(TAG, filename + " not available for playing, check");
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
        String appDataPath = mSingleton.getApplicationContext().getCacheDir().getPath();
        String interFilePath = appDataPath+"/intermediates.midi";
        Log.v(TAG, "java interfile path " + interFilePath);
        setMIDIInterFile(interFilePath);
        String externalStorageState = Environment.getExternalStorageState();
        if (externalStorageState.equals(Environment.MEDIA_MOUNTED)){
            setExternalStorage(Environment.getExternalStorageDirectory().getPath());
            Log.v(TAG, "sdcard path " + Environment.getExternalStorageDirectory().getPath());
        }
    }

    @Override
    protected void onPause() {
        if (!this.isFinishing()){
            mediaPlayer.pause();
        }
        super.onPause();
    }

    @Override
    protected void onResume() {
        mediaPlayer.start();
        super.onResume();
    }
}
