import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(__dirname, "../..");

test("UI single-file bundle is built and contains root container", () => {
  const distHtml = path.join(repoRoot, "ui/dist/index.html");
  assert.ok(fs.existsSync(distHtml), "ui/dist/index.html must exist (run npm run build first)");
  const html = fs.readFileSync(distHtml, "utf8");
  assert.match(html, /<div id="root"><\/div>/);
});

test("CMakeLists.txt contains iOS deployment target, team ID, and encryption exemption", () => {
  const cmake = fs.readFileSync(path.join(repoRoot, "CMakeLists.txt"), "utf8");
  assert.match(cmake, /CMAKE_OSX_DEPLOYMENT_TARGET\s+"1[56]\.0"/);
  assert.match(cmake, /ITSAppUsesNonExemptEncryption/);
  assert.match(cmake, /BuildReactUI/);
});

test("App icon Assets/icon.png is 1024x1024 RGB without alpha channel", () => {
  const iconPath = path.join(repoRoot, "Assets/icon.png");
  assert.ok(fs.existsSync(iconPath), "Assets/icon.png must exist");
  const buf = fs.readFileSync(iconPath);
  assert.equal(buf.toString("ascii", 12, 16), "IHDR");
  const width = buf.readUInt32BE(16);
  const height = buf.readUInt32BE(20);
  const colorType = buf.readUInt8(25);
  assert.equal(width, 1024, "Icon width must be 1024");
  assert.equal(height, 1024, "Icon height must be 1024");
  assert.equal(colorType, 2, "Icon PNG colorType must be 2 (Truecolor RGB without alpha)");
});

test("Fastlane configuration matches Standalone and AUv3 bundle identifiers", () => {
  const fastfile = fs.readFileSync(path.join(repoRoot, "fastlane/Fastfile"), "utf8");
  assert.match(fastfile, /APP_ID = "me\.ca5\./);
  assert.match(fastfile, /AUV3_ID = "me\.ca5\./);
  assert.match(fastfile, /latest_build\.to_i \+ 1/);
});

test("Custom StandaloneApp exists and suppresses feedback loop warning", () => {
  const standaloneAppPath = path.join(repoRoot, "Source/StandaloneApp.cpp");
  assert.ok(fs.existsSync(standaloneAppPath), "Source/StandaloneApp.cpp must exist");
  const cmake = fs.readFileSync(path.join(repoRoot, "CMakeLists.txt"), "utf8");
  assert.match(cmake, /JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1/);
  assert.match(cmake, /Source\/StandaloneApp\.cpp/);
});

