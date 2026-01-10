# Vulkan2D

Vulkan 1.3を使用した2Dゲーム開発用ライブラリです。

## 特徴

- **Vulkan 1.3対応**: 動的レンダリング（Dynamic Rendering）、同期2.0などの最新機能を活用
- **スプライトバッチング**: 効率的な2Dスプライト描画（最大10,000スプライト/バッチ）
- **テクスチャ管理**: PNG/JPG画像のロード、フィルタリング、ラップモード設定
- **フォント描画**: FreeTypeによるTTF/OTFフォントのロードとテキスト描画
- **サウンド再生**: WAV/MP3/OGG/FLACの再生、BGM・効果音、フェード・ピッチ・パン対応
- **2Dカメラ**: パン、ズーム、回転をサポート
- **入力管理**: キーボードとマウスの入力処理
- **変換システム**: 位置、回転、スケール、原点の2D変換

## 必要環境

- CMake 3.20以上
- Vulkan SDK 1.3以上
- C++20対応コンパイラ（Visual Studio 2022、GCC 10以上、Clang 11以上）

## 依存ライブラリ（自動ダウンロード）

- GLFW 3.3.8 - ウィンドウ管理
- GLM 1.0.1 - 数学ライブラリ
- stb_image - 画像読み込み
- FreeType 2.13.2 - フォント描画
- miniaudio 0.11.21 - オーディオ再生

## ビルド方法

```bash
# ビルドディレクトリの作成
mkdir build
cd build

# CMakeの設定（Windows + Visual Studio）
cmake .. -G "Visual Studio 17 2022"

# ビルド
cmake --build . --config Release

# 実行
./Release/Example.exe
```

## 使用例

### 基本的なスプライト描画

```cpp
#include <Vulkan2D/Vulkan2D.h>

int main() {
    // エンジン設定
    V2D::EngineConfig config;
    config.windowConfig.title = "My Game";
    config.windowConfig.width = 1280;
    config.windowConfig.height = 720;
    config.enableValidation = true;  // デバッグ時はtrue

    // エンジン作成
    V2D::Engine engine(config);

    // カメラ作成（画面中央が原点）
    V2D::Camera2D camera(1280.0f, 720.0f);

    // スプライト作成
    V2D::Sprite sprite;
    sprite.SetSize(glm::vec2(100.0f, 100.0f));
    sprite.transform.position = glm::vec2(0.0f, 0.0f);
    sprite.transform.origin = glm::vec2(50.0f, 50.0f);  // 中心を原点に
    sprite.SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤色

    // メインループ
    engine.Run(
        // 更新コールバック
        [&](float deltaTime) {
            // WASDでカメラ移動
            auto* input = engine.GetInput();
            glm::vec2 move(0.0f);
            if (input->IsKeyDown(V2D::Key::W)) move.y += 1.0f;
            if (input->IsKeyDown(V2D::Key::S)) move.y -= 1.0f;
            if (input->IsKeyDown(V2D::Key::A)) move.x -= 1.0f;
            if (input->IsKeyDown(V2D::Key::D)) move.x += 1.0f;
            camera.Move(move * 300.0f * deltaTime);
            
            // スプライトを回転
            sprite.transform.rotation += deltaTime;
        },
        // 描画コールバック
        [&]() {
            auto* batch = engine.GetRenderer()->GetSpriteBatch();
            batch->Begin(&camera);
            batch->Draw(sprite);
            batch->End();
            batch->Render(engine.GetRenderer()->GetCurrentCommandBuffer());
        }
    );

    return 0;
}
```

### テクスチャの使用

```cpp
// テクスチャを読み込み
auto texture = std::make_shared<V2D::Texture>(
    engine.GetVulkanContext(), 
    "image.png"
);

// フィルタリングモードを設定
texture->SetFilter(V2D::TextureFilter::Linear);  // または Nearest
texture->SetWrap(V2D::TextureWrap::Repeat);      // または ClampToEdge, MirroredRepeat

// スプライトにテクスチャを設定
V2D::Sprite sprite(texture);
```

### テキスト描画

