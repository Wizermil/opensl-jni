#include "AudioPlayer.h"
#include <unistd.h>
#include "AudioUtils.h"
#include <complex>
#include "AudioEngine.h"

using namespace audio;

AudioPlayer::AudioPlayer() : _fdPlayerObject(nullptr), _fdPlayerPlay(nullptr),
                             _fdPlayerSeek(nullptr), _fdPlayerVolume(nullptr),
                             _fdPlayerPrefetchedStatus(nullptr), _isHeadAtEnd(false),
                             _isPrefetchedSufficientData(false), _loop(false), _audioId(-1),
                             _assetFd(-1), _volume(1.f), _javaAudioPlayerObj(nullptr) {
}

AudioPlayer::~AudioPlayer() {
    LOGD("_players popo %d", _audioId);
    if (_fdPlayerObject != nullptr) {
        LOGD("_players popo1 %d", _audioId);
        (*_fdPlayerObject)->Destroy(_fdPlayerObject);
        LOGD("_players popo2 %d", _audioId);
        _fdPlayerPlay = nullptr;
        _fdPlayerSeek = nullptr;
        _fdPlayerVolume = nullptr;
        _fdPlayerPrefetchedStatus = nullptr;
    }
    LOGD("_players pipi %d", _audioId);
    if (_assetFd > 0) {
        close(_assetFd);
        _assetFd = -1;
    }

    LOGD("_players papa %d", _audioId);
    _isPrefetchedSufficientData = false;
    _isHeadAtEnd = false;

    _loop = false;
    _audioId = -1;

    JNIEnv *jenv = getJNIEnv();
    if (jenv != nullptr) {
        if (_javaAudioPlayerObj != nullptr) {
            jenv->DeleteGlobalRef(_javaAudioPlayerObj);
            _javaAudioPlayerObj = nullptr;
        }
    }
    LOGD("_players pupu");
}

/**
 * Set pitch, pan and gain to the sound
 */
bool AudioPlayer::setParams(const float pitch, const float pan, const float volume) noexcept {
    bool ret = false;
    if (_fdPlayerVolume != nullptr) {
        if (!setVolume(volume)) {
            LOGEX("SetVolumeLevel _fdPlayerVolume fail");
            return false;
        }
        SLresult result = (*_fdPlayerVolume)->EnableStereoPosition(_fdPlayerVolume,
                                                                   SL_BOOLEAN_TRUE);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("EnableStereoPosition _fdPlayerVolume fail");
            return false;
        }
        result = (*_fdPlayerVolume)->SetStereoPosition(_fdPlayerVolume, (SLpermille) pan * 1000);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("SetStereoPosition _fdPlayerVolume fail");
            return false;
        }
        else {
            _volume = volume;
            ret = true;
        }
    }

    return ret;
}

/**
 * Set volume of the sound (0% -> 100%)
 */
bool AudioPlayer::setVolume(const float volume) noexcept {
    bool ret = false;
    if (_fdPlayerVolume != nullptr) {
        float vol = volume;
        if (volume > 1.0f) {
            vol = 1.0f;
        }
        else if (volume < 0.0f) {
            vol = 0.0f;
        }

        int dbVolume = 2000 * std::log10(vol);
        if (dbVolume < SL_MILLIBEL_MIN) {
            dbVolume = SL_MILLIBEL_MIN;
        }

        SLresult result = (*_fdPlayerVolume)->SetVolumeLevel(_fdPlayerVolume,
                                                             (SLpermille) dbVolume);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("SetVolumeLevel _fdPlayerVolume fail");
        }
        else {
            ret = true;
            _volume = vol;
        }
    }

    return ret;
}

/**
 * Pause sound
 */
bool AudioPlayer::pause() noexcept {
    bool ret = false;
    if (_fdPlayerPlay != nullptr) {
        SLresult result = (*_fdPlayerPlay)->SetPlayState(_fdPlayerPlay, SL_PLAYSTATE_PAUSED);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("SetPlayState _fdPlayerPlay fail");
        }
        else {
            ret = true;
        }
    }
    return ret;
}

/**
 * Play sound
 */
bool AudioPlayer::play() noexcept {
    bool ret = false;
    if (_fdPlayerPlay != nullptr) {
        SLresult result = (*_fdPlayerPlay)->SetPlayState(_fdPlayerPlay, SL_PLAYSTATE_PLAYING);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("SetPlayState _fdPlayerPlay fail");
        }
        else {
            ret = true;
        }
    }
    return ret;
}

/**
 * Resume sound
 */
bool AudioPlayer::resume() noexcept {
    bool ret = false;
    if (_fdPlayerPlay != nullptr) {
        SLresult result = (*_fdPlayerPlay)->SetPlayState(_fdPlayerPlay, SL_PLAYSTATE_PLAYING);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("SetPlayState _fdPlayerPlay fail");
        }
        else {
            ret = true;
        }
    }
    return ret;
}

/**
 * Stop sound meaning it will be destroyed
 */
