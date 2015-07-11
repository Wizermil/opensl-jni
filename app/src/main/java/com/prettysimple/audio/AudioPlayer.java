package com.prettysimple.audio;

public class AudioPlayer {

    public AudioPlayer() {
        _audioId = -1;
    }

    public native boolean init(final String path, final float volume, final boolean loop);
    public native boolean stop();
    public native boolean play();
    public native boolean pause();
    public native boolean resume();
    public native boolean setParams(final float pitch, final float pan, final float volume);
    public native boolean setVolume(final float volume);

    public int getPlayerId() {
        return _audioId;
    }

    private int _audioId;

    static {
        System.loadLibrary("audio");
    }
}
