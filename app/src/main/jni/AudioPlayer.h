#ifndef __AudioPlayer__
#define __AudioPlayer__

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>
#include <cstdint>
#include <jni.h>

namespace audio {
    class AudioPlayer {
    public:
        AudioPlayer();

        AudioPlayer(const AudioPlayer &) = default;

        AudioPlayer &operator=(const AudioPlayer &) & = default;

        AudioPlayer(AudioPlayer &&) = default;

        AudioPlayer &operator=(AudioPlayer &&) & = default;

        virtual ~AudioPlayer();

    public:
        bool setParams(const float pitch, const float pan, const float volume) noexcept;

        bool setVolume(const float volume) noexcept;

        bool play() noexcept;

        bool pause() noexcept;

        bool resume() noexcept;

        bool stop() noexcept;

        void setHeadAtEnd(const bool isHeadAtEnd) noexcept;

        const bool isHeadAtEnd() const noexcept;

        SLuint32 getPrefetchedStatus() noexcept;

        const bool isPrefetchedSufficient() const noexcept;

        const bool isLooping() const noexcept;

        const int getPlayerId() const noexcept;

        void setJavaAudioPlayerObj(const jobject obj) noexcept;

        bool initWithEngine(const SLEngineItf &engineEngine, const SLObjectItf &outputMixObject,
                            AAssetManager *assetManager, const int audioId,
                            const std::string &fileFullPath, const float volume,
                            const bool loop) noexcept;

    private:
        static void prefetchEventCallback(SLPrefetchStatusItf caller, void *context,
                                          SLuint32 prefetchEvent) noexcept;

        static void playEventCallback(SLPlayItf caller, void *context, SLuint32 playEvent) noexcept;

    private:
        SLObjectItf _fdPlayerObject;
        SLPlayItf _fdPlayerPlay;
        SLSeekItf _fdPlayerSeek;
        SLVolumeItf _fdPlayerVolume;
        SLPrefetchStatusItf _fdPlayerPrefetchedStatus;

        bool _isHeadAtEnd;
        bool _isPrefetchedSufficientData;

        bool _loop;
        int _audioId;
        int _assetFd;
        float _volume;

        jobject _javaAudioPlayerObj;
    };
}

#endif