bool AudioPlayer::stop() noexcept {
    bool ret = false;
    if (_fdPlayerPlay != nullptr) {
        SLresult result = (*_fdPlayerPlay)->SetPlayState(_fdPlayerPlay, SL_PLAYSTATE_STOPPED);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("SetPlayState _fdPlayerPlay fail");
        }
        else {
            if (_javaAudioPlayerObj !=
                nullptr) // Set the audioId of the member AudioPlayer.java if set
            {
                JNIEnv *jenv = getJNIEnv();
                if (jenv != nullptr) {
                    jclass audioPlayerClass = jenv->GetObjectClass(_javaAudioPlayerObj);
                    jfieldID audioIdField = jenv->GetFieldID(audioPlayerClass, "_audioId", "I");
                    jenv->SetIntField(_javaAudioPlayerObj, audioIdField, (jint) -1);
                }
            }
            _isHeadAtEnd = true; // Boolean used to force the GC Thread to delete the player
            _loop = false;
            ret = true;
        }
    }
    return ret;
}

/**
 * Setter used by a static method fired when the OpenSL audioEngine reach the end of the sound
 * TODO: This method shouldn't be public
 */
void AudioPlayer::setHeadAtEnd(const bool isHeadAtEnd) noexcept {
    _isHeadAtEnd = isHeadAtEnd;
}

/**
 * Check if we reach the end of the sound or stoped the sound
 */
const bool AudioPlayer::isHeadAtEnd() const noexcept {
    return _isHeadAtEnd;
}

/**
 * Check if we loaded enought data to start playing it
 * Note: It's an extra security to only delete a sound that is in a stable enough state. I'm not 100% it's useful
 */
const bool AudioPlayer::isPrefetchedSufficient() const noexcept {
    return _isPrefetchedSufficientData;
}

/**
 * Get the OpenSL prefeteched status and set the flag if we prefetched enough data
 */
SLuint32 AudioPlayer::getPrefetchedStatus() noexcept {
    SLuint32 status = 0;
    if (_fdPlayerPrefetchedStatus != nullptr) {
        SLresult result = (*_fdPlayerPrefetchedStatus)->GetPrefetchStatus(_fdPlayerPrefetchedStatus,
                                                                          &status);
        if (SL_RESULT_SUCCESS == result) {
            if (status == SL_PREFETCHSTATUS_SUFFICIENTDATA) {
                _isPrefetchedSufficientData = true;
            }
        }
        else {
            LOGEX("GetPrefetchStatus _fdPlayerPrefetchedStatus fail");
            status = 0;
        }
    }
    return status;
}

/**
 * Init hte OpenSL Object required to be able to play the sound
 * Note: We need OpenSL Engine, Mix Obj and AssetManager to be able to init the sound
 */
