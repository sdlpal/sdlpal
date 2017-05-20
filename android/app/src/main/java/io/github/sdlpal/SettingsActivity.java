package io.github.sdlpal;

import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.AppCompatSpinner;
import android.support.v7.widget.SwitchCompat;
import android.support.v7.widget.Toolbar;
import  android.support.v7.app.AlertDialog;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.Spinner;

import java.util.ArrayList;

public class SettingsActivity extends AppCompatActivity {

    public static native boolean loadConfigFile();
    public static native boolean saveConfigFile();
    public static native boolean getConfigBoolean(String item, boolean defval);
    public static native int getConfigInt(String item, boolean defval);
    public static native String getConfigString(String item, boolean defval);
    public static native boolean setConfigBoolean(String item, boolean value);
    public static native boolean setConfigInt(String item, int value);
    public static native boolean setConfigString(String item, String value);

    private static final String KeepAspectRatio = "KeepAspectRatio";
    private static final String LaunchSetting = "LaunchSetting";
    private static final String Stereo = "Stereo";
    private static final String UseSurroundOPL = "UseSurroundOPL";
    private static final String UseTouchOverlay = "UseTouchOverlay";
    private static final String AudioBufferSize = "AudioBufferSize";
    private static final String LogLevel = "LogLevel";
    private static final String OPLSampleRate = "OPLSampleRate";
    private static final String ResampleQuality = "ResampleQuality";
    private static final String SampleRate = "SampleRate";
    private static final String MusicVolume = "MusicVolume";
    private static final String SoundVolume = "SoundVolume";
    private static final String CDFormat = "CD";
    private static final String GamePath = "GamePath";
    private static final String SavePath = "SavePath";
    private static final String MessageFileName = "MessageFileName";
    private static final String LogFileName = "LogFileName";
    private static final String FontFileName = "FontFileName";
    private static final String MusicFormat = "Music";
    private static final String OPLFormat = "OPL";

    private static final int AudioSampleRates[] = { 11025, 22050, 44100 };
    private static final int AudioBufferSizes[] = { 512, 1024, 2048, 4096, 8192 };
    private static final int OPLSampleRates[] = { 12429, 24858, 49716 };
    private static final String CDFormats[] = { "MP3", "OGG" };
    private static final String MusicFormats[] = { "MIDI", "RIX", "MP3", "OGG" };
    private static final String OPLFormats[] = { "DOSBOX", "MAME", "DOSBOXNEW" };

