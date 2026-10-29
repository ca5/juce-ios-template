# AGENTS.md - JUCE iOS App Template AI Agent & Google Jules Guide

このドキュメントは、**Google Jules** などのクラウドコーディングAIエージェントが本リポジトリ（JUCE iOS テンプレートおよび派生アプリ）のコード修正・機能追加・検証・TestFlight配信を行うためのガイドラインです。

---

## 1. リポジトリ構成

- `CMakeLists.txt`: JUCE + iOS Standalone + AUv3 + React Web UI バンドルの統合ビルド定義
- `Source/`: C++ オーディオ DSP および JUCE WebView エディタ実装
  - `PluginProcessor.cpp` / `PluginProcessor.h`: オーディオエンジン、MIDI処理、パラメータ（APVTS）
  - `PluginEditor.cpp` / `PluginEditor.h`: JUCE WebView ホスト、ネイティブ ⇄ Web UI 双方向イベント通信、AUv3 ネイティブ UI オーバーレイ
  - `AUv3/AUv3Manager.h` / `AUv3Manager.mm`: AUv3 プラグインホストマネージャー（プラグイン検出、安全ロード、Dry/Wet ミックス、UI表示）
  - `StandaloneApp.cpp`: "Audio input is muted..." 警告バーを非表示化したカスタム Standalone 実装
- `ui/`: React 19 + TypeScript + Vite によるフロントエンド UI
  - `src/bridge.ts`: JUCE ネイティブ双方向バインディング（イベント送信・購読、AUv3 制御 API 含む）
  - `src/App.tsx`: メイン UI コンポーネント
  - `tests/project.test.mjs`: プロジェクト設定・アイコン・UI バンドル・AUv3 設定自動検証テスト
- `Assets/`:
  - `icon.png`: アプリアイコン（1024×1024 RGB、アルファチャンネル無し必須）
  - `JuceAppTemplate_Standalone.entitlements`: iOS Standalone 実行時に AUv3 プラグインをホストするための `inter-app-audio` エンタイトルメント定義
- `fastlane/`: iOS ビルドおよび TestFlight 自動アップロード設定 (`Fastfile`, `Appfile`)
- `.github/workflows/`:
  - `deploy.yml`: TestFlight 自動配信ワークフロー（手動実行および `v*` タグ push 対応）
  - `test.yml`: PR / push 時の自動テスト・構文検証 CI
- `scripts/rename_project.sh`: アプリ名・バンドルIDを一括置換する量産化スクリプト

---

## 2. Google Jules (Linux VM) 上での作業・検証手順

Jules などの Linux VM 環境では macOS の `xcodebuild` を直接実行できないため、以下の手順に従って修正・検証を行ってください。

### 2.1 修正対象ファイル
1. **UI の変更**: `ui/src/` 配下の TypeScript / React コンポーネントを編集します。
2. **DSP / オーディオ / ネイティブ機能の変更**: `Source/` 配下の `.cpp` / `.h` を編集します。
3. **バージョン更新や設定変更**: `CMakeLists.txt` の `project(... VERSION x.y.z)` を更新します。
   - 新規リリース時は TestFlight 側で前バージョンのトレインが閉じるため、必ずバージョン（またはタグ）をインクリメントしてください。

### 2.2 コミット前の必須検証コマンド
コード修正後は、必ず以下のコマンドを実行してビルドと単体テストがすべてパスすることを確認してください。

```bash
npm --prefix ui ci
npm --prefix ui run build
npm --prefix ui test
```

---

## 3. TestFlight への自動配信フロー

1. コード修正・テスト完了後、ブランチをプッシュまたは PR をマージします。
2. GitHub リポジトリの **Actions** タブから **`Build and Deploy iOS to TestFlight`** (`.github/workflows/deploy.yml`) を手動実行（`Run workflow`）するか、`v*` タグ（例: `v1.0.0`）をプッシュします。
3. GitHub Actions (`macos-latest`) が以下を自動実行します：
   - `vendor/JUCE` の浅いクローン (`git submodule update --init --depth 1 JUCE`)
   - `ui/` の本番単一バンドル生成 (`npm run build`) と単体テスト (`npm test`)
   - CMake による iOS Xcode プロジェクト生成 (`build_ios/`)
   - Fastlane による証明書・プロファイル自動取得（Standalone + AUv3 AppExtension 両対応）、ビルド番号インクリメント、IPA 生成、および **TestFlight** への自動アップロード
