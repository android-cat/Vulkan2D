/**
 * @file AudioManager.cpp
 * @brief オーディオマネージャーの実装
 * 
 * miniaudioを使用したサウンドの再生・管理
 */

#include "Vulkan2D/Audio/AudioManager.h"
#include "miniaudio.h"

#include <unordered_map>
#include <atomic>
#include <cmath>
#include <algorithm>

namespace V2D {

/**
 * @brief 再生中のサウンド情報
 */
struct PlayingSound {
    Sound* sound = nullptr;
    uint64_t id = 0;
    
    float volume = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;
    bool loop = false;
    bool paused = false;
    bool isBGM = false;
    
    // フェード制御
    float fadeVolume = 1.0f;
    float fadeTarget = 1.0f;
    float fadeSpeed = 0.0f;
    bool fadeOutStop = false;
    
    // 再生位置
    size_t currentFrame = 0;
    
    bool IsActive() const { return sound != nullptr; }
};

/**
 * @brief AudioManagerの内部実装
 */
struct AudioManager::Impl {
    ma_engine engine;
    bool engineInitialized = false;
    
    // 再生中のサウンド
    std::unordered_map<uint64_t, PlayingSound> playingSounds;
    std::atomic<uint64_t> nextInstanceId{1};
    
    // デバイス情報
    ma_device device;
    bool deviceInitialized = false;
    
    AudioConfig config;
    
    // オーディオコールバック用
    static void DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    void ProcessAudio(float* pOutput, ma_uint32 frameCount);
};

/**
 * @brief オーディオデータコールバック
 */
void AudioManager::Impl::DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    auto* impl = static_cast<Impl*>(pDevice->pUserData);
    if (impl) {
        impl->ProcessAudio(static_cast<float*>(pOutput), frameCount);
    }
    (void)pInput; // 入力は使用しない
}

/**
 * @brief オーディオ処理
 */
void AudioManager::Impl::ProcessAudio(float* pOutput, ma_uint32 frameCount) {
    // 出力バッファをゼロクリア
    std::memset(pOutput, 0, frameCount * config.channels * sizeof(float));
    
    // 各サウンドをミックス
    for (auto& [id, playing] : playingSounds) {
        if (!playing.IsActive() || playing.paused) {
            continue;
        }
        
        auto* soundData = playing.sound->GetData();
        if (!soundData || soundData->pcmData.empty()) {
            continue;
        }
        
        // 実効音量を計算
        float effectiveVolume = playing.volume * playing.fadeVolume;
        if (playing.isBGM) {
            // BGMのボリューム処理は外部で行う
        }
        
        // オーディオデータをミックス
        for (ma_uint32 frame = 0; frame < frameCount; ++frame) {
            if (playing.currentFrame >= soundData->totalFrames) {
                if (playing.loop) {
                    playing.currentFrame = 0;
                } else {
                    playing.sound = nullptr; // 再生終了
                    break;
                }
            }
            
            // ピッチ処理（簡易実装）
            size_t srcFrame = static_cast<size_t>(playing.currentFrame);
            if (srcFrame < soundData->totalFrames) {
                for (ma_uint32 ch = 0; ch < config.channels && ch < soundData->channels; ++ch) {
                    size_t srcIndex = srcFrame * soundData->channels + ch;
                    size_t dstIndex = frame * config.channels + ch;
                    
                    if (srcIndex < soundData->pcmData.size()) {
                        float sample = soundData->pcmData[srcIndex] * effectiveVolume;
                        
                        // パン処理（ステレオの場合）
                        if (config.channels == 2) {
                            float panL = (playing.pan <= 0.0f) ? 1.0f : (1.0f - playing.pan);
                            float panR = (playing.pan >= 0.0f) ? 1.0f : (1.0f + playing.pan);
                            sample *= (ch == 0) ? panL : panR;
                        }
                        
                        pOutput[dstIndex] += sample;
                    }
                }
            }
            
            playing.currentFrame += static_cast<size_t>(playing.pitch);
        }
    }
    
    // クリッピング防止
    for (ma_uint32 i = 0; i < frameCount * config.channels; ++i) {
        pOutput[i] = std::clamp(pOutput[i], -1.0f, 1.0f);
    }
}

