#pragma once
#include <QObject>
#include <QVector2D>

// センサー抽象基底: PC/Android 両対応
class GravitySource : public QObject {
    Q_OBJECT
public:
    explicit GravitySource(QObject *parent = nullptr) : QObject(parent) {}
    virtual void start() = 0;

signals:
    void gravityChanged(QVector2D g);
};

// ---- Android: 実加速度センサー ----
#ifdef Q_OS_ANDROID
#include <QAccelerometer>
#include <QScreen>

class RealGravitySource : public GravitySource {
    Q_OBJECT
public:
    explicit RealGravitySource(QObject *parent = nullptr);
    void start() override;
private:
    QVector2D mapAxes(float sx, float sy) const;

    QAccelerometer *m_sensor = nullptr;
    QScreen        *m_screen = nullptr;
    QVector2D       m_filtered;
};

#else
// ---- PC: マウス位置 + 移動速度でシミュレート ----
#include <QTimer>
#include <QPoint>

class MouseGravitySource : public GravitySource {
    Q_OBJECT
public:
    explicit MouseGravitySource(QObject *parent = nullptr);
    void start() override;
private:
    QTimer *m_timer  = nullptr;
    QPoint  m_prev;             // 前フレームのマウス位置
    bool    m_first  = true;
};
#endif
