#define MINIAUDIO_IMPLEMENTATION
#include "audio.h"

#include <extras/decoders/libvorbis/miniaudio_libvorbis.c>
#include <extras/decoders/libvorbis/miniaudio_libvorbis.h>
#include <miniaudio.h>
#include <thread>

namespace Ma {

AudioEngine::AudioEngine() : m_endpoint(nullptr) {
    m_decoders.push_back(ma_decoding_backend_libvorbis);

    ma_resource_manager_config resourceManagerConfig = ma_resource_manager_config_init();
    if (!m_decoders.empty()) {
        resourceManagerConfig.ppCustomDecodingBackendVTables = &m_decoders[0];
        resourceManagerConfig.customDecodingBackendCount = m_decoders.size();
    }
    resourceManagerConfig.jobThreadCount = std::thread::hardware_concurrency();
    ma_result result = ma_resource_manager_init(&resourceManagerConfig, &m_resourceManager);

    if (result != MA_SUCCESS) {
        m_isInitialized = false;
        return;
    }

    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.pResourceManager = &m_resourceManager;
    result = ma_engine_init(&engineConfig, &m_engine);

    if (result != MA_SUCCESS) {
        ma_resource_manager_uninit(&m_resourceManager);
        m_isInitialized = false;
    } else {
        m_isInitialized = true;
    }
    m_endpoint = new AudioNode(reinterpret_cast<ma_node_base*>(ma_engine_get_endpoint(&m_engine)), this);
}

AudioEngine::~AudioEngine() {
    if (m_endpoint) {
        delete m_endpoint;
        m_endpoint = nullptr;
    }
    if (m_isInitialized) {
        ma_resource_manager_uninit(&m_resourceManager);
        ma_engine_uninit(&m_engine);
    }
}

std::unique_ptr<Sound> AudioEngine::createSound(const std::string& filePath, bool stream) {
    if (!m_isInitialized)
        return nullptr;

    auto sound = std::make_unique<Sound>();
    ma_uint32 flags = stream ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;

    if (sound->init(&m_engine, filePath, flags)) {
        return sound;
    }

    return nullptr;
}

std::unique_ptr<Sound> AudioEngine::createSound(const void* data, size_t dataSize, bool stream) {
    if (!m_isInitialized)
        return nullptr;

    auto sound = std::make_unique<Sound>();

    if (sound->init(&m_engine, data, dataSize, stream)) {
        return sound;
    }

    return nullptr;
}

void AudioEngine::setMasterVolume(float volume) {
    if (m_isInitialized) {
        ma_engine_set_volume(&m_engine, volume);
    }
}

float AudioEngine::getMasterVolume() const {
    if (!m_isInitialized)
        return 0.0f;
    return ma_engine_get_volume(&m_engine);
}

void AudioEngine::setListenerPosition(unsigned int listenerIndex, float x, float y, float z) {
    if (m_isInitialized) {
        ma_engine_listener_set_position(&m_engine, listenerIndex, x, y, z);
    }
}

void AudioEngine::setListenerDirection(unsigned int listenerIndex, float x, float y, float z) {
    if (m_isInitialized) {
        ma_engine_listener_set_direction(&m_engine, listenerIndex, x, y, z);
    }
}

void AudioEngine::setListenerWorldUp(unsigned int listenerIndex, float x, float y, float z) {
    if (m_isInitialized) {
        ma_engine_listener_set_world_up(&m_engine, listenerIndex, x, y, z);
    }
}

// Static callback for device enumeration
static ma_bool32 enum_device_callback(ma_context* pContext, ma_device_type deviceType, const ma_device_info* pInfo,
                                      void* pUserData) {
    (void)pContext; // Unused
    if (deviceType == ma_device_type_playback) {
        auto* deviceNames = static_cast<std::vector<std::string>*>(pUserData);
        deviceNames->push_back(pInfo->name);
    }
    return MA_TRUE;
}

std::vector<std::string> AudioEngine::getPlaybackDeviceNames() {
    std::vector<std::string> deviceNames;
    ma_context context;

    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        return deviceNames;
    }

