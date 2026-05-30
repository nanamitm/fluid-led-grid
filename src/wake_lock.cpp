#include "wake_lock.h"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QNativeInterface>

static constexpr jint FLAG_KEEP_SCREEN_ON = 0x00000080;

void WakeLock::acquire() {
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
        QJniObject activity{QNativeInterface::QAndroidApplication::context()};
        QJniObject window = activity.callObjectMethod(
            "getWindow", "()Landroid/view/Window;");
        window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
    });
}

void WakeLock::release() {
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
        QJniObject activity{QNativeInterface::QAndroidApplication::context()};
        QJniObject window = activity.callObjectMethod(
            "getWindow", "()Landroid/view/Window;");
        window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
    });
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
