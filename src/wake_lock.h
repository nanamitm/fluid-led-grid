#pragma once

// スリープ抑止 (Android: FLAG_KEEP_SCREEN_ON / Windows: SetThreadExecutionState)
class WakeLock {
public:
    static void acquire();
    static void release();
};