    ma_context_enumerate_devices(&context, enum_device_callback, &deviceNames);

    ma_context_uninit(&context);
    return deviceNames;
}

Sound::Sound() {}

void Sound::close() {
    if (m_isInitialized && m_sound) {
        ma_sound_uninit(&*m_sound);
        m_sound.reset();
    }
    if (m_decoder) {
        delete m_decoder;
    }
    if (m_audioBuffer) {
        ma_free(m_audioBuffer, nullptr);
    }
    m_isInitialized = false;
}

Sound::~Sound() {
    close();
}
bool Sound::init(ma_engine* engine, const std::string& filePath, ma_uint32 flags) {
    close();
    m_sound = std::make_unique<ma_sound>();
    if (!m_sound)
        return false;
    ma_result result = ma_sound_init_from_file(engine, filePath.c_str(), flags, nullptr, nullptr, &*m_sound);
    if (result != MA_SUCCESS) {
        m_sound.reset();
        return false;
    }
    m_isInitialized = true;
    return true;
}

bool Sound::init(ma_engine* engine, const void* data, size_t dataSize, bool stream) {
    close();
    m_sound = std::make_unique<ma_sound>();
    if (!m_sound)
        return false;
    ma_result result = MA_ERROR;
    ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION;

    if (stream) {
        m_decoder = new ma_decoder();
        if (!m_decoder)
            return false;

        result = ma_decoder_init_memory(data, dataSize, nullptr, m_decoder);
        if (result != MA_SUCCESS) {
            delete m_decoder;
            m_decoder = nullptr;
            m_sound.reset();
            return false;
        }

        result = ma_sound_init_from_data_source(engine, m_decoder, flags, NULL, &*m_sound);
        if (result != MA_SUCCESS) {
            ma_decoder_uninit(m_decoder);
            delete m_decoder;
            m_decoder = nullptr;
            m_sound.reset();
            return false;
        }
    } else {
        ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, ma_engine_get_sample_rate(engine));
        ma_uint64 frameCount = 0;
        float* pPCMFrames = nullptr;

        result = ma_decode_memory(data, dataSize, &decoderConfig, &frameCount, (void**)&pPCMFrames);
        if (result != MA_SUCCESS) {
            m_sound.reset();
            return false;
        }

        m_audioBuffer = (ma_audio_buffer*)ma_malloc(sizeof(ma_audio_buffer), nullptr);
        if (!m_audioBuffer) {
            ma_free(pPCMFrames, nullptr);
            m_sound.reset();
            return false;
        }

        ma_audio_buffer_config audioBufferConfig =
            ma_audio_buffer_config_init(decoderConfig.format, decoderConfig.channels, frameCount, pPCMFrames, nullptr);
        result = ma_audio_buffer_init(&audioBufferConfig, m_audioBuffer);
        if (result != MA_SUCCESS) {
            ma_free(m_audioBuffer, nullptr);
            m_audioBuffer = nullptr;
            ma_free(pPCMFrames, nullptr);
            m_sound.reset();
            return false;
        }
        m_audioBuffer->ownsData = MA_TRUE;

        result = ma_sound_init_from_data_source(engine, m_audioBuffer, flags, NULL, &*m_sound);
        if (result != MA_SUCCESS) {
            ma_audio_buffer_uninit(m_audioBuffer);
            ma_free(m_audioBuffer, nullptr);
            m_audioBuffer = nullptr;
            m_sound.reset();
            return false;
        }
    }

    m_isInitialized = true;
    return true;
}

bool Sound::play() {
    if (!m_isInitialized)
        return false;
    return ma_sound_start(&*m_sound) == MA_SUCCESS;
}

bool Sound::stop() {
    if (!m_isInitialized)
        return false;
    return ma_sound_stop(&*m_sound) == MA_SUCCESS;
}

