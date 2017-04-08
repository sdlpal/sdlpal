package com.codeplex.sdlpal;

import org.libsdl.app.SDLActivity;
import android.os.*;
import android.util.*;

public class PalActivity extends SDLActivity {
    public static native void setExternalStorage(String str);  
    @Override  
    public void onCreate(Bundle savedInstanceState) {  
        super.onCreate(savedInstanceState);
//         setContentView(R.layout.main);
        String externalStorageState = Environment.getExternalStorageState();
        if(externalStorageState.equals(Environment.MEDIA_MOUNTED)){
            setExternalStorage(Environment.getExternalStorageDirectory().getPath());
            Log.v("sdlpal-debug", "sdcard path " + Environment.getExternalStorageDirectory().getPath());
        }
    }
}
