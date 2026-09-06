#pragma once

#include <string>

namespace engine::audio {

// Simple audio manager stub for volume control
// This is a placeholder until a real audio system is implemented
class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager() = default;

    // Volume control (0.0 to 1.0)
    void setMasterVolume(float volume) { masterVolume_ = volume; }
    float getMasterVolume() const { return masterVolume_; }

    void setMusicVolume(float volume) { musicVolume_ = volume; }
    float getMusicVolume() const { return musicVolume_; }

    void setSFXVolume(float volume) { sfxVolume_ = volume; }
    float getSFXVolume() const { return sfxVolume_; }

    // Check if audio system is initialized
    bool isInitialized() const { return initialized_; }
    void setInitialized(bool initialized) { initialized_ = initialized; }

private:
    float masterVolume_ = 1.0f;
    float musicVolume_ = 0.7f;
    float sfxVolume_ = 0.8f;
    bool initialized_ = false;
};

} // namespace engine::audio
