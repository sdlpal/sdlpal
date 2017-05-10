package io.github.sdlpal;

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

    public static native void setAppPath(String basepath, String datapath, String cachepath);
    public static native void setScreenSize(int width, int height);

    public static boolean crashed = false;

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

    @Override
    public void onCreate(Bundle savedInstanceState) {  
        super.onCreate(savedInstanceState);

        String dataPath = getApplicationContext().getFilesDir().getPath();
        String cachePath = getApplicationContext().getCacheDir().getPath();
        String sdcardState = Environment.getExternalStorageState();
        if (sdcardState.equals(Environment.MEDIA_MOUNTED)){
            setAppPath(Environment.getExternalStorageDirectory().getPath() + "/sdlpal", dataPath, cachePath);
        } else {
            setAppPath("/sdcard/sdlpal", dataPath, cachePath);
        }

        DisplayMetrics metrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(metrics);
        setScreenSize(metrics.widthPixels, metrics.heightPixels);

        File runningFile = new File(cachePath + "/running");
        crashed = runningFile.exists();

        if (SettingsActivity.loadConfigFile() || crashed) {
            runningFile.delete();

            Intent intent = new Intent(this, SettingsActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            startActivity(intent);
            finish();
        }
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