bool AudioPlayer::initWithEngine(const SLEngineItf &engineEngine,
                                 const SLObjectItf &outputMixObject, AAssetManager *assetManager,
                                 const int audioId, const std::string &fileFullPath,
                                 const float volume, const bool loop) noexcept {
    bool ret = false;
    bool fileFound = false;

    if (engineEngine == nullptr || outputMixObject == nullptr || assetManager == nullptr) {
        return false;
    }

    SLDataSource audioSrc;

    SLDataLocator_AndroidFD loc_fd;
    SLDataLocator_URI loc_uri;

    SLDataFormat_MIME format_mime = {SL_DATAFORMAT_MIME, NULL, SL_CONTAINERTYPE_UNSPECIFIED};
    audioSrc.pFormat = &format_mime;

    if (fileFullPath[0] != '/') {
        std::string relativePath = "";

        const size_t position = fileFullPath.find("assets/");
        if (0 == position) {
            // "assets/" is at the beginning of the path and we don't want it
            const std::string assetsPath = "assets/";
            relativePath += fileFullPath.substr(assetsPath.length());
        }
        else {
            relativePath += fileFullPath;
        }

        AAsset *asset = AAssetManager_open(assetManager, relativePath.c_str(), AASSET_MODE_UNKNOWN);

        // open asset as file descriptor
        off64_t start = 0, length = 0;
        _assetFd = AAsset_openFileDescriptor64(asset, &start, &length);
        AAsset_close(asset);
        if (_assetFd > 0) {
            // configure audio source
            loc_fd.locatorType = SL_DATALOCATOR_ANDROIDFD;
            loc_fd.fd = _assetFd;
            loc_fd.offset = start;
            loc_fd.length = length;

            audioSrc.pLocator = &loc_fd;

            fileFound = true;
        }
    }
    else {
        loc_uri.locatorType = SL_DATALOCATOR_URI;
        loc_uri.URI = (SLchar *) fileFullPath.c_str();
        audioSrc.pLocator = &loc_uri;
        fileFound = true;
    }

    if (fileFound) {
        // configure audio sink
        SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
        SLDataSink audioSnk = {&loc_outmix, NULL};

        // create audio player
        const SLInterfaceID ids[3] = {SL_IID_SEEK, SL_IID_PREFETCHSTATUS, SL_IID_VOLUME};
        const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
        SLresult result = (*engineEngine)->CreateAudioPlayer(engineEngine, &_fdPlayerObject,
                                                             &audioSrc, &audioSnk, 3, ids, req);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("CreateAudioPlayer _fdPlayerObject fail");
            return false;
        }
        // realize the player
        result = (*_fdPlayerObject)->Realize(_fdPlayerObject, SL_BOOLEAN_FALSE);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("Realize _fdPlayerObject fail");
            return false;
        }
        // get the play interface
        result = (*_fdPlayerObject)->GetInterface(_fdPlayerObject, SL_IID_PREFETCHSTATUS,
                                                  &_fdPlayerPrefetchedStatus);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("GetInterface _prefetchedStatus fail");
            return false;
        }
        result = (*_fdPlayerPrefetchedStatus)->SetCallbackEventsMask(_fdPlayerPrefetchedStatus,
                                                                     SL_PREFETCHEVENT_FILLLEVELCHANGE);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("SetCallbackEventsMask _prefetchedStatus fail");
            return false;
        }
        result = (*_fdPlayerPrefetchedStatus)->SetFillUpdatePeriod(_fdPlayerPrefetchedStatus, 10);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("SetFillUpdatePeriod _prefetchedStatus fail");
            return false;
        }
        result = (*_fdPlayerPrefetchedStatus)->RegisterCallback(_fdPlayerPrefetchedStatus,
                                                                AudioPlayer::prefetchEventCallback,
                                                                (void *) (intptr_t) audioId);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("RegisterCallback _prefetchedStatus fail");
            return false;
        }
        // get the play interface
        result = (*_fdPlayerObject)->GetInterface(_fdPlayerObject, SL_IID_PLAY, &_fdPlayerPlay);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("GetInterface _fdPlayerPlay fail");
            return false;
        }
        result = (*_fdPlayerPlay)->SetCallbackEventsMask(_fdPlayerPlay, SL_PLAYEVENT_HEADATEND);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("SetCallbackEventsMask _fdPlayerPlay fail");
            return false;
        }
        result = (*_fdPlayerPlay)->RegisterCallback(_fdPlayerPlay, AudioPlayer::playEventCallback,
                                                    (void *) (intptr_t) audioId);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("RegisterCallback _fdPlayerPlay fail");
            return false;
        }
        // get the seek interface
        result = (*_fdPlayerObject)->GetInterface(_fdPlayerObject, SL_IID_SEEK, &_fdPlayerSeek);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("GetInterface _fdPlayerSeek fail");
            return false;
        }
        // get the volume interface
        result = (*_fdPlayerObject)->GetInterface(_fdPlayerObject, SL_IID_VOLUME, &_fdPlayerVolume);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("GetInterface _fdPlayerVolume fail");
            return false;
        }
        _loop = loop;
        if (loop) {
            result = (*_fdPlayerSeek)->SetLoop(_fdPlayerSeek, SL_BOOLEAN_TRUE, 0, SL_TIME_UNKNOWN);
            if (SL_RESULT_SUCCESS != result) {
                LOGEX("SetLoop _fdPlayerSeek fail");
                return false;
            }
        }

        int dbVolume = 2000 * std::log10(volume);
        if (dbVolume < SL_MILLIBEL_MIN) {
            dbVolume = SL_MILLIBEL_MIN;
        }
        result = (*_fdPlayerVolume)->SetVolumeLevel(_fdPlayerVolume, (SLpermille) dbVolume);
        if (SL_RESULT_SUCCESS != result) {
            LOGEX("SetVolumeLevel _fdPlayerVolume fail");
            return false;
        }
        _volume = volume;

        _audioId = audioId;
        ret = true;
    }

    return ret;
}

void AudioPlayer::prefetchEventCallback(SLPrefetchStatusItf caller, void *context,
                                        SLuint32 prefetchEvent) noexcept {
    if ((prefetchEvent & SL_PREFETCHEVENT_FILLLEVELCHANGE) == SL_PREFETCHEVENT_FILLLEVELCHANGE) {
        int audioId = (int) (intptr_t) context;
        AudioEngine *engine = AudioEngine::getInstance();
        engine->getPrefetchedStatus(audioId);
    }
}

void AudioPlayer::playEventCallback(SLPlayItf caller, void *context, SLuint32 playEvent) noexcept {
    if ((playEvent & SL_PLAYEVENT_HEADATEND) == SL_PLAYEVENT_HEADATEND) {
        int audioId = (int) (intptr_t) context;
        AudioEngine *engine = AudioEngine::getInstance();
        engine->setHeadAtEnd(audioId);
    }
}

const int AudioPlayer::getPlayerId() const noexcept {
    return _audioId;
}

const bool AudioPlayer::isLooping() const noexcept {
    return _loop;
}

void AudioPlayer::setJavaAudioPlayerObj(const jobject obj) noexcept {
    JNIEnv *jenv = getJNIEnv();
    if (jenv != nullptr) {
        if (_javaAudioPlayerObj != nullptr) {
            jenv->DeleteGlobalRef(_javaAudioPlayerObj);
            _javaAudioPlayerObj = nullptr;
        }
        if (obj != nullptr) {
            _javaAudioPlayerObj = jenv->NewGlobalRef(obj);
        }
    }
}
