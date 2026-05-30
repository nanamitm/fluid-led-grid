#include "gravity_source.h"

#ifdef Q_OS_ANDROID

#include <QAccelerometerReading>
#include <QGuiApplication>

RealGravitySource::RealGravitySource(QObject *parent)
    : GravitySource(parent)
    , m_sensor(new QAccelerometer(this))
{}

// 画面の回転に応じてセンサー軸を画面座標系に変換する
//   Android センサー: X=右, Y=上(画面Y反転), Z=手前
//   画面座標:         X=右, Y=下
//   → 基本は Y を反転、さらに画面回転に合わせて X/Y を入れ替え
QVector2D RealGravitySource::mapAxes(float sx, float sy) const {
    if (!m_screen) return {sx, -sy};

    switch (m_screen->orientation()) {
    case Qt::LandscapeOrientation:         // 90° 時計回り
        return { sy,  sx};
    case Qt::InvertedPortraitOrientation:  // 180°
        return {-sx,  sy};
    case Qt::InvertedLandscapeOrientation: // 270°
        return {-sy, -sx};
    default:                               // Portrait (0°, 通常)
        return { sx, -sy};
    }
}

void RealGravitySource::start() {
    m_screen = QGuiApplication::primaryScreen();

    // 画面回転を追跡
    if (m_screen) {
        m_screen->setOrientationUpdateMask(
            Qt::PortraitOrientation | Qt::InvertedPortraitOrientation |
            Qt::LandscapeOrientation | Qt::InvertedLandscapeOrientation);
    }

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

MouseGravitySource::MouseGravitySource(QObject *parent)
    : GravitySource(parent)
    , m_timer(new QTimer(this))
{}

void MouseGravitySource::start() {
    connect(m_timer, &QTimer::timeout, this, [this]() {
        auto *screen = QGuiApplication::primaryScreen();
        if (!screen) return;

        QPoint pos  = QCursor::pos();
        QSize  size = screen->size();

        if (m_first) { m_prev = pos; m_first = false; }

        // 位置成分: 画面中央を水平、端で ±9.8
        float gx_pos = (pos.x() / float(size.width())  - 0.5f) * 2.0f * 9.8f;
        float gy_pos = (pos.y() / float(size.height()) - 0.5f) * 2.0f * 9.8f;

        // 速度成分: マウスを素早く動かすと追加の衝撃 (実機を振る動作に対応)
        //   1フレーム(16ms)あたりのピクセル移動量 → ±9.8 にスケール
        float dx = float(pos.x() - m_prev.x());
        float dy = float(pos.y() - m_prev.y());
        float gx_vel = dx * (9.8f / float(size.width())  * 6.0f);
        float gy_vel = dy * (9.8f / float(size.height()) * 6.0f);

        m_prev = pos;

        // 合成: 位置で傾き方向、速度で衝撃の強さを決める
        emit gravityChanged({ gx_pos + gx_vel, gy_pos + gy_vel });
    });
    m_timer->start(16);
}

#endif
