/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

package com.sdlpal.sdlpal;

import org.libsdl.app.SDLActivity;

import android.content.Intent;
import android.os.*;
import android.util.*;
import android.media.*;
import android.net.Uri;
import java.io.*;

public class PalActivity extends SDLActivity {

    static {
        System.loadLibrary("SDL2");
        System.loadLibrary("main");
    }

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
