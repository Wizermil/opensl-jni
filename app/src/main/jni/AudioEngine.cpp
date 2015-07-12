#include "AudioEngine.h"
#include "AudioUtils.h"

using namespace audio;

extern "C"
{
    static JavaVM *gVm = nullptr;

    /**
     * Return the JNIEnv and handle multithreaded env for the code to interact with Java
     */
    JNIEnv *getJNIEnv()
    {
        JNIEnv *env = nullptr;
        if (gVm != nullptr)
        {
            jint ret = gVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
            switch (ret)
            {
                case JNI_OK :
                    break;
                case JNI_EDETACHED :
                    if (gVm->AttachCurrentThread(&env, NULL) < 0)
                    {
                        env = nullptr;
                    }
                    break;
                case JNI_EVERSION :
                default :
                    env = nullptr;
                    break;
            }
        }
        return env;
    }

    /**
     * Return the audioId memeber of AudioPlayer.java class
     */
    int getAudioId(JNIEnv *env, jobject javaAudioPlayer)
    {
        int ret = -1;
        if (env != nullptr)
        {
            jclass audioPlayerClass = env->GetObjectClass(javaAudioPlayer);
            if (audioPlayerClass != nullptr)
            {
                jfieldID audioIdField = env->GetFieldID(audioPlayerClass, "_audioId", "I");
                if (audioIdField != nullptr)
                {
                    ret = env->GetIntField(javaAudioPlayer, audioIdField);
                }
            }
        }
        return ret;
    }

    /**
     * Set audioId member of AudioPlayer.java
     */
    bool setAudioId(JNIEnv *env, jobject javaAudioPlayer, int audioId)
    {
        bool ret = false;
        if (env != nullptr)
        {
            jclass audioPlayerClass = env->GetObjectClass(javaAudioPlayer);
            if (audioPlayerClass != nullptr)
            {
                jfieldID audioIdField = env->GetFieldID(audioPlayerClass, "_audioId", "I");
                if (audioIdField != nullptr)
                {
                    env->SetIntField(javaAudioPlayer, audioIdField, (jint) audioId);
                    ret = true;
                }
            }
        }
        return ret;
    }

    /**
     * Init the libaudio
     * Cache JavaVM*
     * Note: Maybe it would better to also cache JNIEnv*
     */
    JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
    {
        JNIEnv *env = nullptr;
        if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK)
        {
            return JNI_ERR;
        }

        gVm = vm;

        return JNI_VERSION_1_6;
    }

    /**
     * Unload libaudio
     * Make sure that we clean all our singletons correctly if the lib must be unload
     */
    JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved)
    {
        gVm = nullptr;
        AudioEngine::getInstance()->destroy();
    }

    /**
     * Init OpenSL to then interact with a sound (play/stop/pause/resume/setVolume/setParams)
     * Implementation of the init method in AudioPlayer.java
     */
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_init(JNIEnv *env, jobject thiz, jstring path, jfloat volume, jboolean loop)
    {
        bool ret = false;

        const int audioId = getAudioId(env, thiz); // try to recover the audioIde from the AudioPlayer.java object

        if (audioId < 0) {
            const char *pathC = env->GetStringUTFChars(path, nullptr);
            AudioPlayer *player = AudioEngine::getInstance()->createPlayerWithPath(pathC, (float) volume, (bool) loop); // Can return nullptr if the audio engine or the assemanager is not init correctly
            env->ReleaseStringUTFChars(path, pathC);

            if (player != nullptr)
            {
                setAudioId(env, thiz, player->getPlayerId()); // Set the audioId member of java class AudioPlayer.java
                player->setJavaAudioPlayerObj( thiz); // Store the jobject in GlobalRef to be able to easily set the audioId back when you stop a sound

                ret = true;
            }
        }
        return ret;
    }

    /**
     * Stop a sound
     * Implementation of the stop method in AudioPlayer.java
     */
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_stop(JNIEnv *env, jobject thiz)
    {
        bool ret = false;
        AudioEngine *engine = AudioEngine::getInstance();

        const int audioId = getAudioId(env, thiz); // try to recover the audioIde from the AudioPlayer.java object

        if (audioId > 0)
        {
            ret = engine->stop(audioId);
        }
        return ret;
    }

    /**
     * Play a sound
     * Implementation of the play method in AudioPlayer.java
    */
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_play(JNIEnv *env, jobject thiz)
    {
        bool ret = false;
        AudioEngine *engine = AudioEngine::getInstance();

        const int audioId = getAudioId(env, thiz); // try to recover the audioIde from the AudioPlayer.java object

        if (audioId > 0)
        {
            ret = engine->play(audioId);
        }
        return ret;
    }

    /**
     * Resume a sound that has been paused
     * Implementation of the resume method in AudioPlayer.java
     */
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_resume(JNIEnv *env, jobject thiz)
    {
        bool ret = false;
        AudioEngine *engine = AudioEngine::getInstance();

        const int audioId = getAudioId(env, thiz);

        if (audioId > 0)
        {
            ret = engine->resume(audioId);
        }
        return ret;
    }

    /**
     * Pause a sound
     * Implementation of the pause method in AudioPlayer.java
     */
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_pause(JNIEnv *env, jobject thiz)
    {
        bool ret = false;
        AudioEngine *engine = AudioEngine::getInstance();

        const int audioId = getAudioId(env, thiz); // try to recover the audioIde from the AudioPlayer.java object

        if (audioId > 0)
        {
            ret = engine->pause(audioId);
        }
        return ret;
    }

    /**
     * Change pitch, pan and gain of a sound
     * Implementation of the setParams method in AudioPlayer.java
     */
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_setParams(JNIEnv *env, jobject thiz, jfloat pitch, jfloat pan, jfloat volume) {
        bool ret = false;
        AudioEngine *engine = AudioEngine::getInstance();

        const int audioId = getAudioId(env, thiz); // try to recover the audioIde from the AudioPlayer.java object

        if (audioId > 0)
        {
            ret = engine->setParams(audioId, (float) pitch, (float) pan, (float) volume);
        }
        return ret;
    }

    /**
     * Change volume of a sound
     * Implementation of the setVolume method in AudioPlayer.java
     */
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioPlayer_setVolume(JNIEnv *env, jobject thiz, jfloat volume)
    {
        bool ret = false;
        AudioEngine *engine = AudioEngine::getInstance();

        const int audioId = getAudioId(env, thiz); // try to recover the audioIde from the AudioPlayer.java object

        if (audioId > 0)
        {
            ret = engine->setVolume(audioId, (float) volume);
        }
        return ret;
    }

    /**
     * Implementation of the setAssetManager method in AudioEngine.java
     * TODO: Optimize how we set the java AssetManager to reach sound in /assets
     */
    JNIEXPORT void JNICALL Java_com_prettysimple_audio_AudioEngine_setAssetManager(JNIEnv *env, jobject thiz, jobject assetManager)
    {
        AudioEngine::getInstance()->setAssetManager(assetManager);
    }

    /**
     * Implementation of pauseAll method in AudioEngine.java
     * It's a convinient method to easily pause all the sounds at once
     */
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioEngine_pauseAll(JNIEnv *env, jobject thiz)
    {
        return AudioEngine::getInstance()->pauseAll();
    }

    /**
      * Implementation of pauseAll method in AudioEngine.java
      * It's a convinient method to easily pause all the sounds at once
      */
    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioEngine_resumeAll(JNIEnv *env, jobject thiz)
    {
        return AudioEngine::getInstance()->resumeAll();
    }

    JNIEXPORT bool JNICALL Java_com_prettysimple_audio_AudioEngine_stopAll(JNIEnv *env, jobject thiz)
    {
        return AudioEngine::getInstance()->stopAll();
    }
}

