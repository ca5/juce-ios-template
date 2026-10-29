# JUCE iOS App Template

JUCE 8 + React Web UI + CMake による、**iOS Standalone アプリ** および **AUv3 (Audio Unit v3) プラグイン** を超高速に量産・TestFlight 自動配信するためのテンプレートリポジトリです。

---

## 🌟 特徴

- **クロスプラットフォーム DSP & Native UI**:
  - JUCE 8 + CMake による最新設計。
  - iOS Standalone アプリ + AUv3 AppExtension（GarageBand, AUM, Logic Pro for iPad等で動作）を同時生成。
  - macOS デスクトップ環境でもそのまま即時実行・デバッグ可能。
- **AUv3 プラグインホスティング機能内蔵**:
  - Standalone アプリからサードパーティ製および Apple 標準 AUv3 プラグインをスキャン・ロード・UI 表示・エフェクト処理可能。
  - `inter-app-audio` エンタイトルメント、非同期プラグイン検知通知 (`AVAudioUnitComponentManagerRegistrationsChangedNotification`)、安全なバスレイアウト設定、クラッシュ防止の安全アンロードを最初から解決済み。
- **Modern Single-File Web UI**:
  - React 19 + TypeScript + Vite (`vite-plugin-singlefile`) による軽量・美麗な単一 HTML バンドル。
  - C++ Native ⇄ Web UI の双方向通信ブリッジ内蔵（パラメータ同期・イベント発行）。
- **完全自動化された TestFlight CI/CD**:
  - Fastlane による Apple Distribution 証明書 & Provisioning Profile の自動取得・更新。
  - Standalone と AUv3 の両ターゲットに対する手動署名（Manual Code Signing）パッチ。
  - ビルド番号（`CFBundleVersion`）の自動インクリメント。
  - GitHub Actions からのワンクリックまたはタグ push による TestFlight 自動配信。
- **フィードバック警告バー抑制済みのカスタム Standalone 実装**:
  - JUCE 標準の Standalone アプリで画面上部に出てしまう `"Audio input is muted to avoid feedback loop"` 赤色警告バーを完全非表示化し、クリーンなフルスクリーン表示を実現 (`Source/StandaloneApp.cpp`)。
- **1コマンド量産化スクリプト**:
  - `./scripts/rename_project.sh` を叩くだけで、プロジェクト名・Bundle ID・Team ID を一括置換。
- **Google Jules / AI Agent ガイド (`AGENTS.md`)**:
  - AI エージェントが自律的に機能追加やテスト・検証を実行できる標準ガイドライン完備。

---

## 🚀 新しいアプリの作成手順（量産クイックスタート）

### 1. テンプレートから新規リポジトリを作成
GitHub 上でこのリポジトリをテンプレートとして使用するか、クローンします。

```bash
git clone --recurse-submodules <REPO_URL> MyNewApp
cd MyNewApp
```

### 2. プロジェクト名・Bundle ID の一括置換
付属のスクリプトを実行して、アプリ名・バンドルID・Team ID を一括更新します。

```bash
./scripts/rename_project.sh MySynth me.ca5.mysynth MX8BPUQ84Q Ca5x MySy
```

### 3. アプリアイコンの配置
`Assets/icon.png` をご自身の **1024×1024 RGB PNG（アルファチャンネル無し）** に差し替えます。

### 4. UI 開発 & ローカルテスト
```bash
# 依存パッケージのインストール
npm --prefix ui install

# UI バンドル生成 & 自動検証テスト
npm --prefix ui run build
npm --prefix ui test
```

### 5. TestFlight 自動配信
リポジトリの **Settings > Secrets and variables > Actions** に以下3つの App Store Connect API Key シークレットを設定します：
- `APP_STORE_CONNECT_API_KEY_ID`
- `APP_STORE_CONNECT_ISSUER_ID`
- `APP_STORE_CONNECT_API_KEY_CONTENT`

設定後、GitHub Actions タブから **`Build and Deploy iOS to TestFlight`** を手動実行するか、`v1.0.0` のようなタグを push すれば自動的に TestFlight へ配信されます。

---

## 🛠️ プロジェクト構造

```
.
├── CMakeLists.txt           # JUCE & iOS ビルドマスター設定
├── Source/                  # C++ DSP & WebView ホスト実装
│   ├── PluginProcessor.h / .cpp
│   └── PluginEditor.h / .cpp
├── ui/                      # React 19 + TypeScript + Vite
│   ├── src/
│   │   ├── bridge.ts        # JUCE ⇄ Web UI 双方向バインディング
│   │   └── App.tsx          # メイン UI
│   └── tests/               # 自動検証スイート
├── Assets/                  # 1024x1024 アプリアイコン
├── fastlane/                # TestFlight 自動アップロード定義
├── .github/workflows/       # GitHub Actions (CI & TestFlight デプロイ)
├── scripts/
│   └── rename_project.sh    # リネーム・量産化スクリプト
└── AGENTS.md                # AI エージェント開発ガイド
```
