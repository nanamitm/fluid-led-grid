#include "gravity_source.h"

#ifdef Q_OS_ANDROID

#include <QAccelerometerReading>
#include <QGuiApplication>

RealGravitySource::RealGravitySource(QObject *parent)
    : GravitySource(parent)
    , m_sensor(new QAccelerometer(this))
{}

// 画面の回転に応じてセンサー軸を画面座標系に変換する
//   Android センサー座標 (static, 機体固定):
//     X: 右=正, Y: 上=正, Z: 画面手前=正
//   画面座標 (portrait):
//     X: 右=正, Y: 下=正
//   → センサーは X が反転 (右に傾けると sensor.x が負になる)
//   → Y は変換不要 (直立時 sensor.y ≈ +9.8, 画面下方向に正として動く)
QVector2D RealGravitySource::mapAxes(float sx, float sy) const {
    if (!m_screen) return {-sx, sy};

    switch (m_screen->orientation()) {
    case Qt::LandscapeOrientation:         // 90° 時計回り
        return {-sy, -sx};
    case Qt::InvertedPortraitOrientation:  // 180°
        return { sx, -sy};
    case Qt::InvertedLandscapeOrientation: // 270°
        return { sy,  sx};
    default:                               // Portrait (0°, 通常)
        return {-sx,  sy};
    }
}

void RealGravitySource::start() {
    m_screen = QGuiApplication::primaryScreen();
    m_sensor->setDataRate(60);
    connect(m_sensor, &QAccelerometer::readingChanged, this, [this]() {
        auto *r = m_sensor->reading();
        const float alpha = 0.8f;
        QVector2D mapped = mapAxes((float)r->x(), (float)r->y());
        m_filtered = m_filtered * alpha + mapped * (1.0f - alpha);
        emit gravityChanged(m_filtered);
    });
    m_sensor->start();
}

#else

#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>

MouseGravitySource::MouseGravitySource(QObject *parent)
    : GravitySource(parent)
    , m_timer(new QTimer(this))
{}

void MouseGravitySource::start() {
    connect(m_timer, &QTimer::timeout, this, [this]() {
        auto *screen = QGuiApplication::primaryScreen();
        if (!screen) return;

        QPoint absPos = QCursor::pos();
        auto  *win    = QGuiApplication::focusWindow();
        float  halfW  = win && win->isVisible() ? win->width()  * 0.5f : screen->size().width()  * 0.5f;
        float  halfH  = win && win->isVisible() ? win->height() * 0.5f : screen->size().height() * 0.5f;
        float  centX  = win && win->isVisible() ? win->x() + halfW : screen->size().width()  * 0.5f;
        float  centY  = win && win->isVisible() ? win->y() + halfH : screen->size().height() * 0.5f;

        if (m_first) { m_prev = absPos; m_first = false; }

        float gx_pos = (absPos.x() - centX) / halfW * 9.8f;
        float gy_pos = (absPos.y() - centY) / halfH * 9.8f;

        float dx = float(absPos.x() - m_prev.x());
        float dy = float(absPos.y() - m_prev.y());
        float gx_vel = dx / halfW * 9.8f * 3.0f;
        float gy_vel = dy / halfH * 9.8f * 3.0f;

        m_prev = absPos;

        // 合成: 位置で傾き方向、速度で衝撃の強さを決める
        emit gravityChanged({ gx_pos + gx_vel, gy_pos + gy_vel });
    });
    m_timer->start(16);
}

#endif