AudioEngine *AudioEngine::_instance = nullptr;

AudioEngine::AudioEngine() : _audioIds(0)
, _engineObject(nullptr)
, _engineEngine(nullptr)
, _outputMixObject(nullptr)
, _assetManager(nullptr)
, _stopGc(false)
, _doneGc(false)
{
}

AudioEngine::~AudioEngine()
{
    clean();
    { // Delete all the AudioPlayers stored in the engine
        std::lock_guard<std::mutex> lock(_playersMutex);
        for (auto it = _players.begin(); it != _players.end();)
        {
            it->second->stop();
            const AudioPlayer *tmp = it->second;
            it = _players.erase(it);
            delete tmp;
        }
    }
    JNIEnv *jenv = getJNIEnv(); // Delete the GlobalRef on AssetManager to be GC
    if (jenv != nullptr && _assetManager != nullptr)
    {
        jenv->DeleteGlobalRef(_assetManager);
        _assetManager = nullptr;
    }
    _audioIds = 0;
}

/**
 * Destroy OpenSL Objects and GC Thread running
 */
void AudioEngine::clean() noexcept
{
    if (_outputMixObject)
    {
        (*_outputMixObject)->Destroy(_outputMixObject);
        _outputMixObject = nullptr;
    }
    if (_engineObject)
    {
        (*_engineObject)->Destroy(_engineObject);
        _engineObject = nullptr;
    }
    _engineEngine = nullptr;

    if (!_doneGc && _threadGc.joinable()) // Kill the thread in charge of cleaning up the list of *AudioPlayer
    {
        _stopGc = true;
        _condition.notify_all();
        _threadGc.join();
        _doneGc = false;
        _stopGc = false;
    }
}