```cpp
// フォントを読み込み（サイズ32ピクセル）
V2D::Font font(engine.GetVulkanContext(), "C:/Windows/Fonts/meiryo.ttc", 32);

// テキストレンダラーを作成
V2D::TextRenderer textRenderer(engine.GetVulkanContext());

// 描画コールバック内で
batch->Begin(&camera);

// 基本的なテキスト描画
textRenderer.DrawText(batch, &font, "Hello World!",
                     glm::vec2(0.0f, 0.0f),           // 位置
                     V2D::Color::White(),             // 色（Colorクラス使用）
                     1.0f,                            // スケール
                     V2D::TextAlign::Center);         // 揃え位置

// 影付きテキスト
textRenderer.DrawTextWithShadow(batch, &font, "Title",
                               glm::vec2(0.0f, 100.0f),
                               V2D::Color::Gold(),                    // 文字色
                               V2D::Color::Black().WithAlpha(0.5f),   // 影色
                               glm::vec2(2.0f, 2.0f),              // 影オフセット
                               1.5f, V2D::TextAlign::Center);

batch->End();
```

### 色の指定方法

```cpp
// === Colorクラスを使った色指定 ===

// RGB値で指定（0-255）
sprite.SetColor(V2D::Color::RGB(255, 128, 0));           // オレンジ
sprite.SetColor(V2D::Color::RGBA(255, 0, 0, 128));       // 半透明の赤

// HTMLカラーコードで指定
sprite.SetColor(V2D::Color::Hex("#FF6600"));             // オレンジ
sprite.SetColor(V2D::Color::Hex("#FF0000FF"));           // 不透明の赤（アルファ付き）
sprite.SetColor(V2D::Color::Hex("00FF00"));              // 緑（#省略可）

// HSV値で指定（色相0-360°, 彩度0-1, 明度0-1）
sprite.SetColor(V2D::Color::HSV(120.0f, 1.0f, 1.0f));    // 緑
sprite.SetColor(V2D::Color::HSVA(240.0f, 0.5f, 1.0f, 0.5f)); // 半透明の薄い青

// 定義済みカラー
sprite.SetColor(V2D::Color::Red());
sprite.SetColor(V2D::Color::Green());
sprite.SetColor(V2D::Color::Blue());
sprite.SetColor(V2D::Color::Yellow());
sprite.SetColor(V2D::Color::Cyan());
sprite.SetColor(V2D::Color::Magenta());
sprite.SetColor(V2D::Color::White());
sprite.SetColor(V2D::Color::Black());
sprite.SetColor(V2D::Color::Gray());
sprite.SetColor(V2D::Color::Gold());       // ゲームでよく使う色
sprite.SetColor(V2D::Color::SkyBlue());
sprite.SetColor(V2D::Color::Crimson());
sprite.SetColor(V2D::Color::Transparent()); // 完全透明

// === Spriteの便利メソッド ===

// 直接RGB/RGBA指定（0-255）
sprite.SetColorRGB(255, 128, 0);      // RGB
sprite.SetColorRGBA(255, 0, 0, 128);  // RGBA
sprite.SetColorHex("#FF6600");        // HTMLカラーコード
sprite.SetAlpha(0.5f);                // アルファ値のみ変更

// === 色の操作 ===

V2D::Color color = V2D::Color::Red();
color.WithAlpha(0.5f);         // アルファ値を変更した新しい色
color.Lighten(0.2f);           // 明るくした色
color.Darken(0.2f);            // 暗くした色
color.Lerp(V2D::Color::Blue(), 0.5f);  // 2色の中間色

// glm::vec4との互換性（暗黙変換可能）
glm::vec4 vec = V2D::Color::RGB(255, 0, 0);  // 自動変換
```

### 入力処理

```cpp
V2D::InputManager* input = engine.GetInput();

// キー入力
if (input->IsKeyDown(V2D::Key::Space)) { }      // 押されている間
if (input->IsKeyPressed(V2D::Key::Enter)) { }   // 押した瞬間
if (input->IsKeyReleased(V2D::Key::Escape)) { } // 離した瞬間

// マウス入力
if (input->IsMouseButtonDown(V2D::MouseButton::Left)) { }
glm::vec2 mousePos = input->GetMousePosition();
glm::vec2 mouseDelta = input->GetMouseDelta();
float scroll = input->GetScrollDelta();
```

