/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
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

import android.app.Activity;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatEditText;
import androidx.appcompat.widget.AppCompatSpinner;
import androidx.appcompat.widget.SwitchCompat;
import androidx.appcompat.widget.Toolbar;
import androidx.appcompat.app.AlertDialog;
import android.view.View;
import android.widget.AdapterView;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.provider.DocumentsContract;
import android.util.Log;
import android.database.Cursor;
import android.media.AudioManager;

import java.io.File;
import java.util.List;

public class SettingsActivity extends AppCompatActivity {

    static {
        System.loadLibrary("SDL3");
        System.loadLibrary("main");
    }

    private static final String TAG = "settings-debug";

    public static native boolean loadConfigFile();
    public static native boolean saveConfigFile();
    public static native boolean getConfigBoolean(String item, boolean defval);
    public static native int getConfigInt(String item, boolean defval);
    public static native String getConfigString(String item, boolean defval);
    public static native boolean setConfigBoolean(String item, boolean value);
    public static native boolean setConfigInt(String item, int value);
    public static native boolean setConfigString(String item, String value);
    public static native boolean checkResourceFiles(String path, String msgfile);
    public static native String getGitRevision();
    public static native boolean isDirWritable(String path);

    private static final String KeepAspectRatio = "KeepAspectRatio";
    private static final String LaunchSetting = "LaunchSetting";
    private static final String Stereo = "Stereo";
    private static final String UseSurroundOPL = "UseSurroundOPL";
    private static final String UseTouchOverlay = "UseTouchOverlay";
    private static final String EnableAviPlay = "EnableAviPlay";
    private static final String AudioBufferSize = "AudioBufferSize";
    private static final String LogLevel = "LogLevel";
    private static final String OPLSampleRate = "OPLSampleRate";
    private static final String ResampleQuality = "ResampleQuality";
    private static final String SampleRate = "SampleRate";
    private static final String MusicVolume = "MusicVolume";
    private static final String SoundVolume = "SoundVolume";
    private static final String CDFormat = "CD";
    public  static final String GamePath = "GamePath";
    private static final String SavePath = "SavePath";
    private static final String ShaderPath = "ShaderPath";
    private static final String MessageFileName = "MessageFileName";
    private static final String LogFileName = "LogFileName";
    private static final String FontFileName = "FontFileName";
    private static final String MusicFormat = "Music";
    private static final String OPLCore = "OPLCore";
    private static final String OPLChip = "OPLChip";
    private static final String EnableGLSL = "EnableGLSL";
    private static final String EnableHDR = "EnableHDR";
    private static final String Shader = "Shader";
    private static final String TextureWidth = "TextureWidth";
    private static final String TextureHeight = "TextureHeight";

    private static final int AudioSampleRates[] = { 11025, 22050, 44100, 48000 };
    private static final int AudioBufferSizes[] = { 512, 1024, 2048, 4096, 8192 };
    private static final int OPLSampleRates[] = { 11025, 12429, 22050, 24858, 44100, 49716 };
    private static final String CDFormats[] = { "None", "MP3", "OGG", "OPUS" };
    private static final String MusicFormats[] = { "MIDI", "RIX", "MP3", "OGG", "OPUS" };
    private static final String OPLCores[] = { "MAME", "DBFLT", "DBINT", "NUKED" };
    private static final String OPLChips[] = { "OPL2", "OPL3" };
    private static final String AspectRatios[] = { "16:10", "4:3" };

    private SettingsActivity mInstance = this;
    private static SettingsActivity mSingleton = null;

    private static final int BROWSE_GAMEDIR_CODE = 30001;
    private static final int BROWSE_MSGFILE_CODE = 30002;
    private static final int BROWSE_FONTFILE_CODE = 30003;
    private static final int BROWSE_SHADER_CODE = 30004;

    private static int nativeSampleRate;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mSingleton = this;
        super.onCreate(savedInstanceState);

        AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
        if (audioManager != null) {
            nativeSampleRate = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE) != null ? Integer.parseInt(audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE)) : 48000;
        }

        setContentView(R.layout.activity_settings);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        toolbar.setSubtitle(getResources().getString(R.string.title_settings) + " (" + getGitRevision() + ")");

        findViewById(R.id.edFolder).setFocusable(false);

        ((SwitchCompat)findViewById(R.id.swMsgFile)).setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                findViewById(R.id.edMsgFile).setVisibility(isChecked ? View.VISIBLE : View.GONE);
                findViewById(R.id.btnBrowseMsgFile).setVisibility(isChecked ? View.VISIBLE : View.GONE);
            }
        });

        ((SwitchCompat)findViewById(R.id.swFontFile)).setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                findViewById(R.id.edFontFile).setVisibility(isChecked ? View.VISIBLE : View.GONE);
                findViewById(R.id.btnBrowseFontFile).setVisibility(isChecked ? View.VISIBLE : View.GONE);
            }
        });

        ((SwitchCompat)findViewById(R.id.swLogFile)).setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                findViewById(R.id.edLogFile).setVisibility(isChecked ? View.VISIBLE : View.GONE);
            }
        });

        ((SwitchCompat)findViewById(R.id.swEnableGLSL)).setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                findViewById(R.id.glslBlock).setVisibility(isChecked ? View.VISIBLE : View.GONE);
            }
        });

        ((AppCompatSpinner)findViewById(R.id.spMusFmt)).setOnItemSelectedListener(new Spinner.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                findViewById(R.id.layoutOPL).setVisibility(position == 1 ? View.VISIBLE : View.GONE);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                findViewById(R.id.layoutOPL).setVisibility(View.VISIBLE);
            }
        });

        findViewById(R.id.btnDefault).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setDefaults();
            }
        });

        findViewById(R.id.btnReset).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                resetConfigs();
            }
        });

        findViewById(R.id.btnFinish).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String gamePath = ((EditText)findViewById(R.id.edFolder)).getText().toString();
                String msgFile = ((EditText)findViewById(R.id.edMsgFile)).getText().toString();
                File fileMsg =new File(msgFile);
                if (!checkResourceFiles(gamePath, fileMsg.getName())) {
                    AlertDialog.Builder builder = new AlertDialog.Builder(mInstance);
                    builder.setMessage(getString(R.string.msg_data_not_found_header) + "\n" +
                            gamePath + "\n\n" +
                            getString(R.string.msg_data_not_found_footer));
                    builder.setCancelable(true);
                    builder.setPositiveButton(android.R.string.ok, null);
                    builder.create().show();
                    return;
                }

                if (!setConfigs()) return;
                setConfigBoolean(LaunchSetting, false);
                saveConfigFile();

                AlertDialog.Builder builder = new AlertDialog.Builder(mInstance);
                builder.setMessage(R.string.msg_exit);
                builder.setCancelable(false);
                builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        Intent intent = new Intent(mInstance, PalActivity.class);
                        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                        startActivity(intent);
                        finish();

                    }
                });
                builder.create().show();
            }
        });

        findViewById(R.id.btnBrowseFolder).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
                i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
                i.putExtra(DocumentsContract.EXTRA_INITIAL_URI, MainActivity.getDocTreeUri());

                startActivityForResult(i, BROWSE_GAMEDIR_CODE);
            }
        });

        findViewById(R.id.btnBrowseMsgFile).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                i.putExtra(DocumentsContract.EXTRA_INITIAL_URI, MainActivity.getDocumentUriFromPath(MainActivity.getBasePath()+((EditText)findViewById(R.id.edMsgFile)).getText().toString()));
                i.setType("*/*");

                startActivityForResult(i, BROWSE_MSGFILE_CODE);
            }
        });

        findViewById(R.id.btnBrowseFontFile).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                i.putExtra(DocumentsContract.EXTRA_INITIAL_URI, MainActivity.getDocumentUriFromPath(MainActivity.getBasePath()+((EditText)findViewById(R.id.edFontFile)).getText().toString()));
                i.setType("*/*");

                startActivityForResult(i, BROWSE_FONTFILE_CODE);
            }
        });

        findViewById(R.id.btnBrowseShader).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                i.putExtra(DocumentsContract.EXTRA_INITIAL_URI, MainActivity.getDocumentUriFromPath(MainActivity.getBasePath()+((EditText)findViewById(R.id.edShader)).getText().toString()));
                i.setType("*/*");

                startActivityForResult(i, BROWSE_SHADER_CODE);
            }
        });

        resetConfigs();

        //1st time override for android
        ((EditText)findViewById(R.id.edFolder)).setText(MainActivity.getBasePath());

        if (PalActivity.crashed) {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setMessage(R.string.msg_crash);
            builder.setCancelable(false);
            builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    dialogInterface.dismiss();
                }
            });
            builder.create().show();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent resultData) {
        if (resultCode == Activity.RESULT_OK) {
            String filePath = null;
            Uri uri = null;
            if (resultData != null) {
                uri = resultData.getData();
                filePath = MainActivity.getPath(uri);
                String basePath = MainActivity.getBasePath();
                if( requestCode == BROWSE_GAMEDIR_CODE ) {
                    if( !basePath.equals(filePath) ) {
                        getContentResolver().releasePersistableUriPermission(MainActivity.getDocTreeUri(), Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                        getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
                        MainActivity.setPersistedUri(uri);
                        loadConfigFile();
                        resetConfigs();
                    }
                }else{
                    if( filePath.isEmpty() || !filePath.startsWith(basePath) ) {
                        AlertDialog.Builder builder = new AlertDialog.Builder(this);
                        builder.setMessage(R.string.msg_path_unsuitable);
                        builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialogInterface, int i) {
                                dialogInterface.dismiss();
                            }
                        });
                        builder.create().show();
                        return;
                    }
                    filePath = filePath.replace(basePath, "");
                    if(filePath.startsWith("/")) {
                        filePath = filePath.substring(1);
                    }
                }
            }
            if (filePath != null) {
                if (requestCode == BROWSE_GAMEDIR_CODE) {
                    ((EditText) findViewById(R.id.edFolder)).setText(filePath);
                } else if (requestCode == BROWSE_MSGFILE_CODE) {
                    ((EditText) findViewById(R.id.edMsgFile)).setText(filePath);
                } else if (requestCode == BROWSE_FONTFILE_CODE) {
                    ((EditText) findViewById(R.id.edFontFile)).setText(filePath);
                } else if (requestCode == BROWSE_SHADER_CODE) {
                    ((EditText) findViewById(R.id.edShader)).setText(filePath);
                }
            }
        }
    }

    protected int findMatchedIntIndex(int value, int[] values, int defaultIndex) {
        for(int i = 0; i < values.length; i++) {
            if (values[i] == value)
                return i;
        }
        return defaultIndex;
    }

    protected int findMatchedStringIndex(String value, String[] values, int defaultIndex) {
        for(int i = 0; i < values.length; i++) {
            if (values[i].equalsIgnoreCase(value))
                return i;
        }
        return defaultIndex;
    }

    protected void setDefaults() {
        String sdcardState = Environment.getExternalStorageState();

        findViewById(R.id.edMsgFile).setVisibility(View.GONE);
        findViewById(R.id.btnBrowseMsgFile).setVisibility(View.GONE);
        findViewById(R.id.edFontFile).setVisibility(View.GONE);
        findViewById(R.id.btnBrowseFontFile).setVisibility(View.GONE);
        findViewById(R.id.edLogFile).setVisibility(View.GONE);
        findViewById(R.id.layoutOPL).setVisibility(View.VISIBLE);
        findViewById(R.id.glslBlock).setVisibility(View.GONE);

        ((SeekBar)findViewById(R.id.sbMusVol)).setProgress(getConfigInt(MusicVolume, true));
        ((SeekBar)findViewById(R.id.sbSFXVol)).setProgress(getConfigInt(SoundVolume, true));
        ((SeekBar)findViewById(R.id.sbQuality)).setProgress(getConfigInt(ResampleQuality, true));

        ((EditText)findViewById(R.id.edFolder)).setText(MainActivity.getBasePath());

        ((EditText)findViewById(R.id.edMsgFile)).setText("");
        ((EditText)findViewById(R.id.edFontFile)).setText("");
        ((EditText)findViewById(R.id.edLogFile)).setText("");

        ((SwitchCompat)findViewById(R.id.swMsgFile)).setChecked(false);
        ((SwitchCompat)findViewById(R.id.swFontFile)).setChecked(false);
        ((SwitchCompat)findViewById(R.id.swLogFile)).setChecked(false);
        ((SwitchCompat)findViewById(R.id.swAVI)).setChecked(getConfigBoolean(EnableAviPlay, true));
        ((SwitchCompat)findViewById(R.id.swTouch)).setChecked(getConfigBoolean(UseTouchOverlay, true));
        ((SwitchCompat)findViewById(R.id.swAspect)).setChecked(getConfigBoolean(KeepAspectRatio, true));
        ((SwitchCompat)findViewById(R.id.swSurround)).setChecked(getConfigBoolean(UseSurroundOPL, true));
        ((SwitchCompat)findViewById(R.id.swStereo)).setChecked(getConfigBoolean(Stereo, true));

        ((AppCompatSpinner)findViewById(R.id.spLogLevel)).setSelection(getConfigInt(LogLevel, true));
        ((AppCompatSpinner)findViewById(R.id.spSample)).setSelection(findMatchedIntIndex(nativeSampleRate, AudioSampleRates, 3));    // 48000Hz
        ((AppCompatSpinner)findViewById(R.id.spBuffer)).setSelection(findMatchedIntIndex(getConfigInt(AudioBufferSize, true), AudioBufferSizes, 1));    // 1024
        ((AppCompatSpinner)findViewById(R.id.spCDFmt)).setSelection(findMatchedStringIndex(getConfigString(CDFormat, true), CDFormats, 1));     // None
        ((AppCompatSpinner)findViewById(R.id.spMusFmt)).setSelection(findMatchedStringIndex(getConfigString(MusicFormat, true), MusicFormats, 1));    // RIX
        ((AppCompatSpinner)findViewById(R.id.spOPLCore)).setSelection(findMatchedStringIndex(getConfigString(OPLCore, true), OPLCores, 1));       // DBFLT
        ((AppCompatSpinner)findViewById(R.id.spOPLChip)).setSelection(findMatchedStringIndex(getConfigString(OPLChip, true), OPLChips, 0));       // OPL2
        ((AppCompatSpinner)findViewById(R.id.spOPLRate)).setSelection(findMatchedIntIndex(getConfigInt(OPLSampleRate, true), OPLSampleRates, 5));  // 49716Hz
    }


    protected void resetConfigs() {
        findViewById(R.id.edMsgFile).setVisibility(View.GONE);
        findViewById(R.id.btnBrowseMsgFile).setVisibility(View.GONE);
        findViewById(R.id.edFontFile).setVisibility(View.GONE);
        findViewById(R.id.btnBrowseFontFile).setVisibility(View.GONE);
        findViewById(R.id.edLogFile).setVisibility(View.GONE);
        findViewById(R.id.layoutOPL).setVisibility(View.VISIBLE);
        findViewById(R.id.glslBlock).setVisibility(View.GONE);

        ((SeekBar)findViewById(R.id.sbMusVol)).setProgress(getConfigInt(MusicVolume, false));
        ((SeekBar)findViewById(R.id.sbSFXVol)).setProgress(getConfigInt(SoundVolume, false));
        ((SeekBar)findViewById(R.id.sbQuality)).setProgress(getConfigInt(ResampleQuality, false)); // Best quality

        String msgFile, fontFile, logFile, shader, textureWidth, textureHeight;
        ((EditText)findViewById(R.id.edFolder)).setText(getConfigString(GamePath, false));
        ((EditText)findViewById(R.id.edMsgFile)).setText(msgFile = getConfigString(MessageFileName, false));
        ((EditText)findViewById(R.id.edFontFile)).setText(fontFile = getConfigString(FontFileName, false));
        ((EditText)findViewById(R.id.edLogFile)).setText(logFile = getConfigString(LogFileName, false));
        ((EditText)findViewById(R.id.edShader)).setText(shader = getConfigString(Shader, false));
        ((EditText)findViewById(R.id.edTextureWidth)).setText(textureWidth = String.valueOf(getConfigInt(TextureWidth, false)));
        ((EditText)findViewById(R.id.edTextureHeight)).setText(textureHeight = String.valueOf(getConfigInt(TextureHeight, false)));

        ((SwitchCompat)findViewById(R.id.swMsgFile)).setChecked(msgFile != null && !msgFile.isEmpty());
        ((SwitchCompat)findViewById(R.id.swFontFile)).setChecked(fontFile != null && !fontFile.isEmpty());
        ((SwitchCompat)findViewById(R.id.swLogFile)).setChecked(logFile != null && !logFile.isEmpty());
        ((SwitchCompat)findViewById(R.id.swAVI)).setChecked(getConfigBoolean(EnableAviPlay, false));
        ((SwitchCompat)findViewById(R.id.swTouch)).setChecked(getConfigBoolean(UseTouchOverlay, false));
        ((SwitchCompat)findViewById(R.id.swAspect)).setChecked(getConfigBoolean(KeepAspectRatio, false));
        ((SwitchCompat)findViewById(R.id.swSurround)).setChecked(getConfigBoolean(UseSurroundOPL, false));
        ((SwitchCompat)findViewById(R.id.swStereo)).setChecked(getConfigBoolean(Stereo, false));
        ((SwitchCompat)findViewById(R.id.swEnableGLSL)).setChecked(getConfigBoolean(EnableGLSL, false));
        ((SwitchCompat)findViewById(R.id.swEnableHDR)).setChecked(getConfigBoolean(EnableHDR, false));

        ((AppCompatSpinner)findViewById(R.id.spLogLevel)).setSelection(getConfigInt(LogLevel, false));
        ((AppCompatSpinner)findViewById(R.id.spSample)).setSelection(findMatchedIntIndex(getConfigInt(SampleRate, false), AudioSampleRates, 3));
        ((AppCompatSpinner)findViewById(R.id.spBuffer)).setSelection(findMatchedIntIndex(getConfigInt(AudioBufferSize, false), AudioBufferSizes, 1));
        ((AppCompatSpinner)findViewById(R.id.spCDFmt)).setSelection(findMatchedStringIndex(getConfigString(CDFormat, false), CDFormats, 1));
        ((AppCompatSpinner)findViewById(R.id.spMusFmt)).setSelection(findMatchedStringIndex(getConfigString(MusicFormat, false), MusicFormats, 1));
        ((AppCompatSpinner)findViewById(R.id.spOPLCore)).setSelection(findMatchedStringIndex(getConfigString(OPLCore, false), OPLCores, 1));
        if (((AppCompatSpinner)findViewById(R.id.spOPLCore)).getSelectedItemId() == 3) {
            ((AppCompatSpinner)findViewById(R.id.spOPLChip)).setSelection(1);       // OPL3
        } else {
            ((AppCompatSpinner)findViewById(R.id.spOPLChip)).setSelection(findMatchedStringIndex(getConfigString(OPLChip, false), OPLChips, 0));
        }
        ((AppCompatSpinner)findViewById(R.id.spOPLRate)).setSelection(findMatchedIntIndex(getConfigInt(OPLSampleRate, false), OPLSampleRates, 5));
    }

    protected boolean setConfigs() {
        if (((EditText)findViewById(R.id.edFolder)).getText().toString().isEmpty()) {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setMessage(R.string.msg_empty);
            builder.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    dialogInterface.dismiss();
                }
            });
            builder.create().show();
            return false;
        }

        setConfigInt(MusicVolume, ((SeekBar)findViewById(R.id.sbMusVol)).getProgress());
        setConfigInt(SoundVolume, ((SeekBar)findViewById(R.id.sbSFXVol)).getProgress());
        setConfigInt(ResampleQuality, ((SeekBar)findViewById(R.id.sbQuality)).getProgress());

        setConfigString(GamePath, ((EditText)findViewById(R.id.edFolder)).getText().toString());
        setConfigString(SavePath, ((EditText)findViewById(R.id.edFolder)).getText().toString());
        setConfigString(ShaderPath, ((EditText)findViewById(R.id.edFolder)).getText().toString());
        setConfigString(MessageFileName, ((SwitchCompat)findViewById(R.id.swMsgFile)).isChecked() ? ((EditText)findViewById(R.id.edMsgFile)).getText().toString() : null);
        setConfigString(FontFileName, ((SwitchCompat)findViewById(R.id.swFontFile)).isChecked() ? ((EditText)findViewById(R.id.edFontFile)).getText().toString() : null);
        setConfigString(LogFileName, ((SwitchCompat)findViewById(R.id.swLogFile)).isChecked() ? ((EditText)findViewById(R.id.edLogFile)).getText().toString() : null);
        setConfigString(Shader, ((EditText)findViewById(R.id.edShader)).getText().toString());

        setConfigBoolean(UseTouchOverlay, ((SwitchCompat)findViewById(R.id.swTouch)).isChecked());
        setConfigBoolean(KeepAspectRatio, ((SwitchCompat)findViewById(R.id.swAspect)).isChecked());
        setConfigBoolean(UseSurroundOPL, ((SwitchCompat)findViewById(R.id.swSurround)).isChecked());
        setConfigBoolean(Stereo, ((SwitchCompat)findViewById(R.id.swStereo)).isChecked());
        setConfigBoolean(EnableGLSL, ((SwitchCompat)findViewById(R.id.swEnableGLSL)).isChecked());
        setConfigBoolean(EnableHDR, ((SwitchCompat)findViewById(R.id.swEnableHDR)).isChecked());

        setConfigInt(LogLevel, ((AppCompatSpinner)findViewById(R.id.spLogLevel)).getSelectedItemPosition());
        setConfigInt(SampleRate, Integer.parseInt((String)((AppCompatSpinner)findViewById(R.id.spSample)).getSelectedItem()));
        setConfigInt(AudioBufferSize, Integer.parseInt((String)((AppCompatSpinner)findViewById(R.id.spBuffer)).getSelectedItem()));
        setConfigString(CDFormat, (String)((AppCompatSpinner)findViewById(R.id.spCDFmt)).getSelectedItem());
        setConfigString(MusicFormat, (String)((AppCompatSpinner)findViewById(R.id.spMusFmt)).getSelectedItem());
        setConfigString(OPLCore, (String)((AppCompatSpinner)findViewById(R.id.spOPLCore)).getSelectedItem());
        if (((AppCompatSpinner)findViewById(R.id.spOPLCore)).getSelectedItemId() == 3) {
            setConfigString(OPLChip, OPLChips[1]);
        } else {
            setConfigString(OPLChip, (String) ((AppCompatSpinner) findViewById(R.id.spOPLChip)).getSelectedItem());
        }
        setConfigInt(OPLSampleRate, Integer.parseInt((String)((AppCompatSpinner)findViewById(R.id.spOPLRate)).getSelectedItem()));
        setConfigInt(TextureWidth, Integer.parseInt((String)((EditText)findViewById(R.id.edTextureWidth)).getText().toString()));
        setConfigInt(TextureHeight, Integer.parseInt((String)((EditText)findViewById(R.id.edTextureHeight)).getText().toString()));

        return true;
    }
}