AudioManager::AudioManager(const AudioConfig& config)
    : m_Impl(std::make_unique<Impl>())
    , m_MasterVolume(config.masterVolume)
    , m_SFXVolume(config.sfxVolume)
    , m_BGMVolume(config.bgmVolume)
{
    m_Impl->config = config;
    
    // デバイス設定
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = config.channels;
    deviceConfig.sampleRate = config.sampleRate;
    deviceConfig.dataCallback = Impl::DataCallback;
    deviceConfig.pUserData = m_Impl.get();
    
    // デバイスを初期化
    ma_result result = ma_device_init(nullptr, &deviceConfig, &m_Impl->device);
    if (result != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize audio device");
    }
    m_Impl->deviceInitialized = true;
    
    // デバイスを開始
    result = ma_device_start(&m_Impl->device);
    if (result != MA_SUCCESS) {
        ma_device_uninit(&m_Impl->device);
        m_Impl->deviceInitialized = false;
        throw std::runtime_error("Failed to start audio device");
    }
}

AudioManager::~AudioManager() {
    if (m_Impl && m_Impl->deviceInitialized) {
        ma_device_uninit(&m_Impl->device);
    }
}

SoundInstance AudioManager::Play(Sound* sound, const PlayOptions& options) {
    if (!sound || !sound->IsLoaded()) {
        return SoundInstance{};
    }
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    uint64_t id = m_Impl->nextInstanceId++;
    
    PlayingSound playing;
    playing.sound = sound;
    playing.id = id;
    playing.volume = options.volume * m_MasterVolume * m_SFXVolume;
    playing.pitch = options.pitch;
    playing.pan = options.pan;
    playing.loop = options.loop;
    playing.currentFrame = 0;
    
    // フェードイン設定
    if (options.fadeIn && options.fadeInTime > 0.0f) {
        playing.fadeVolume = 0.0f;
        playing.fadeTarget = 1.0f;
        playing.fadeSpeed = 1.0f / options.fadeInTime;
    }
    
    m_Impl->playingSounds[id] = playing;
    
    return SoundInstance{id};
}

SoundInstance AudioManager::Play(Sound* sound, float volume, bool loop) {
    PlayOptions options;
    options.volume = volume;
    options.loop = loop;
    return Play(sound, options);
}

SoundInstance AudioManager::PlayBGM(Sound* sound, float volume, float fadeInTime) {
    // 現在のBGMを停止
    StopBGM(0.5f);
    
    if (!sound || !sound->IsLoaded()) {
        return SoundInstance{};
    }
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    uint64_t id = m_Impl->nextInstanceId++;
    
    PlayingSound playing;
    playing.sound = sound;
    playing.id = id;
    playing.volume = volume * m_MasterVolume * m_BGMVolume;
    playing.pitch = 1.0f;
    playing.pan = 0.0f;
    playing.loop = true;
    playing.isBGM = true;
    playing.currentFrame = 0;
    
    // フェードイン設定
    if (fadeInTime > 0.0f) {
        playing.fadeVolume = 0.0f;
        playing.fadeTarget = 1.0f;
        playing.fadeSpeed = 1.0f / fadeInTime;
    }
    
    m_Impl->playingSounds[id] = playing;
    m_CurrentBGM = SoundInstance{id};
    
    return m_CurrentBGM;
}

void AudioManager::Stop(SoundInstance instance, float fadeOutTime) {
    if (!instance.IsValid()) return;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Impl->playingSounds.find(instance.GetId());
    if (it != m_Impl->playingSounds.end()) {
        if (fadeOutTime > 0.0f) {
            it->second.fadeTarget = 0.0f;
            it->second.fadeSpeed = 1.0f / fadeOutTime;
            it->second.fadeOutStop = true;
        } else {
            m_Impl->playingSounds.erase(it);
        }
    }
}

void AudioManager::Pause(SoundInstance instance) {
    if (!instance.IsValid()) return;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Impl->playingSounds.find(instance.GetId());
    if (it != m_Impl->playingSounds.end()) {
        it->second.paused = true;
    }
}

void AudioManager::Resume(SoundInstance instance) {
    if (!instance.IsValid()) return;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Impl->playingSounds.find(instance.GetId());
    if (it != m_Impl->playingSounds.end()) {
        it->second.paused = false;
    }
}

void AudioManager::StopAll(float fadeOutTime) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (fadeOutTime > 0.0f) {
        for (auto& [id, playing] : m_Impl->playingSounds) {
            playing.fadeTarget = 0.0f;
            playing.fadeSpeed = 1.0f / fadeOutTime;
            playing.fadeOutStop = true;
        }
    } else {
        m_Impl->playingSounds.clear();
    }
    
    m_CurrentBGM = SoundInstance{};
}