### サウンド再生

```cpp
V2D::AudioManager* audio = engine.GetAudio();

// === サウンドのロード ===

// 効果音（メモリに全データをロード）
V2D::Sound sfx("click.wav");

// BGM（ストリーミングモード、メモリ節約）
V2D::Sound bgm("music.mp3", true);  // 第2引数=trueでストリーミング

// === 再生 ===

// 効果音を再生
audio->Play(&sfx, 0.8f);            // 音量80%
audio->Play(&sfx, 1.0f, true);      // ループ再生

// BGMを再生（自動ループ）
audio->PlayBGM(&bgm, 0.5f);         // 音量50%
audio->PlayBGM(&bgm, 0.5f, 2.0f);   // 2秒フェードイン

// 詳細なオプション付き再生
V2D::PlayOptions options;
options.volume = 0.7f;    // 音量
options.pitch = 1.2f;     // ピッチ（1.0が通常）
options.pan = -0.5f;      // パン（-1.0=左, 0=中央, 1.0=右）
options.loop = false;     // ループ
options.fadeIn = true;    // フェードイン
options.fadeInTime = 1.0f; // フェードイン時間（秒）

V2D::SoundInstance inst = audio->Play(&sfx, options);

// === 再生中の制御 ===

audio->Pause(inst);                 // 一時停止
audio->Resume(inst);                // 再開
audio->Stop(inst);                  // 停止
audio->Stop(inst, 0.5f);            // 0.5秒フェードアウトして停止

audio->SetVolume(inst, 0.5f);       // 音量変更
audio->SetPitch(inst, 1.5f);        // ピッチ変更
audio->SetPan(inst, 0.8f);          // パン変更

// === グローバル制御 ===

audio->SetMasterVolume(0.8f);       // マスターボリューム
audio->SetSFXVolume(1.0f);          // 効果音ボリューム
audio->SetBGMVolume(0.6f);          // BGMボリューム

audio->StopBGM(1.0f);               // BGMを1秒フェードアウト
audio->StopAll();                   // 全サウンド停止
audio->PauseAll();                  // 全サウンド一時停止
audio->ResumeAll();                 // 全サウンド再開

// === 状態確認 ===

if (audio->IsPlaying(inst)) { }     // 再生中か
bool paused = audio->IsPaused(inst); // 一時停止中か
bool bgmPlaying = audio->IsBGMPlaying(); // BGMが再生中か
```

## プロジェクト構造

```
Vulkan2D/
├── CMakeLists.txt
├── README.md
├── include/
│   └── Vulkan2D/
│       ├── Vulkan2D.h          # メインヘッダー（すべてをインクルード）
│       ├── Core/
│       │   ├── Engine.h        # エンジンクラス
│       │   └── Window.h        # ウィンドウ管理
│       ├── Renderer/
│       │   ├── VulkanContext.h # Vulkanコンテキスト
│       │   ├── Renderer2D.h    # 2Dレンダラー
│       │   ├── Pipeline.h      # グラフィックスパイプライン
│       │   ├── Shader.h        # シェーダー管理
│       │   ├── Buffer.h        # バッファ管理
│       │   ├── Texture.h       # テクスチャ管理
│       │   └── SpriteBatch.h   # スプライトバッチ
│       ├── Graphics/
│       │   ├── Sprite.h        # スプライトクラス
│       │   ├── Camera2D.h      # 2Dカメラ
│       │   ├── Font.h          # フォント管理
│       │   └── TextRenderer.h  # テキスト描画
│       ├── Math/
│       │   └── Transform2D.h   # 2D変換
        ├── Input/
        │   └── InputManager.h  # 入力管理
        └── Audio/
            ├── Sound.h         # サウンドクラス
            └── AudioManager.h  # オーディオ管理
├── src/                        # 実装ファイル
├── shaders/                    # GLSLシェーダー
│   ├── sprite.vert             # 頂点シェーダー
│   └── sprite.frag             # フラグメントシェーダー
└── examples/                   # サンプルコード
    └── main.cpp
```

