package com.prettysimple.audio;

import android.content.res.AssetManager;

public class AudioEngine {

    private static AudioEngine instance = null;

    public static AudioEngine getInstance() {
        if(instance == null) {
            instance = new AudioEngine();
        }
        return instance;
    }

    public native boolean stopAll();

    public native boolean pauseAll();

    public native boolean resumeAll();

    public native void setAssetManager(final AssetManager assetManager);

    static {
        System.loadLibrary("audio");
    }
}
