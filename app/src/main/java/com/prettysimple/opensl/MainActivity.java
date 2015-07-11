package com.prettysimple.opensl;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import java.util.Date;
import java.util.Vector;

import com.prettysimple.audio.AudioEngine;
import com.prettysimple.audio.AudioPlayer;

public class MainActivity extends Activity implements View.OnClickListener {

    private AssetManager _assetManager = null;

    private Vector<AudioPlayer> _players;

    private Button _btnPlay = null;
    private Button _btnStop = null;
    private Button _btnStopAll = null;
    private Button _btnPauseAll = null;
    private Button _btnResumeAll = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        _players = new Vector<>();

        AudioEngine.getInstance().setAssetManager(getAssets());

        _btnPlay = (Button)findViewById(R.id.bt_play);
        _btnPlay.setOnClickListener(this);

        _btnStop = (Button)findViewById(R.id.bt_stop);
        _btnStop.setOnClickListener(this);

        _btnStopAll = (Button)findViewById(R.id.bt_stopall);
        _btnStopAll.setOnClickListener(this);
        _btnPauseAll = (Button)findViewById(R.id.bt_pauseall);
        _btnPauseAll.setOnClickListener(this);
        _btnResumeAll = (Button)findViewById(R.id.bt_resumeall);
        _btnResumeAll.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        if (v == _btnPlay) {
            Date date = new Date();
            final AudioPlayer player = new AudioPlayer();
            if (player.init((date.getTime() % 18) + ".ogg", 1.f, true)) {
                player.play();
                _players.add(player);
            }
        } else if (v == _btnStop) {
            if (_players.size() > 0) {
                final AudioPlayer player = _players.lastElement();
                Log.v("libaudio", "stop " + player.getPlayerId());
                if (player.stop()) {
                    _players.remove(player);
                }
            }
        } else if (v == _btnStopAll) {
            AudioEngine.getInstance().stopAll();
            _players.clear();
        } else if (v == _btnPauseAll) {
            AudioEngine.getInstance().pauseAll();
        } else if (v == _btnResumeAll) {
            AudioEngine.getInstance().resumeAll();
        }
    }
}
