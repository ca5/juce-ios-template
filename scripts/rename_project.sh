#!/usr/bin/env bash
# ==============================================================================
# rename_project.sh - Instantly rebrand and customize this JUCE iOS template
#
# Usage:
#   ./scripts/rename_project.sh <ProjectName> <BundleID> [TeamID] [ManufacturerCode] [PluginCode]
#
# Example:
#   ./scripts/rename_project.sh DrumMachine me.ca5.drummachine MX8BPUQ84Q Ca5x DrMc
# ==============================================================================

set -euo pipefail

NEW_NAME="${1:-}"
NEW_BUNDLE_ID="${2:-}"
NEW_TEAM_ID="${3:-MX8BPUQ84Q}"
NEW_MFR_CODE="${4:-Ca5x}"
NEW_PLUGIN_CODE="${5:-Jatp}"

if [ -z "$NEW_NAME" ] || [ -z "$NEW_BUNDLE_ID" ]; then
    echo "Usage: ./scripts/rename_project.sh <ProjectName> <BundleID> [TeamID] [ManufacturerCode] [PluginCode]"
    echo "Example: ./scripts/rename_project.sh DrumMachine me.ca5.drummachine MX8BPUQ84Q Ca5x DrMc"
    exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "🚀 Rebranding template to: $NEW_NAME ($NEW_BUNDLE_ID)..."

# 1. Update CMakeLists.txt
sed -i '' "s/project(JuceAppTemplate/project($NEW_NAME/g" "$ROOT_DIR/CMakeLists.txt"
sed -i '' "s/juce_add_plugin(JuceAppTemplate/juce_add_plugin($NEW_NAME/g" "$ROOT_DIR/CMakeLists.txt"
sed -i '' "s/BUNDLE_ID \"me.ca5.template\"/BUNDLE_ID \"$NEW_BUNDLE_ID\"/g" "$ROOT_DIR/CMakeLists.txt"
sed -i '' "s/PRODUCT_NAME \"JuceAppTemplate\"/PRODUCT_NAME \"$NEW_NAME\"/g" "$ROOT_DIR/CMakeLists.txt"
sed -i '' "s/DEVELOPMENT_TEAM \"MX8BPUQ84Q\"/DEVELOPMENT_TEAM \"$NEW_TEAM_ID\"/g" "$ROOT_DIR/CMakeLists.txt"
sed -i '' "s/MANUFACTURER_CODE \"Ca5x\"/MANUFACTURER_CODE \"$NEW_MFR_CODE\"/g" "$ROOT_DIR/CMakeLists.txt"
sed -i '' "s/PLUGIN_CODE \"Jatp\"/PLUGIN_CODE \"$NEW_PLUGIN_CODE\"/g" "$ROOT_DIR/CMakeLists.txt"
sed -i '' "s/JuceAppTemplate/$NEW_NAME/g" "$ROOT_DIR/CMakeLists.txt"

# 2. Update fastlane/Appfile & fastlane/Fastfile
sed -i '' "s/app_identifier(\"me.ca5.template\")/app_identifier(\"$NEW_BUNDLE_ID\")/g" "$ROOT_DIR/fastlane/Appfile"
sed -i '' "s/team_id(\"MX8BPUQ84Q\")/team_id(\"$NEW_TEAM_ID\")/g" "$ROOT_DIR/fastlane/Appfile"

sed -i '' "s/APP_ID = \"me.ca5.template\"/APP_ID = \"$NEW_BUNDLE_ID\"/g" "$ROOT_DIR/fastlane/Fastfile"
sed -i '' "s/AUV3_ID = \"me.ca5.template.templateAUv3\"/AUV3_ID = \"${NEW_BUNDLE_ID}.${NEW_NAME}AUv3\"/g" "$ROOT_DIR/fastlane/Fastfile"
sed -i '' "s/TEAM_ID = ENV\[\"APPLE_TEAM_ID\"\] || \"MX8BPUQ84Q\"/TEAM_ID = ENV\[\"APPLE_TEAM_ID\"\] || \"$NEW_TEAM_ID\"/g" "$ROOT_DIR/fastlane/Fastfile"
sed -i '' "s/JuceAppTemplate\.xcodeproj/${NEW_NAME}.xcodeproj/g" "$ROOT_DIR/fastlane/Fastfile"
sed -i '' "s/JuceAppTemplate_Standalone/${NEW_NAME}_Standalone/g" "$ROOT_DIR/fastlane/Fastfile"
sed -i '' "s/JuceAppTemplate_AUv3/${NEW_NAME}_AUv3/g" "$ROOT_DIR/fastlane/Fastfile"
sed -i '' "s/JuceAppTemplate/$NEW_NAME/g" "$ROOT_DIR/fastlane/Fastfile"

# 3. Rename Standalone entitlements
if [ -f "$ROOT_DIR/Assets/JuceAppTemplate_Standalone.entitlements" ]; then
    mv "$ROOT_DIR/Assets/JuceAppTemplate_Standalone.entitlements" "$ROOT_DIR/Assets/${NEW_NAME}_Standalone.entitlements"
    sed -i '' "s/JuceAppTemplate_Standalone\.entitlements/${NEW_NAME}_Standalone.entitlements/g" "$ROOT_DIR/CMakeLists.txt"
fi

# 4. Update C++ Source classes
find "$ROOT_DIR/Source" -type f \( -name "*.h" -o -name "*.cpp" -o -name "*.mm" \) -exec sed -i '' "s/JuceAppTemplate/$NEW_NAME/g" {} +

# 5. Update UI Title
sed -i '' "s/JUCE APP TEMPLATE/$NEW_NAME/g" "$ROOT_DIR/ui/src/App.tsx"
sed -i '' "s/<title>JUCE App Template<\/title>/<title>$NEW_NAME<\/title>/g" "$ROOT_DIR/ui/index.html"

echo "✅ Rebrand completed successfully!"
echo "Next steps:"
echo "  1. Replace Assets/icon.png with your 1024x1024 RGB app icon."
echo "  2. Test locally: npm --prefix ui run build && npm --prefix ui test"
echo "  3. Push to GitHub and deploy to TestFlight!"