void AudioManager::StopBGM(float fadeOutTime) {
    if (!m_CurrentBGM.IsValid()) return;
    
    Stop(m_CurrentBGM, fadeOutTime);
    m_CurrentBGM = SoundInstance{};
}

void AudioManager::PauseAll() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    for (auto& [id, playing] : m_Impl->playingSounds) {
        playing.paused = true;
    }
}

void AudioManager::ResumeAll() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    for (auto& [id, playing] : m_Impl->playingSounds) {
        playing.paused = false;
    }
}

void AudioManager::SetVolume(SoundInstance instance, float volume) {
    if (!instance.IsValid()) return;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Impl->playingSounds.find(instance.GetId());
    if (it != m_Impl->playingSounds.end()) {
        float baseVolume = it->second.isBGM ? m_BGMVolume : m_SFXVolume;
        it->second.volume = volume * m_MasterVolume * baseVolume;
    }
}

void AudioManager::SetMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_MasterVolume = std::clamp(volume, 0.0f, 1.0f);
    
    // 再生中のサウンドの音量を更新
    for (auto& [id, playing] : m_Impl->playingSounds) {
        float baseVolume = playing.isBGM ? m_BGMVolume : m_SFXVolume;
        // 元のユーザー指定音量を保持するため、ここでは更新しない
        // Update時に再計算される
    }
}

void AudioManager::SetSFXVolume(float volume) {
    m_SFXVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::SetBGMVolume(float volume) {
    m_BGMVolume = std::clamp(volume, 0.0f, 1.0f);
}

void AudioManager::SetPitch(SoundInstance instance, float pitch) {
    if (!instance.IsValid()) return;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Impl->playingSounds.find(instance.GetId());
    if (it != m_Impl->playingSounds.end()) {
        it->second.pitch = std::clamp(pitch, 0.1f, 4.0f);
    }
}

void AudioManager::SetPan(SoundInstance instance, float pan) {
    if (!instance.IsValid()) return;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Impl->playingSounds.find(instance.GetId());
    if (it != m_Impl->playingSounds.end()) {
        it->second.pan = std::clamp(pan, -1.0f, 1.0f);
    }
}

bool AudioManager::IsPlaying(SoundInstance instance) const {
    if (!instance.IsValid()) return false;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Impl->playingSounds.find(instance.GetId());
    return it != m_Impl->playingSounds.end() && it->second.IsActive() && !it->second.paused;
}

bool AudioManager::IsPaused(SoundInstance instance) const {
    if (!instance.IsValid()) return false;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Impl->playingSounds.find(instance.GetId());
    return it != m_Impl->playingSounds.end() && it->second.paused;
}

bool AudioManager::IsBGMPlaying() const {
    return m_CurrentBGM.IsValid() && IsPlaying(m_CurrentBGM);
}

void AudioManager::Update(float deltaTime) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    // フェード処理と終了したサウンドの削除
    std::vector<uint64_t> toRemove;
    
    for (auto& [id, playing] : m_Impl->playingSounds) {
        // 終了したサウンドをマーク
        if (!playing.IsActive()) {
            toRemove.push_back(id);
            continue;
        }
        
        // フェード処理
        if (playing.fadeSpeed > 0.0f) {
            if (playing.fadeVolume < playing.fadeTarget) {
                playing.fadeVolume += playing.fadeSpeed * deltaTime;
                if (playing.fadeVolume >= playing.fadeTarget) {
                    playing.fadeVolume = playing.fadeTarget;
                    playing.fadeSpeed = 0.0f;
                }
            } else if (playing.fadeVolume > playing.fadeTarget) {
                playing.fadeVolume -= playing.fadeSpeed * deltaTime;
                if (playing.fadeVolume <= playing.fadeTarget) {
                    playing.fadeVolume = playing.fadeTarget;
                    playing.fadeSpeed = 0.0f;
                    
                    // フェードアウト完了で停止
                    if (playing.fadeOutStop && playing.fadeTarget == 0.0f) {
                        toRemove.push_back(id);
                    }
                }
            }
        }
    }
    
    // 終了したサウンドを削除
    for (uint64_t id : toRemove) {
        m_Impl->playingSounds.erase(id);
    }
}

} // namespace V2D