/**
 * Cleanup the singleton
 */
void AudioEngine::destroy() noexcept
{
    delete AudioEngine::_instance;
    AudioEngine::_instance = nullptr;
}

AudioEngine *AudioEngine::getInstance() noexcept
{
    if (_instance == nullptr)
    {
        AudioEngine::_instance = new AudioEngine();
    }

    return AudioEngine::_instance;
}

/**
 * Return the AAssetMaanger from the jobect stored as GlobalRef
 */
AAssetManager *AudioEngine::getAssetManager() const noexcept
{
    JNIEnv *jenv = getJNIEnv();
    AAssetManager *ret = nullptr;
    if (_assetManager != nullptr && jenv != nullptr)
    {
        ret = AAssetManager_fromJava(jenv, _assetManager);
    }
    return ret;
}

/**
 * Store AssetManager jobject as GlobalRef
 */
void AudioEngine::setAssetManager(const jobject assetManager)
{
    JNIEnv *jenv = getJNIEnv();
    if (jenv != nullptr) {
        if (_assetManager != nullptr) // Just a security to make sure that we don't create too many GlobalRef
        {
            jenv->DeleteGlobalRef(_assetManager);
            _assetManager = nullptr;
        }
        if (assetManager != nullptr)
        {
            _assetManager = jenv->NewGlobalRef(assetManager);
        }
    }
}

/**
 * Factory to create *AudioPlayer and easily managed lifecycle of the objects
 */
AudioPlayer *AudioEngine::createPlayerWithPath(const std::string &fileFullPath, const float volume, const bool loop) noexcept
{
    AudioPlayer *ret = nullptr;
    if (initOpenSL() && _assetManager != nullptr)
    {
        ret = new AudioPlayer();
        const bool init = ret->initWithEngine(_engineEngine, _outputMixObject, getAssetManager(),  ++_audioIds, fileFullPath, volume, loop);
        if (init)
        {
            std::lock_guard<std::mutex> lock(_playersMutex); // /!\ a thread (audioPlayerGc) in charge of deleting instances of AudioEngine is running
            _players[_audioIds] = ret;
            _condition.notify_one(); // to decrease cpu usage of the thread he can be in stasis
        }
        else
        { // If we are not able to create the AudioPlayer we clean the memory
            delete ret;
            ret = nullptr;
        }
    }
    return ret;
}

