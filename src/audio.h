#ifndef MINIAUDIO_WRAPPER_H
#define MINIAUDIO_WRAPPER_H
#include "singleton.h"

#include <memory>
#include <miniaudio.h>
#include <string>
#include <vector>

namespace Ma {

class AudioEngine;

class AudioNode {
  protected:
    ma_node_base* node;
    AudioEngine* engine;

  public:
    AudioNode() : node(nullptr) {}
    AudioNode(ma_node_base* node, AudioEngine* engine) : node(node), engine(engine) {}

    ~AudioNode() = default;
    inline ma_node_base* getMaNode() { return node; }

    inline AudioEngine* getEngine() const { return engine; }

    inline unsigned int getInputBusCount() { return node ? ma_node_get_input_bus_count(node) : 0; }

    inline unsigned int getOutputBusCount() { return node ? ma_node_get_output_bus_count(node) : 0; }

    inline unsigned int getInputChannels(unsigned int bus) { return node ? ma_node_get_input_channels(node, bus) : 0; }

    inline unsigned int getOutputChannels(unsigned int bus) {
        return node ? ma_node_get_output_channels(node, bus) : 0;
    }

    inline bool attachOutputBus(unsigned int output_bus, AudioNode* destination, unsigned int destination_input_bus) {
        return node && destination
                   ? (ma_node_attach_output_bus(node, output_bus, destination->getMaNode(), destination_input_bus)) ==
                         MA_SUCCESS
                   : false;
    }

    inline bool detachOutputBus(unsigned int bus) {
        return node ? ma_node_detach_output_bus(node, bus) == MA_SUCCESS : false;
    }

    inline bool detachAllOutputBuses() { return node ? ma_node_detach_all_output_buses(node) == MA_SUCCESS : false; }

    inline bool setOutputBusVolume(unsigned int bus, float volume) {
        return node ? ma_node_set_output_bus_volume(node, bus, volume) == MA_SUCCESS : false;
    }

    inline float getOutputBusVolume(unsigned int bus) { return node ? ma_node_get_output_bus_volume(node, bus) : 0.0f; }

    inline bool setState(ma_node_state state) { return node ? ma_node_set_state(node, state) == MA_SUCCESS : false; }

    inline ma_node_state getState() { return node ? ma_node_get_state(node) : ma_node_state_stopped; }

    inline bool setStateTime(ma_node_state state, unsigned long long time) {
        return node ? ma_node_set_state_time(node, state, time) == MA_SUCCESS : false;
    }

    inline unsigned long long getStateTime(ma_node_state state) {
        return node ? ma_node_get_state_time(node, state) : 0;
    }

    inline ma_node_state getStateByTime(unsigned long long global_time) {
        return node ? ma_node_get_state_by_time(node, global_time) : ma_node_state_stopped;
    }

    inline ma_node_state getStateByTimeRange(unsigned long long global_time_begin, unsigned long long global_time_end) {
        return node ? ma_node_get_state_by_time_range(node, global_time_begin, global_time_end) : ma_node_state_stopped;
    }

    inline unsigned long long getTime() { return node ? ma_node_get_time(node) : 0; }

    inline bool setTime(unsigned long long local_time) {
        return node ? ma_node_set_time(node, local_time) == MA_SUCCESS : false;
    }
};

class Sound {
  public:
    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;
    ~Sound();

    void close();
    bool play();
    bool stop();
    bool isPlaying() const;
    bool atEnd() const;

    void setVolume(float volume);
    float getVolume() const;
    void setPan(float pan);
    float getPan() const;
    void setPitch(float pitch);
    float getPitch() const;
    void setLooping(bool isLooping);
    bool isLooping() const;

    bool seekToFrame(ma_uint64 frameIndex);
    ma_uint64 getLengthInFrames() const;
    ma_uint64 getCursorInFrames() const;

    void setPositioning(ma_positioning positioning);
    void setPosition(float x, float y, float z);
    void setVelocity(float x, float y, float z);
    void setDirection(float x, float y, float z);
    void setSpatialization(bool enabled);

    bool pause();
    bool isPaused() const;

    void setFade(float volumeStart, float volumeEnd, ma_uint64 durationInFrames);
    void setTimedFade(float volumeStart, float volumeEnd, ma_uint64 durationInFrames, ma_uint64 absoluteTimeInFrames);
    float getCurrentFadeVolume() const;

    void setStartTime(ma_uint64 absoluteTimeInFrames);
    void setStopTime(ma_uint64 absoluteTimeInFrames);
    void setStopTimeWithFade(ma_uint64 absoluteTimeInFrames, ma_uint64 fadeDurationInFrames);

    void setPanMode(ma_pan_mode mode);
    ma_pan_mode getPanMode() const;
    void setAttenuationModel(ma_attenuation_model model);
    ma_attenuation_model getAttenuationModel() const;
    void setRolloff(float rolloff);
    float getRolloff() const;
    void setMinDistance(float minDistance);
    float getMinDistance() const;
    void setMaxDistance(float maxDistance);
    float getMaxDistance() const;
    void setCone(float innerAngleRadians, float outerAngleRadians, float outerGain);
    void getCone(float* innerAngleRadians, float* outerAngleRadians, float* outerGain) const;
    void setDopplerFactor(float factor);
    float getDopplerFactor() const;

    Sound();

  private:
    friend class AudioEngine;
    mutable std::unique_ptr<ma_sound> m_sound;
    ma_decoder* m_decoder = nullptr;          // Used when streaming from memory
    ma_audio_buffer* m_audioBuffer = nullptr; // Used when pre-decoding from memory
    bool m_isInitialized = false;
    // Initializers called by AudioEngine
    bool init(ma_engine* engine, const std::string& filePath, ma_uint32 flags);
    bool init(ma_engine* engine, const void* data, size_t dataSize, bool stream);
};

class AudioEngine {
    AudioNode* m_endpoint;

  public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    inline AudioNode* getEndpoint() { return m_endpoint; }

    std::unique_ptr<Sound> createSound(const std::string& filePath, bool stream = false);

    std::unique_ptr<Sound> createSound(const void* data, size_t dataSize, bool stream = false);

    void setMasterVolume(float volume);
    float getMasterVolume() const;

    void setListenerPosition(unsigned int listenerIndex, float x, float y, float z);
    void setListenerDirection(unsigned int listenerIndex, float x, float y, float z);
    void setListenerWorldUp(unsigned int listenerIndex, float x, float y, float z);

    static std::vector<std::string> getPlaybackDeviceNames();

    bool isOk() const { return m_isInitialized; }

  private:
    mutable ma_engine m_engine;
    bool m_isInitialized = false;
    ma_resource_manager m_resourceManager;
    std::vector<ma_decoding_backend_vtable*> m_decoders;
};

} // namespace Ma

#define g_audioEngine CSingleton<Ma::AudioEngine>::GetInstance()
#endif // MINIAUDIO_WRAPPER_H
