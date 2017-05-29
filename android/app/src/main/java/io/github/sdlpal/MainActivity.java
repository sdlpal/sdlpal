package io.github.sdlpal;

import android.Manifest;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.*;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.provider.Settings;
import android.net.Uri;

import java.io.*;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = "sdlpal-debug";

    public static native void setAppPath(String basepath, String datapath, String cachepath);

    public static boolean crashed = false;

    private final static int REQUEST_FILESYSTEM_ACCESS_CODE = 101;
    private final AppCompatActivity mActivity = this;

    interface RequestForPermissions {
        void request();
    }

    private void requestForPermissions() {
        // Since granting writing permission implicitly grants reading permission, no need to explicitly add reading permission here
        ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, REQUEST_FILESYSTEM_ACCESS_CODE);
    }

    private void alertUser(int id, final RequestForPermissions req) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage(id);
        builder.setCancelable(false);
        builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                req.request();
            }
        });
        builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                System.exit(1);
            }
        });
        builder.create().show();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch (requestCode) {
            case REQUEST_FILESYSTEM_ACCESS_CODE:
                for(int i = 0; i < permissions.length; i++) {
                    switch(permissions[i]) {
                        case Manifest.permission.WRITE_EXTERNAL_STORAGE:
                            if (grantResults[i] == PackageManager.PERMISSION_GRANTED) {
                                StartGame();
                                break;
                            }
                            if (ActivityCompat.shouldShowRequestPermissionRationale(mActivity, permissions[i])) {
                                alertUser(R.string.toast_requestpermission, new RequestForPermissions() {
                                    @Override
                                    public void request() {
                                        requestForPermissions();
                                    }
                                });
                            } else {
                                alertUser(R.string.toast_grantpermission, new RequestForPermissions() {
                                    @Override
                                    public void request() {
                                        Intent intent = new Intent();
                                        intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                                        Uri uri = Uri.fromParts("package", getPackageName(), null);
                                        intent.setData(uri);
                                        startActivity(intent);
                                    }
                                });
                            }
                    }
                }
                break;
        }
    }

    public void onStart() {
        super.onStart();
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M || ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED ) {
            StartGame();
        } else {
            requestForPermissions();
        }
    }

    public void StartGame() {

        System.loadLibrary("SDL2");
        System.loadLibrary("main");

        String dataPath = getApplicationContext().getFilesDir().getPath();
        String cachePath = getApplicationContext().getCacheDir().getPath();
        String sdcardState = Environment.getExternalStorageState();
        if (sdcardState.equals(Environment.MEDIA_MOUNTED)){
            setAppPath(Environment.getExternalStorageDirectory().getPath() + "/sdlpal/", dataPath, cachePath);
        } else {
            setAppPath("/sdcard/sdlpal/", dataPath, cachePath);
        }

        File runningFile = new File(cachePath + "/running");
        crashed = runningFile.exists();

        Intent intent;
        if (SettingsActivity.loadConfigFile() || crashed) {
            runningFile.delete();

            intent = new Intent(this, SettingsActivity.class);
        } else {
            intent = new Intent(this, PalActivity.class);
        }
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(intent);
        finish();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }
}