## API概要

### Engine
メインのエンジンクラス。ウィンドウ、Vulkanコンテキスト、レンダラー、入力を統合管理します。

| メソッド | 説明 |
|---------|------|
| `Run(onUpdate, onRender)` | メインループを実行 |
| `Stop()` | エンジンを停止 |
| `GetWindow()` | ウィンドウを取得 |
| `GetVulkanContext()` | Vulkanコンテキストを取得 |
| `GetRenderer()` | 2Dレンダラーを取得 |
| `GetInput()` | 入力マネージャーを取得 |
| `GetAudio()` | オーディオマネージャーを取得 |
| `GetFPS()` | 現在のFPSを取得 |
| `GetDeltaTime()` | デルタタイムを取得 |

### Camera2D
2Dカメラ。正射影投影を使用し、パン、ズーム、回転をサポートします。

| メソッド | 説明 |
|---------|------|
| `SetPosition(pos)` | カメラ位置を設定 |
| `Move(delta)` | カメラを相対移動 |
| `SetZoom(zoom)` | ズーム倍率を設定 |
| `Zoom(delta)` | ズームを相対変更 |
| `SetRotation(rad)` | 回転を設定（ラジアン） |
| `ScreenToWorld(pos)` | スクリーン座標→ワールド座標 |
| `WorldToScreen(pos)` | ワールド座標→スクリーン座標 |

### SpriteBatch
効率的なスプライト描画のためのバッチレンダリングシステム。

| メソッド | 説明 |
|---------|------|
| `Begin(camera)` | バッチを開始 |
| `Draw(sprite)` | スプライトを描画キューに追加 |
| `Draw(texture, pos, size, color)` | 直接描画 |
| `End()` | バッチを終了 |
| `Render(cmdBuffer)` | バッチをレンダリング |

### Sprite
2Dスプライトを表すクラス。

| プロパティ/メソッド | 説明 |
|-------------------|------|
| `transform.position` | 位置 |
| `transform.rotation` | 回転（ラジアン） |
| `transform.scale` | スケール |
| `transform.origin` | 回転の原点 |
| `SetTexture(tex)` | テクスチャを設定 |
| `SetColor(color)` | 色を設定（glm::vec4またはColor） |
| `SetColorRGB(r, g, b)` | RGB値で色を設定（0-255） |
| `SetColorRGBA(r, g, b, a)` | RGBA値で色を設定（0-255） |
| `SetColorHex(hex)` | HTMLカラーコードで色を設定 |
| `SetAlpha(alpha)` | アルファ値のみを設定 |
| `SetSize(size)` | サイズを設定 |
| `SetSourceRect(rect)` | ソース矩形を設定（スプライトシート用） |

### Color
色のユーティリティクラス。RGB(0-255)やHTMLカラーコードなど様々な形式で色を指定できます。

| メソッド | 説明 |
|---------|------|
| `Color::RGB(r, g, b)` | RGB値から作成（0-255） |
| `Color::RGBA(r, g, b, a)` | RGBA値から作成（0-255） |
| `Color::RGBAlpha(r, g, b, alpha)` | RGB + 浮動小数点アルファ |
| `Color::Hex(hex)` | HTMLカラーコードから作成 |
| `Color::HSV(h, s, v)` | HSV値から作成 |
| `Color::HSVA(h, s, v, a)` | HSVA値から作成 |
| `WithAlpha(alpha)` | アルファ値を変更した色を返す |
| `Lighten(amount)` | 明るくした色を返す |
| `Darken(amount)` | 暗くした色を返す |
| `Lerp(other, t)` | 2色を混合した色を返す |
| `ToVec4()` | glm::vec4に変換 |
| `ToHex()` | HTMLカラーコードに変換 |

**定義済みカラー**: `White()`, `Black()`, `Red()`, `Green()`, `Blue()`, `Yellow()`, `Cyan()`, `Magenta()`, `Orange()`, `Purple()`, `Pink()`, `Gray()`, `Gold()`, `Silver()`, `SkyBlue()`, `Crimson()`, `Navy()`, `Transparent()`

### Font
FreeTypeを使用したフォント管理。

