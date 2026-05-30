#include "wake_lock.h"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QNativeInterface>

static constexpr jint FLAG_KEEP_SCREEN_ON = 0x00000080;

void WakeLock::acquire() {
    // Qt6 Android: main Qt thread = Android main thread → 直接 JNI 呼び出し可能
    // context() は Activity を返す (Application ではない)
    QJniObject activity{QNativeInterface::QAndroidApplication::context()};
    if (!activity.isValid()) return;

    QJniObject window = activity.callObjectMethod(
        "getWindow", "()Landroid/view/Window;");
    if (!window.isValid()) return;

    window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
}

void WakeLock::release() {
    QJniObject activity{QNativeInterface::QAndroidApplication::context()};
    if (!activity.isValid()) return;

    QJniObject window = activity.callObjectMethod(
        "getWindow", "()Landroid/view/Window;");
    if (!window.isValid()) return;

    window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
}

#elif defined(Q_OS_WIN)
#include <windows.h>

void WakeLock::acquire() {
    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
}

void WakeLock::release() {
    SetThreadExecutionState(ES_CONTINUOUS);
}

#else
void WakeLock::acquire() {}
void WakeLock::release() {}
#endif
