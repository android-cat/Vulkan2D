# Vulkan2D アーキテクチャ設計書

## 概要
Vulkan2Dは、Vulkan APIをバックエンドに持つ2Dゲームおよびグラフィックスエンジンです。主要なモジュールは以下の通り構成されています。

## モジュール構成
1. **Core**
   - `Engine`: エンジンの初期化、メインループの制御、各サブシステムのライフサイクル管理を行います。
   - `Window`: GLFWを用いたウィンドウ生成とイベント（入力・リサイズ等）の管理を行います。

2. **Renderer**
   - `VulkanContext`: Vulkanインスタンス、論理デバイス、スワップチェーンの管理など、Vulkan特有の低レベルAPIをカプセル化します。
   - `Renderer2D` / `SpriteBatch`: 2D描画コマンドの効率的なバッチ処理とレンダリングパイプラインを統合管理します。

3. **Graphics**
   - `Camera2D`, `Sprite`, `Font`, `TextRenderer`: エンジン利用者が実際に扱う、レンダリングされる具体的な要素の抽象化レイヤーです。

4. **Math**
   - `Transform2D`: 位置、回転、スケールなどの2D空間計算（GLMベース）を提供します。

5. **Input & Audio**
   - `InputManager`: キーボードやマウス入力のステート管理を提供します。
   - `AudioManager`, `Sound`: miniaudioを用いた音声資源のロードと再生機構を提供します。
