package io.github.sdlpal;

import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.AppCompatSpinner;
import android.support.v7.widget.SwitchCompat;
import android.support.v7.widget.Toolbar;
import  android.support.v7.app.AlertDialog;
import android.view.View;
import android.widget.AdapterView;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.Spinner;

public class SettingsActivity extends AppCompatActivity {

    public static native boolean loadConfigFile();
    public static native boolean saveConfigFile();
    public static native boolean getConfigBoolean(String item);
    public static native int getConfigInt(String item);
    public static native String getConfigString(String item);
    public static native boolean setConfigBoolean(String item, boolean value);
    public static native boolean setConfigInt(String item, int value);
    public static native boolean setConfigString(String item, String value);

    private static final String FullScreen = "FullScreen";
    private static final String KeepAspectRatio = "KeepAspectRatio";
    private static final String LaunchSetting = "LaunchSetting";
    private static final String Stereo = "Stereo";
    private static final String UseEmbeddedFonts = "UseEmbeddedFonts";
    private static final String UseSurroundOPL = "UseSurroundOPL";
    private static final String UseTouchOverlay = "UseTouchOverlay";
    private static final String AudioBufferSize = "AudioBufferSize";
    private static final String CodePage = "CodePage";
    private static final String OPLSampleRate = "OPLSampleRate";
    private static final String ResampleQuality = "ResampleQuality";
    private static final String SampleRate = "SampleRate";
    private static final String MusicVolume = "MusicVolume";
    private static final String SoundVolume = "SoundVolume";
    private static final String CDFormat = "CD";
    private static final String GamePath = "GamePath";
    private static final String SavePath = "SavePath";
    private static final String MessageFileName = "MessageFileName";
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

        ((SwitchCompat)findViewById(R.id.swCustomLang)).setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                findViewById(R.id.swGameLang).setVisibility(isChecked ? View.GONE : View.VISIBLE);
                findViewById(R.id.edLangFile).setVisibility(isChecked ? View.VISIBLE : View.GONE);
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
                        System.exit(0);
                    }
                });
                builder.create().show();
            }
        });

        setDefaults();

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
        findViewById(R.id.edLangFile).setVisibility(View.GONE);
        findViewById(R.id.layoutOPL).setVisibility(View.VISIBLE);

        ((SeekBar)findViewById(R.id.sbMusVol)).setProgress(getConfigInt(MusicVolume));
        ((SeekBar)findViewById(R.id.sbSFXVol)).setProgress(getConfigInt(SoundVolume));
        ((SeekBar)findViewById(R.id.sbQuality)).setProgress(getConfigInt(ResampleQuality)); // Best quality

        String langFile = getConfigString(MessageFileName);
        ((EditText)findViewById(R.id.edFolder)).setText(getConfigString(GamePath));
        ((EditText)findViewById(R.id.edLangFile)).setText(langFile);

        ((SwitchCompat)findViewById(R.id.swEmbedFont)).setChecked(getConfigBoolean(UseEmbeddedFonts));
        ((SwitchCompat)findViewById(R.id.swCustomLang)).setChecked(langFile != null && !langFile.isEmpty());
        ((SwitchCompat)findViewById(R.id.swGameLang)).setChecked(getConfigInt(CodePage) != 0);
        ((SwitchCompat)findViewById(R.id.swTouch)).setChecked(getConfigBoolean(UseTouchOverlay));
        ((SwitchCompat)findViewById(R.id.swAspect)).setChecked(getConfigBoolean(KeepAspectRatio));
        ((SwitchCompat)findViewById(R.id.swSurround)).setChecked(getConfigBoolean(UseSurroundOPL));

        ((AppCompatSpinner)findViewById(R.id.spSample)).setSelection(findMatchedIntIndex(getConfigInt(SampleRate), AudioSampleRates, 2));    // 44100Hz
        ((AppCompatSpinner)findViewById(R.id.spBuffer)).setSelection(findMatchedIntIndex(getConfigInt(AudioBufferSize), AudioBufferSizes, 1));    // 1024
        ((AppCompatSpinner)findViewById(R.id.spCDFmt)).setSelection(findMatchedStringIndex(getConfigString(CDFormat), CDFormats, 1));     // OGG
        ((AppCompatSpinner)findViewById(R.id.spMusFmt)).setSelection(findMatchedStringIndex(getConfigString(MusicFormat), MusicFormats, 1));    // RIX
        ((AppCompatSpinner)findViewById(R.id.spOPL)).setSelection(findMatchedStringIndex(getConfigString(OPLFormat), OPLFormats, 1));       // MAME
        ((AppCompatSpinner)findViewById(R.id.spOPLRate)).setSelection(findMatchedIntIndex(getConfigInt(OPLSampleRate), OPLSampleRates, 2));  // 49716Hz
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
        setConfigString(MessageFileName, ((EditText)findViewById(R.id.edLangFile)).getText().toString());

        setConfigBoolean(UseEmbeddedFonts, ((SwitchCompat)findViewById(R.id.swEmbedFont)).isChecked());
        setConfigInt(CodePage, ((SwitchCompat)findViewById(R.id.swGameLang)).isChecked() ? 1 : 0);
        setConfigBoolean(UseTouchOverlay, ((SwitchCompat)findViewById(R.id.swTouch)).isChecked());
        setConfigBoolean(KeepAspectRatio, ((SwitchCompat)findViewById(R.id.swAspect)).isChecked());
        setConfigBoolean(UseSurroundOPL, ((SwitchCompat)findViewById(R.id.swSurround)).isChecked());

        setConfigInt(SampleRate, Integer.parseInt((String)((AppCompatSpinner)findViewById(R.id.spSample)).getSelectedItem()));
        setConfigInt(AudioBufferSize, Integer.parseInt((String)((AppCompatSpinner)findViewById(R.id.spBuffer)).getSelectedItem()));
        setConfigString(CDFormat, (String)((AppCompatSpinner)findViewById(R.id.spCDFmt)).getSelectedItem());
        setConfigString(MusicFormat, (String)((AppCompatSpinner)findViewById(R.id.spMusFmt)).getSelectedItem());
        setConfigString(OPLFormat, (String)((AppCompatSpinner)findViewById(R.id.spOPL)).getSelectedItem());
        setConfigInt(OPLSampleRate, Integer.parseInt((String)((AppCompatSpinner)findViewById(R.id.spOPLRate)).getSelectedItem()));

        return true;
    }
}
