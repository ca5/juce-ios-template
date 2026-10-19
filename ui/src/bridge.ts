// JUCE C++ Native <-> Web UI Bridge

type EventCallback = (payload: any) => void;

class JuceBridge {
  private listeners: Map<string, Set<EventCallback>> = new Map();

  constructor() {
    if (typeof window !== "undefined") {
      (window as any).__juceReceiveEvent = (eventName: string, payload: any) => {
        const callbacks = this.listeners.get(eventName);
        if (callbacks) {
          callbacks.forEach((cb) => cb(payload));
        }
      };
    }
  }

  // Check if running inside JUCE Native WebView
  public isNative(): boolean {
    if (typeof window === "undefined") return false;
    const anyWin = window as any;
    return !!(anyWin.__JUCE__?.backend?.emitEvent || anyWin.webkit?.messageHandlers?.juce);
  }

  // Send an event or parameter change to JUCE C++
  public send(eventName: string, payload: any): void {
    if (typeof window !== "undefined") {
      const anyWin = window as any;
      if (anyWin.__JUCE__?.backend?.emitEvent) {
        anyWin.__JUCE__.backend.emitEvent("juceBridge", { event: eventName, payload });
        return;
      }
      if (anyWin.webkit?.messageHandlers?.juce) {
        anyWin.webkit.messageHandlers.juce.postMessage({
          event: eventName,
          payload
        });
        return;
      }
    }
    console.log(`[JuceBridge Mock] Send: ${eventName}`, payload);
  }

  // Subscribe to native JUCE events
  public on(eventName: string, callback: EventCallback): () => void {
    if (!this.listeners.has(eventName)) {
      this.listeners.set(eventName, new Set());
    }
    this.listeners.get(eventName)!.add(callback);

    return () => {
      this.listeners.get(eventName)?.delete(callback);
    };
  }
}

export const bridge = new JuceBridge();