bool Sound::isPlaying() const {
    if (!m_isInitialized)
        return false;
    return ma_sound_is_playing(&*m_sound);
}

bool Sound::atEnd() const {
    if (!m_isInitialized)
        return true;
    return ma_sound_at_end(&*m_sound);
}

void Sound::setVolume(float volume) {
    if (m_isInitialized) {
        ma_sound_set_volume(&*m_sound, volume);
    }
}

float Sound::getVolume() const {
    if (!m_isInitialized)
        return 0.0f;
    return ma_sound_get_volume(&*m_sound);
}

void Sound::setPan(float pan) {
    if (m_isInitialized) {
        ma_sound_set_pan(&*m_sound, pan);
    }
}

float Sound::getPan() const {
    if (!m_isInitialized)
        return 0.0f;
    return ma_sound_get_pan(&*m_sound);
}

void Sound::setPitch(float pitch) {
    if (m_isInitialized) {
        ma_sound_set_pitch(&*m_sound, pitch);
    }
}

float Sound::getPitch() const {
    if (!m_isInitialized)
        return 1.0f;
    return ma_sound_get_pitch(&*m_sound);
}

void Sound::setLooping(bool isLooping) {
    if (m_isInitialized) {
        ma_sound_set_looping(&*m_sound, isLooping);
    }
}

bool Sound::isLooping() const {
    if (!m_isInitialized)
        return false;
    return ma_sound_is_looping(&*m_sound);
}

bool Sound::seekToFrame(ma_uint64 frameIndex) {
    if (!m_isInitialized)
        return false;
    return ma_sound_seek_to_pcm_frame(&*m_sound, frameIndex) == MA_SUCCESS;
}

ma_uint64 Sound::getLengthInFrames() const {
    if (!m_isInitialized)
        return 0;
    ma_uint64 length = 0;
    ma_sound_get_length_in_pcm_frames(&*m_sound, &length);
    return length;
}

ma_uint64 Sound::getCursorInFrames() const {
    if (!m_isInitialized)
        return 0;
    ma_uint64 cursor = 0;
    ma_sound_get_cursor_in_pcm_frames(&*m_sound, &cursor);
    return cursor;
}

void Sound::setPositioning(ma_positioning positioning) {
    if (m_isInitialized) {
        ma_sound_set_positioning(&*m_sound, positioning);
    }
}

void Sound::setPosition(float x, float y, float z) {
    if (m_isInitialized) {
        ma_sound_set_position(&*m_sound, x, y, z);
    }
}

void Sound::setVelocity(float x, float y, float z) {
    if (m_isInitialized) {
        ma_sound_set_velocity(&*m_sound, x, y, z);
    }
}

void Sound::setDirection(float x, float y, float z) {
    if (m_isInitialized) {
        ma_sound_set_direction(&*m_sound, x, y, z);
    }
}

void Sound::setSpatialization(bool enabled) {
    if (m_isInitialized) {
        ma_sound_set_spatialization_enabled(&*m_sound, enabled);
    }
}

bool Sound::pause() {
    return stop();
}

bool Sound::isPaused() const {
    if (!m_isInitialized)
        return false;
    return !isPlaying() && !atEnd();
}

void Sound::setFade(float volumeStart, float volumeEnd, ma_uint64 durationInFrames) {
    if (m_isInitialized) {
        ma_sound_set_fade_in_pcm_frames(&*m_sound, volumeStart, volumeEnd, durationInFrames);
    }
}

void Sound::setTimedFade(float volumeStart, float volumeEnd, ma_uint64 durationInFrames,
                         ma_uint64 absoluteTimeInFrames) {
    if (m_isInitialized) {
        ma_sound_set_fade_start_in_pcm_frames(&*m_sound, volumeStart, volumeEnd, durationInFrames,
                                              absoluteTimeInFrames);
    }
}

float Sound::getCurrentFadeVolume() const {
    if (!m_isInitialized)
        return 0.0f;
    return ma_sound_get_current_fade_volume(&*m_sound);
}

