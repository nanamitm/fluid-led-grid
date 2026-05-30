#include "gravity_source.h"

#ifdef Q_OS_ANDROID

#include <QAccelerometerReading>

RealGravitySource::RealGravitySource(QObject *parent)
    : GravitySource(parent)
    , m_sensor(new QAccelerometer(this))
{}

void RealGravitySource::start() {
    m_sensor->setDataRate(60);
    connect(m_sensor, &QAccelerometer::readingChanged, this, [this]() {
        auto *r = m_sensor->reading();
        // ローパスフィルタ (α=0.8) でノイズ軽減
        const float alpha = 0.8f;
        m_filtered = m_filtered * alpha
                   + QVector2D((float)r->x(), (float)r->y()) * (1.0f - alpha);
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