    private SettingsActivity mInstance = this;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        ((SwitchCompat)findViewById(R.id.swMsgFile)).setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                findViewById(R.id.edMsgFile).setVisibility(isChecked ? View.VISIBLE : View.GONE);
            }
        });

        ((SwitchCompat)findViewById(R.id.swFontFile)).setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                findViewById(R.id.edFontFile).setVisibility(isChecked ? View.VISIBLE : View.GONE);
            }
        });

        ((SwitchCompat)findViewById(R.id.swLogFile)).setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                findViewById(R.id.edLogFile).setVisibility(isChecked ? View.VISIBLE : View.GONE);
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

        resetConfigs();

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

    protected int findMatchedIntIndex(int value, int[] values, int defaultIndex) {
        for(int i = 0; i < values.length; i++) {
            if (values[i] == value)
                return i;
        }
        return defaultIndex;
    }

    protected int findMatchedStringIndex(String value, String[] values, int defaultIndex) {
        for(int i = 0; i < values.length; i++) {
            if (values[i].equals(value))
                return i;
        }
        return defaultIndex;
    }

    protected void setDefaults() {
        String sdcardState = Environment.getExternalStorageState();

        findViewById(R.id.edMsgFile).setVisibility(View.GONE);
        findViewById(R.id.edFontFile).setVisibility(View.GONE);
        findViewById(R.id.edLogFile).setVisibility(View.GONE);
        findViewById(R.id.layoutOPL).setVisibility(View.VISIBLE);

        ((SeekBar)findViewById(R.id.sbMusVol)).setProgress(getConfigInt(MusicVolume, true));
        ((SeekBar)findViewById(R.id.sbSFXVol)).setProgress(getConfigInt(SoundVolume, true));
        ((SeekBar)findViewById(R.id.sbQuality)).setProgress(getConfigInt(ResampleQuality, true));

        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)){
            ((EditText)findViewById(R.id.edFolder)).setText(Environment.getExternalStorageDirectory().getPath() + "/sdlpal/");
        } else {
            ((EditText)findViewById(R.id.edFolder)).setText("/sdcard/sdlpal/");
        }
        ((EditText)findViewById(R.id.edMsgFile)).setText("");
        ((EditText)findViewById(R.id.edFontFile)).setText("");
        ((EditText)findViewById(R.id.edLogFile)).setText("");

        ((SwitchCompat)findViewById(R.id.swMsgFile)).setChecked(false);
        ((SwitchCompat)findViewById(R.id.swFontFile)).setChecked(false);
        ((SwitchCompat)findViewById(R.id.swLogFile)).setChecked(false);
        ((SwitchCompat)findViewById(R.id.swTouch)).setChecked(getConfigBoolean(UseTouchOverlay, true));
        ((SwitchCompat)findViewById(R.id.swAspect)).setChecked(getConfigBoolean(KeepAspectRatio, true));
        ((SwitchCompat)findViewById(R.id.swSurround)).setChecked(getConfigBoolean(UseSurroundOPL, true));
        ((SwitchCompat)findViewById(R.id.swStereo)).setChecked(getConfigBoolean(Stereo, true));

        ((AppCompatSpinner)findViewById(R.id.spLogLevel)).setSelection(getConfigInt(LogLevel, true));
        ((AppCompatSpinner)findViewById(R.id.spSample)).setSelection(findMatchedIntIndex(getConfigInt(SampleRate, true), AudioSampleRates, 2));    // 44100Hz
        ((AppCompatSpinner)findViewById(R.id.spBuffer)).setSelection(findMatchedIntIndex(getConfigInt(AudioBufferSize, true), AudioBufferSizes, 1));    // 1024
        ((AppCompatSpinner)findViewById(R.id.spCDFmt)).setSelection(findMatchedStringIndex(getConfigString(CDFormat, true), CDFormats, 1));     // OGG
        ((AppCompatSpinner)findViewById(R.id.spMusFmt)).setSelection(findMatchedStringIndex(getConfigString(MusicFormat, true), MusicFormats, 1));    // RIX
        ((AppCompatSpinner)findViewById(R.id.spOPL)).setSelection(findMatchedStringIndex(getConfigString(OPLFormat, true), OPLFormats, 1));       // MAME
        ((AppCompatSpinner)findViewById(R.id.spOPLRate)).setSelection(findMatchedIntIndex(getConfigInt(OPLSampleRate, true), OPLSampleRates, 2));  // 49716Hz
    }


    protected void resetConfigs() {
        findViewById(R.id.edMsgFile).setVisibility(View.GONE);
        findViewById(R.id.edFontFile).setVisibility(View.GONE);
        findViewById(R.id.edLogFile).setVisibility(View.GONE);
        findViewById(R.id.layoutOPL).setVisibility(View.VISIBLE);

        ((SeekBar)findViewById(R.id.sbMusVol)).setProgress(getConfigInt(MusicVolume, false));
        ((SeekBar)findViewById(R.id.sbSFXVol)).setProgress(getConfigInt(SoundVolume, false));
        ((SeekBar)findViewById(R.id.sbQuality)).setProgress(getConfigInt(ResampleQuality, false)); // Best quality

        String msgFile, fontFile, logFile;
        ((EditText)findViewById(R.id.edFolder)).setText(getConfigString(GamePath, false));
        ((EditText)findViewById(R.id.edMsgFile)).setText(msgFile = getConfigString(MessageFileName, false));
        ((EditText)findViewById(R.id.edFontFile)).setText(fontFile = getConfigString(FontFileName, false));
        ((EditText)findViewById(R.id.edLogFile)).setText(logFile = getConfigString(LogFileName, false));

        ((SwitchCompat)findViewById(R.id.swMsgFile)).setChecked(msgFile != null && !msgFile.isEmpty());
        ((SwitchCompat)findViewById(R.id.swFontFile)).setChecked(fontFile != null && !fontFile.isEmpty());
        ((SwitchCompat)findViewById(R.id.swLogFile)).setChecked(logFile != null && !logFile.isEmpty());
        ((SwitchCompat)findViewById(R.id.swTouch)).setChecked(getConfigBoolean(UseTouchOverlay, false));
        ((SwitchCompat)findViewById(R.id.swAspect)).setChecked(getConfigBoolean(KeepAspectRatio, false));
        ((SwitchCompat)findViewById(R.id.swSurround)).setChecked(getConfigBoolean(UseSurroundOPL, false));
        ((SwitchCompat)findViewById(R.id.swStereo)).setChecked(getConfigBoolean(Stereo, false));

        ((AppCompatSpinner)findViewById(R.id.spLogLevel)).setSelection(getConfigInt(LogLevel, false));
        ((AppCompatSpinner)findViewById(R.id.spSample)).setSelection(findMatchedIntIndex(getConfigInt(SampleRate, false), AudioSampleRates, 2));    // 44100Hz
        ((AppCompatSpinner)findViewById(R.id.spBuffer)).setSelection(findMatchedIntIndex(getConfigInt(AudioBufferSize, false), AudioBufferSizes, 1));    // 1024
        ((AppCompatSpinner)findViewById(R.id.spCDFmt)).setSelection(findMatchedStringIndex(getConfigString(CDFormat, false), CDFormats, 1));     // OGG
        ((AppCompatSpinner)findViewById(R.id.spMusFmt)).setSelection(findMatchedStringIndex(getConfigString(MusicFormat, false), MusicFormats, 1));    // RIX
        ((AppCompatSpinner)findViewById(R.id.spOPL)).setSelection(findMatchedStringIndex(getConfigString(OPLFormat, false), OPLFormats, 1));       // MAME
        ((AppCompatSpinner)findViewById(R.id.spOPLRate)).setSelection(findMatchedIntIndex(getConfigInt(OPLSampleRate, false), OPLSampleRates, 2));  // 49716Hz
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
        setConfigString(MessageFileName, ((SwitchCompat)findViewById(R.id.swMsgFile)).isChecked() ? ((EditText)findViewById(R.id.edMsgFile)).getText().toString() : null);
        setConfigString(FontFileName, ((SwitchCompat)findViewById(R.id.swFontFile)).isChecked() ? ((EditText)findViewById(R.id.edFontFile)).getText().toString() : null);
        setConfigString(LogFileName, ((SwitchCompat)findViewById(R.id.swLogFile)).isChecked() ? ((EditText)findViewById(R.id.edLogFile)).getText().toString() : null);

        setConfigBoolean(UseTouchOverlay, ((SwitchCompat)findViewById(R.id.swTouch)).isChecked());
        setConfigBoolean(KeepAspectRatio, ((SwitchCompat)findViewById(R.id.swAspect)).isChecked());
        setConfigBoolean(UseSurroundOPL, ((SwitchCompat)findViewById(R.id.swSurround)).isChecked());
        setConfigBoolean(Stereo, ((SwitchCompat)findViewById(R.id.swStereo)).isChecked());

        setConfigInt(LogLevel, ((AppCompatSpinner)findViewById(R.id.spLogLevel)).getSelectedItemPosition());
        setConfigInt(SampleRate, Integer.parseInt((String)((AppCompatSpinner)findViewById(R.id.spSample)).getSelectedItem()));
        setConfigInt(AudioBufferSize, Integer.parseInt((String)((AppCompatSpinner)findViewById(R.id.spBuffer)).getSelectedItem()));
        setConfigString(CDFormat, (String)((AppCompatSpinner)findViewById(R.id.spCDFmt)).getSelectedItem());
        setConfigString(MusicFormat, (String)((AppCompatSpinner)findViewById(R.id.spMusFmt)).getSelectedItem());
        setConfigString(OPLFormat, (String)((AppCompatSpinner)findViewById(R.id.spOPL)).getSelectedItem());
        setConfigInt(OPLSampleRate, Integer.parseInt((String)((AppCompatSpinner)findViewById(R.id.spOPLRate)).getSelectedItem()));

        return true;
    }
}