| メソッド | 説明 |
|---------|------|
| `Font(context, path, size)` | TTF/OTFフォントを読み込み |
| `MeasureText(text)` | テキストのサイズを計測 |
| `GetFontSize()` | フォントサイズを取得 |

### TextRenderer
テキスト描画クラス。

| メソッド | 説明 |
|---------|------|
| `DrawText(batch, font, text, pos, color, scale, align)` | テキストを描画 |
| `DrawTextWithShadow(...)` | 影付きテキストを描画 |

### InputManager
キーボードとマウスの入力を処理します。

| メソッド | 説明 |
|---------|------|
| `IsKeyDown(key)` | キーが押されているか |
| `IsKeyPressed(key)` | キーが今フレームで押されたか |
| `IsKeyReleased(key)` | キーが今フレームで離されたか |
| `IsMouseButtonDown(btn)` | マウスボタンが押されているか |
| `GetMousePosition()` | マウス位置を取得 |
| `GetMouseDelta()` | マウス移動量を取得 |
| `GetScrollDelta()` | スクロール量を取得 |

### Sound
音声ファイルを管理するクラス。WAV/MP3/OGG/FLACに対応。

| メソッド | 説明 |
|---------|------|
| `Sound(path)` | 音声ファイルをロード |
| `Sound(path, streaming)` | ストリーミングモード指定でロード |
| `IsLoaded()` | ロード成功したか |
| `GetDuration()` | サウンドの長さ（秒） |
| `IsStreaming()` | ストリーミングモードか |

### AudioManager
サウンドの再生・停止・音量調整などを管理するクラス。

| メソッド | 説明 |
|---------|------|
| `Play(sound, volume, loop)` | サウンドを再生 |
| `Play(sound, options)` | オプション付きで再生 |
| `PlayBGM(sound, volume, fadeIn)` | BGMを再生（自動ループ） |
| `Stop(instance, fadeOut)` | サウンドを停止 |
| `Pause(instance)` | サウンドを一時停止 |
| `Resume(instance)` | サウンドを再開 |
| `StopAll(fadeOut)` | 全サウンドを停止 |
| `StopBGM(fadeOut)` | BGMを停止 |
| `SetVolume(instance, volume)` | 音量を設定 |
| `SetPitch(instance, pitch)` | ピッチを設定 |
| `SetPan(instance, pan)` | パンを設定 |
| `SetMasterVolume(volume)` | マスターボリュームを設定 |
| `SetSFXVolume(volume)` | 効果音ボリュームを設定 |
| `SetBGMVolume(volume)` | BGMボリュームを設定 |
| `IsPlaying(instance)` | 再生中か確認 |
| `IsBGMPlaying()` | BGMが再生中か |

### PlayOptions
サウンド再生の詳細オプション。

| プロパティ | 説明 |
|------------|------|
| `volume` | 音量（0.0-1.0） |
| `pitch` | ピッチ（1.0=通常、高いと速い） |
| `pan` | パン（-1.0=左、0.0=中央、1.0=右） |
| `loop` | ループ再生 |
| `fadeIn` | フェードイン有効化 |
| `fadeInTime` | フェードイン時間（秒） |

## 座標系

- **ワールド座標**: Y+が上、X+が右
- **画面中央が原点** (0, 0)
- **左上**: (-width/2, +height/2)
- **右下**: (+width/2, -height/2)

## ライセンス

このプロジェクトは [MIT License](LICENSE) の下で公開されています。

### サードパーティライブラリ

このプロジェクトは以下のオープンソースライブラリを使用しています：

| ライブラリ | ライセンス | 用途 |
|-----------|-----------|------|
| [GLFW](https://www.glfw.org/) | Zlib License | ウィンドウ管理 |
| [GLM](https://github.com/g-truc/glm) | MIT License | 数学ライブラリ |
| [stb_image](https://github.com/nothings/stb) | Public Domain / MIT License | 画像読み込み |
| [FreeType](https://freetype.org/) | FreeType License (BSD-style) | フォント描画 |
| [miniaudio](https://miniaud.io/) | Public Domain / MIT License | オーディオ再生 |
