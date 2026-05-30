#include "fluid_model.h"
#include <QSettings>
#include <cmath>

FluidModel::FluidModel(QObject *parent)
    : QObject(parent)
{
    QSettings s("FluidLedGrid", "FluidLedGrid");
    m_sim.setViscosity(s.value("viscosity",   0.2f).toFloat());
    m_sensitivity = qBound(0.2f, s.value("sensitivity", 1.0f).toFloat(), 3.0f);
    m_colorHue    = qBound(0,    s.value("colorHue",    200).toInt(),    359);
}

// スナップショットから読む (レンダースレッドから安全に呼べる)
float FluidModel::brightness(int x, int y) const { return m_brightSnap[y][x]; }

void FluidModel::setViscosity(float v) {
    if (qFuzzyCompare(v, m_sim.viscosity())) return;
    m_sim.setViscosity(v);
    QSettings("FluidLedGrid", "FluidLedGrid").setValue("viscosity", v);
    emit viscosityChanged(v);
}

void FluidModel::setSensitivity(float s) {
    s = qBound(0.2f, s, 3.0f);
    if (qFuzzyCompare(s, m_sensitivity)) return;
    m_sensitivity = s;
    applyGravityToSim();
    QSettings("FluidLedGrid", "FluidLedGrid").setValue("sensitivity", s);
    emit sensitivityChanged(s);
}

void FluidModel::setColorHue(int hue) {
    hue = qBound(0, hue, 359);
    if (hue == m_colorHue) return;
    m_colorHue = hue;
    QSettings("FluidLedGrid", "FluidLedGrid").setValue("colorHue", hue);
    emit colorHueChanged(hue);
}

void FluidModel::reset() { m_sim.reset(); }

void FluidModel::tick() {
    m_sim.step(0.016f);
    // emit 前にスナップショットを更新 → レンダースレッドは安定したデータを読む
    const int N = FluidParticles::GRID;
    for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x)
            m_brightSnap[y][x] = m_sim.brightness(x, y);
    emit updated();
}

void FluidModel::onGravityChanged(QVector2D g) {
    m_rawGravity = g;
    applyGravityToSim();
}

void FluidModel::applyGravityToSim() {
    // 表示グリッドは +45° 回転描画されているため、
    // 重力ベクトルを -45° 回転してシミュレーション座標系に合わせる
    // 回転 -45°: x' = (gx - gy) / √2,  y' = (gx + gy) / √2
    static const float S = 1.0f / float(M_SQRT2);
    QVector2D rotated(
        (m_rawGravity.x() - m_rawGravity.y()) * S,
        (m_rawGravity.x() + m_rawGravity.y()) * S);
    m_sim.setAcceleration(rotated * m_sensitivity);
}
