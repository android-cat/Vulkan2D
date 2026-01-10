/**
 * @file AudioManager.h
 * @brief オーディオ管理クラス
 * 
 * サウンドの再生、停止、音量調整などを統括するクラス。
 * miniaudioライブラリをバックエンドとして使用。
 */

#pragma once

#include "Vulkan2D/Audio/Sound.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace V2D {

/**
 * @brief オーディオシステムの設定
 */
struct AudioConfig {
    uint32_t sampleRate = 44100;    ///< サンプルレート（Hz）
    uint32_t channels = 2;          ///< チャンネル数（1=モノラル, 2=ステレオ）
    float masterVolume = 1.0f;      ///< マスターボリューム（0.0-1.0）
    float sfxVolume = 1.0f;         ///< 効果音ボリューム（0.0-1.0）
    float bgmVolume = 1.0f;         ///< BGMボリューム（0.0-1.0）
};

/**
 * @brief 再生オプション
 */
struct PlayOptions {
    float volume = 1.0f;            ///< 再生音量（0.0-1.0）
    float pitch = 1.0f;             ///< ピッチ（1.0が通常、2.0で2倍速）
    float pan = 0.0f;               ///< パン（-1.0=左, 0.0=中央, 1.0=右）
    bool loop = false;              ///< ループ再生
    bool fadeIn = false;            ///< フェードイン
    float fadeInTime = 0.0f;        ///< フェードイン時間（秒）
};

/**
 * @brief オーディオ管理クラス
 * 
 * サウンドの再生・停止・音量調整などを行う。
 * スレッドセーフな設計。
 */
class AudioManager {
public:
    /**
     * @brief コンストラクタ
     * @param config オーディオ設定
     */
    explicit AudioManager(const AudioConfig& config = AudioConfig{});

    /**
     * @brief デストラクタ
     */
    ~AudioManager();

    // コピー禁止
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // ========================================
    // サウンド再生
    // ========================================

    /**
     * @brief サウンドを再生
     * @param sound 再生するサウンド
     * @param options 再生オプション
     * @return サウンドインスタンス（再生の制御に使用）
     */
    SoundInstance Play(Sound* sound, const PlayOptions& options = PlayOptions{});

    /**
     * @brief サウンドを再生（簡易版）
     * @param sound 再生するサウンド
     * @param volume 音量（0.0-1.0）
     * @param loop ループ再生するか
     * @return サウンドインスタンス
     */
    SoundInstance Play(Sound* sound, float volume, bool loop = false);

    /**
     * @brief BGMを再生（現在のBGMを停止して新しいBGMを再生）
     * @param sound 再生するサウンド
     * @param volume 音量（0.0-1.0）
     * @param fadeInTime フェードイン時間（秒、0で即座に再生）
     * @return サウンドインスタンス
     */
    SoundInstance PlayBGM(Sound* sound, float volume = 1.0f, float fadeInTime = 0.0f);

    // ========================================
    // サウンド制御
    // ========================================

    /**
     * @brief サウンドを停止
     * @param instance 停止するサウンドインスタンス
     * @param fadeOutTime フェードアウト時間（秒、0で即座に停止）
     */
    void Stop(SoundInstance instance, float fadeOutTime = 0.0f);

    /**
     * @brief サウンドを一時停止
     * @param instance 一時停止するサウンドインスタンス
     */
    void Pause(SoundInstance instance);

    /**
     * @brief サウンドを再開
     * @param instance 再開するサウンドインスタンス
     */
    void Resume(SoundInstance instance);

    /**
     * @brief すべてのサウンドを停止
     * @param fadeOutTime フェードアウト時間（秒）
     */
    void StopAll(float fadeOutTime = 0.0f);

    /**
     * @brief BGMを停止
     * @param fadeOutTime フェードアウト時間（秒）
     */
    void StopBGM(float fadeOutTime = 0.0f);

    /**
     * @brief すべてのサウンドを一時停止
     */
    void PauseAll();

    /**
     * @brief すべてのサウンドを再開
     */
    void ResumeAll();

    // ========================================
    // 音量制御
    // ========================================

    /**
     * @brief インスタンスの音量を設定
     * @param instance サウンドインスタンス
     * @param volume 音量（0.0-1.0）
     */
    void SetVolume(SoundInstance instance, float volume);

    /**
     * @brief マスターボリュームを設定
     * @param volume 音量（0.0-1.0）
     */
    void SetMasterVolume(float volume);

    /**
     * @brief 効果音ボリュームを設定
     * @param volume 音量（0.0-1.0）
     */
    void SetSFXVolume(float volume);

    /**
     * @brief BGMボリュームを設定
     * @param volume 音量（0.0-1.0）
     */
    void SetBGMVolume(float volume);

    /**
     * @brief マスターボリュームを取得
     * @return 音量（0.0-1.0）
     */
    float GetMasterVolume() const { return m_MasterVolume; }

    /**
     * @brief 効果音ボリュームを取得
     * @return 音量（0.0-1.0）
     */
    float GetSFXVolume() const { return m_SFXVolume; }

    /**
     * @brief BGMボリュームを取得
     * @return 音量（0.0-1.0）
     */
    float GetBGMVolume() const { return m_BGMVolume; }

    // ========================================
    // ピッチ・パン制御
    // ========================================

    /**
     * @brief インスタンスのピッチを設定
     * @param instance サウンドインスタンス
     * @param pitch ピッチ（1.0が通常速度）
     */
    void SetPitch(SoundInstance instance, float pitch);

    /**
     * @brief インスタンスのパンを設定
     * @param instance サウンドインスタンス
     * @param pan パン（-1.0=左, 0.0=中央, 1.0=右）
     */
    void SetPan(SoundInstance instance, float pan);

    // ========================================
    // 状態確認
    // ========================================

    /**
     * @brief サウンドが再生中か確認
     * @param instance サウンドインスタンス
     * @return 再生中の場合true
     */
    bool IsPlaying(SoundInstance instance) const;

    /**
     * @brief サウンドが一時停止中か確認
     * @param instance サウンドインスタンス
     * @return 一時停止中の場合true
     */
    bool IsPaused(SoundInstance instance) const;

    /**
     * @brief BGMが再生中か確認
     * @return 再生中の場合true
     */
    bool IsBGMPlaying() const;

    /**
     * @brief 更新処理（フェードなどの処理）
     * @param deltaTime フレーム時間（秒）
     */
    void Update(float deltaTime);

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;

    float m_MasterVolume = 1.0f;
    float m_SFXVolume = 1.0f;
    float m_BGMVolume = 1.0f;

    SoundInstance m_CurrentBGM;

    mutable std::mutex m_Mutex;
};

} // namespace V2D
