import React, { useState, useEffect } from "react";
import { bridge } from "./bridge";

export const App: React.FC = () => {
  const [gain, setGain] = useState<number>(0.8);
  const [isPlaying, setIsPlaying] = useState<boolean>(false);

  useEffect(() => {
    const unsub = bridge.on("stateUpdate", (data: any) => {
      if (data?.gain !== undefined) setGain(data.gain);
    });
    return unsub;
  }, []);

  const handleGainChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const val = parseFloat(e.target.value);
    setGain(val);
    bridge.send("setGain", val);
  };

  const toggleTrigger = () => {
    const nextState = !isPlaying;
    setIsPlaying(nextState);
    bridge.send("triggerNote", { active: nextState, note: 60 });
  };

  return (
    <div style={{
      display: "flex",
      flexDirection: "column",
      justifyContent: "space-between",
      alignItems: "center",
      height: "100%",
      padding: "24px 16px",
      background: "radial-gradient(circle at 50% 20%, #1e2433 0%, #0c0e14 100%)",
      color: "#ffffff"
    }}>
      {/* Header */}
      <header style={{ textAlign: "center", marginTop: "12px" }}>
        <h1 style={{ fontSize: "24px", fontWeight: "700", letterSpacing: "1px", color: "#00f0d4" }}>
          JUCE APP TEMPLATE
        </h1>
        <p style={{ fontSize: "12px", color: "#8a94a6", marginTop: "4px" }}>
          iOS Standalone + AUv3 App Extension
        </p>
      </header>

      {/* Main Control Panel */}
      <div style={{
        display: "flex",
        flexDirection: "column",
        alignItems: "center",
        width: "100%",
        maxWidth: "360px",
        gap: "24px"
      }}>
        {/* Trigger Button */}
        <button
          onClick={toggleTrigger}
          style={{
            width: "140px",
            height: "140px",
            borderRadius: "50%",
            border: `3px solid ${isPlaying ? "#00f0d4" : "#2d3748"}`,
            background: isPlaying ? "rgba(0, 240, 212, 0.2)" : "rgba(255, 255, 255, 0.03)",
            color: isPlaying ? "#00f0d4" : "#ffffff",
            fontSize: "18px",
            fontWeight: "700",
            cursor: "pointer",
            boxShadow: isPlaying ? "0 0 30px rgba(0, 240, 212, 0.4)" : "none",
            transition: "all 0.15s ease"
          }}
        >
          {isPlaying ? "ACTIVE" : "TRIGGER"}
        </button>

        {/* Gain Slider */}
        <div style={{ width: "100%", padding: "0 16px" }}>
          <div style={{ display: "flex", justifyContent: "space-between", marginBottom: "8px", fontSize: "14px" }}>
            <span style={{ color: "#a0aec0" }}>MASTER GAIN</span>
            <span style={{ fontWeight: "600", color: "#00f0d4" }}>{Math.round(gain * 100)}%</span>
          </div>
          <input
            type="range"
            min="0"
            max="1"
            step="0.01"
            value={gain}
            onChange={handleGainChange}
            style={{
              width: "100%",
              height: "8px",
              borderRadius: "4px",
              accentColor: "#00f0d4",
              background: "#2d3748",
              cursor: "pointer"
            }}
          />
        </div>
      </div>

      {/* Footer Info */}
      <footer style={{ fontSize: "11px", color: "#4a5568", textAlign: "center" }}>
        JUCE 8 • CMake • Vite SingleFile • Fastlane TestFlight
      </footer>
    </div>
  );
};