/**
 * Init the OpenSL audio engine and mix to be able to play sounds
 */
bool AudioEngine::initOpenSL() noexcept
{
    if (_engineEngine != nullptr && _outputMixObject != nullptr)
    {
        return true;
    }

    bool error = true;

    // create engine
    SLEngineOption EngineOption[] = { (SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE };

    SLresult result = slCreateEngine(&_engineObject, 1, EngineOption, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS == result)
    {
        // realize the engine
        result = (*_engineObject)->Realize(_engineObject, SL_BOOLEAN_FALSE);
        if (SL_RESULT_SUCCESS == result)
        {
            // get the engine interface, which is needed in order to create other objects
            result = (*_engineObject)->GetInterface(_engineObject, SL_IID_ENGINE, &_engineEngine);
            if (SL_RESULT_SUCCESS == result)
            {
                // create output mix
                const SLInterfaceID outputMixIIDs[] = {};
                const SLboolean outputMixReqs[] = {};
                result = (*_engineEngine)->CreateOutputMix(_engineEngine, &_outputMixObject, 0, outputMixIIDs, outputMixReqs);
                if (SL_RESULT_SUCCESS == result)
                {
                    // realize the output mix
                    result = (*_outputMixObject)->Realize(_outputMixObject, SL_BOOLEAN_FALSE);
                    if (SL_RESULT_SUCCESS == result)
                    {
                        if (!_doneGc)
                        {
                            _threadGc = std::thread(&AudioEngine::audioPlayerGc, this, 100);

                            // TODO: Remove this trhead that is only used to rerproduce a bug in OpenSL Destroy Object will be fixed.
                            _threadTest = std::thread(&AudioEngine::audioPlayerTest, this, 4 * 16);
                        }
                        error = false;
                    }
                    else
                    {
                        LOGEX("Realize _outputMixObject fail");
                    }
                }
                else
                {
                    LOGEX("CreateOutputMix _engineEngine fail");
                }
            }
            else
            {
                LOGEX("GetInterface _engineObject fail");
            }
        }
        else
        {
            LOGEX("Realize _engineObject fail");
        }
    }
    else
    {
        LOGEX("slCreateEngine _engineObject fail");
    }

    if (error) // If there is an error I clean the memory
    {
        clean();
    }
    return !error;
}

/**
 * Thread in charge of deleting *AudioPlayer to keep the memory clean
 */
void AudioEngine::audioPlayerGc(const int sleep) noexcept
{
    while (!_stopGc)
    {
        size_t playersLength = 0;
        { // We check if there is an *AudioPlayer that can be destroyed (there is a limit of AudioPlayer that can run at the same time)
            std::lock_guard<std::mutex> lock(_playersMutex);
            for (auto it = _players.begin(); it != _players.end();)
            {
                if (it->second->isHeadAtEnd() && it->second->isPrefetchedSufficient() && !it->second->isLooping())
                {
                    it->second->stop();
                    const AudioPlayer *tmp = it->second;
                    it = _players.erase(it);
                    delete tmp;
                }
                else
                {
                    ++it;
                }
            }
            playersLength = _players.size();
        }
        if (playersLength <= 0) // Put the thread in stasis if there are no sounds playing
        {
            std::unique_lock<std::mutex> lock(_pauselMutex);
            _condition.wait(lock);
        }
        std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
    }
    _doneGc = true;
}

void AudioEngine::audioPlayerTest(const int sleep) noexcept
{
    int audioId = -1;
    bool leftPlaying = false;
    AudioPlayer *sound = nullptr;
    std::string soundPath = "";

    while (!_stopGc)
    {
        if (audioId != -1)
        {
            std::lock_guard<std::mutex> lock(_playersMutex);
            const auto &it = _players.find(audioId);
            if (it != _players.end())
            {
                it->second->stop();
                audioId = -1;
            }
        }

        if (!leftPlaying)
        {
            soundPath = "cse_dialog1.ogg";
        }
        else
        {
            soundPath = "cse_dialog2.ogg";
        }

        leftPlaying = !leftPlaying;

        sound = createPlayerWithPath(soundPath, 1.f, true);
        if (sound != nullptr)
        {
            sound->play();
            audioId = sound->getPlayerId();
        }
        std::this_thread::yield();
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
    }
}

bool AudioEngine::stop(const int audioId) noexcept
{
    bool ret = false;
    std::lock_guard<std::mutex> lock(_playersMutex);
    const auto &it = _players.find(audioId);
    if (it != _players.end())
    {
        ret = it->second->stop();
    }
    return ret;
}

bool AudioEngine::play(const int audioId) noexcept
{
    bool ret = false;
    std::lock_guard<std::mutex> lock(_playersMutex);
    const auto &it = _players.find(audioId);
    if (it != _players.end())
    {
        ret = it->second->play();
    }
    return ret;
}

bool AudioEngine::pause(const int audioId) noexcept
{
    bool ret = false;
    std::lock_guard<std::mutex> lock(_playersMutex);
    const auto &it = _players.find(audioId);
    if (it != _players.end())
    {
        ret = it->second->pause();
    }
    return ret;
}

bool AudioEngine::resume(const int audioId) noexcept
{
    bool ret = false;
    std::lock_guard<std::mutex> lock(_playersMutex);
    const auto &it = _players.find(audioId);
    if (it != _players.end()) {
        ret = it->second->resume();
    }
    return ret;
}

bool AudioEngine::setParams(const int audioId, const float pitch, const float pan, const float volume) noexcept
{
    bool ret = false;
    std::lock_guard<std::mutex> lock(_playersMutex);
    const auto &it = _players.find(audioId);
    if (it != _players.end())
    {
        ret = it->second->setParams(pitch, pan, volume);
    }
    return ret;
}

bool AudioEngine::setVolume(const int audioId, const float volume) noexcept
{
    bool ret = false;
    std::lock_guard<std::mutex> lock(_playersMutex);
    const auto &it = _players.find(audioId);
    if (it != _players.end()) {
        ret = it->second->setVolume(volume);
    }
    return ret;
}

/**
 * Stop all AudioPlayers
 */
bool AudioEngine::stopAll() noexcept
{
    bool ret = true;
    std::lock_guard<std::mutex> lock(_playersMutex);
    for (const auto &it : _players)
    {
        ret &= it.second->stop();
    }
    return ret;
}

/**
 * Pause all AudioPlayers
 */
bool AudioEngine::pauseAll() noexcept
{
    bool ret = true;
    std::lock_guard<std::mutex> lock(_playersMutex);
    for (const auto &it : _players)
    {
        ret &= it.second->pause();
    }
    return ret;
}

void AudioEngine::setHeadAtEnd(const int audioId) noexcept
{
    std::lock_guard<std::mutex> lock(_playersMutex);
    const auto &it = _players.find(audioId);
    if (it != _players.end())
    {
        it->second->setHeadAtEnd(true);
    }
}

SLuint32 AudioEngine::getPrefetchedStatus(const int audioId) noexcept
{
    SLuint32 ret = 0;
    std::lock_guard<std::mutex> lock(_playersMutex);
    const auto &it = _players.find(audioId);
    if (it != _players.end())
    {
        ret = it->second->getPrefetchedStatus();
    }
    return ret;
}

/**
 * Resume all AudioPlayers
 */
bool AudioEngine::resumeAll() noexcept
{
    bool ret = true;
    std::lock_guard<std::mutex> lock(_playersMutex);
    for (const auto &it : _players)
    {
        ret &= it.second->resume();
    }
    return ret;
}
