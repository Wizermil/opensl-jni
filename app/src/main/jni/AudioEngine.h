#ifndef __AudioEngine__
#define __AudioEngine__

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <string>
#include <map>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "AudioPlayer.h"
#include <cstdint>
#include <jni.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

extern "C"
{
    JNIEnv *getJNIEnv();

    JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved);
    JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved);

    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_init(JNIEnv *env, jobject thiz, jstring path, jfloat volume, jboolean loop);
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_play(JNIEnv *env, jobject thiz);
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_stop(JNIEnv *env, jobject thiz);
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_pause(JNIEnv *env, jobject thiz);
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_resume(JNIEnv *env, jobject thiz);
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_setParams(JNIEnv *env, jobject thiz, jfloat pitch, jfloat pan, jfloat volume);
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_setVolume(JNIEnv *env, jobject thiz, jfloat volume);

    JNIEXPORT void JNICALL Java_com_prettysimple_audio_AudioEngine_setAssetManager(JNIEnv *env, jobject thiz, jobject assetManager);
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioEngine_pauseAll(JNIEnv *env, jobject thiz);
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioEngine_resumeAll(JNIEnv *env, jobject thiz);
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioEngine_stopAll(JNIEnv *env, jobject thiz);
}

namespace audio
{
    class AudioEngine
    {
    protected:
        AudioEngine();

        AudioEngine(const AudioEngine &) = delete;

        AudioEngine &operator=(const AudioEngine &) & = delete;

        AudioEngine(AudioEngine &&) = delete;

        AudioEngine &operator=(AudioEngine &&) & = delete;

        virtual ~AudioEngine();

    public:
        static AudioEngine *getInstance() noexcept;

        AudioPlayer *createPlayerWithPath(const std::string &fileFullPath, const float volume, const bool loop) noexcept;

        AAssetManager *getAssetManager() const noexcept;

        void setAssetManager(const jobject _assetManager);

        void setHeadAtEnd(const int audioId) noexcept;

        SLuint32 getPrefetchedStatus(const int audioId) noexcept;

        bool stop(const int audioId) noexcept;

        bool play(const int audioId) noexcept;

        bool pause(const int audioId) noexcept;

        bool resume(const int audioId) noexcept;

        bool setParams(const int audioId, const float pitch, const float pan, const float volume) noexcept;

        bool setVolume(const int audioId, const float volume) noexcept;

        void destroy() noexcept;

        bool stopAll() noexcept;

        bool pauseAll() noexcept;

        bool resumeAll() noexcept;

    private:
        bool initOpenSL() noexcept;

        void audioPlayerGc(const int sleep) noexcept;

        void audioPlayerTest(const int sleep) noexcept;

        void clean() noexcept;

    private:
        int _audioIds;
        static AudioEngine *_instance;
        std::mutex _playersMutex;
        std::map<int, AudioPlayer *> _players;

        SLObjectItf _engineObject;
        SLEngineItf _engineEngine;
        SLObjectItf _outputMixObject;

        jobject _assetManager;

        std::atomic<bool> _stopGc;
        std::atomic<bool> _doneGc;
        std::thread _threadGc;
        std::mutex _pauselMutex;
        std::condition_variable _condition;

        std::thread _threadTest;
    };
}

#endif