void Sound::setStartTime(ma_uint64 absoluteTimeInFrames) {
    if (m_isInitialized) {
        ma_sound_set_start_time_in_pcm_frames(&*m_sound, absoluteTimeInFrames);
    }
}

void Sound::setStopTime(ma_uint64 absoluteTimeInFrames) {
    if (m_isInitialized) {
        ma_sound_set_stop_time_in_pcm_frames(&*m_sound, absoluteTimeInFrames);
    }
}

void Sound::setStopTimeWithFade(ma_uint64 absoluteTimeInFrames, ma_uint64 fadeDurationInFrames) {
    if (m_isInitialized) {
        ma_sound_set_stop_time_with_fade_in_pcm_frames(&*m_sound, absoluteTimeInFrames, fadeDurationInFrames);
    }
}

void Sound::setPanMode(ma_pan_mode mode) {
    if (m_isInitialized) {
        ma_sound_set_pan_mode(&*m_sound, mode);
    }
}

ma_pan_mode Sound::getPanMode() const {
    if (!m_isInitialized)
        return ma_pan_mode_balance; // Default value from miniaudio
    return ma_sound_get_pan_mode(&*m_sound);
}

void Sound::setAttenuationModel(ma_attenuation_model model) {
    if (m_isInitialized) {
        ma_sound_set_attenuation_model(&*m_sound, model);
    }
}

ma_attenuation_model Sound::getAttenuationModel() const {
    if (!m_isInitialized)
        return ma_attenuation_model_inverse; // Default value from miniaudio
    return ma_sound_get_attenuation_model(&*m_sound);
}

void Sound::setRolloff(float rolloff) {
    if (m_isInitialized) {
        ma_sound_set_rolloff(&*m_sound, rolloff);
    }
}

float Sound::getRolloff() const {
    if (!m_isInitialized)
        return 1.0f; // Default value from miniaudio
    return ma_sound_get_rolloff(&*m_sound);
}

void Sound::setMinDistance(float minDistance) {
    if (m_isInitialized) {
        ma_sound_set_min_distance(&*m_sound, minDistance);
    }
}

float Sound::getMinDistance() const {
    if (!m_isInitialized)
        return 1.0f; // Default value from miniaudio
    return ma_sound_get_min_distance(&*m_sound);
}

void Sound::setMaxDistance(float maxDistance) {
    if (m_isInitialized) {
        ma_sound_set_max_distance(&*m_sound, maxDistance);
    }
}

float Sound::getMaxDistance() const {
    if (!m_isInitialized)
        return 1000.0f; // A common large default
    return ma_sound_get_max_distance(&*m_sound);
}

void Sound::setCone(float innerAngleRadians, float outerAngleRadians, float outerGain) {
    if (m_isInitialized) {
        ma_sound_set_cone(&*m_sound, innerAngleRadians, outerAngleRadians, outerGain);
    }
}

void Sound::getCone(float* innerAngleRadians, float* outerAngleRadians, float* outerGain) const {
    if (m_isInitialized && innerAngleRadians && outerAngleRadians && outerGain) {
        ma_sound_get_cone(&*m_sound, innerAngleRadians, outerAngleRadians, outerGain);
    } else {
        // Fill with default values if not initialized or pointers are null
        if (innerAngleRadians)
            *innerAngleRadians = 6.283185f; // 360 degrees
        if (outerAngleRadians)
            *outerAngleRadians = 6.283185f; // 360 degrees
        if (outerGain)
            *outerGain = 0.0f;
    }
}

void Sound::setDopplerFactor(float factor) {
    if (m_isInitialized) {
        ma_sound_set_doppler_factor(&*m_sound, factor);
    }
}

float Sound::getDopplerFactor() const {
    if (!m_isInitialized)
        return 1.0f; // Default value from miniaudio
    return ma_sound_get_doppler_factor(&*m_sound);
}
} // namespace Ma
