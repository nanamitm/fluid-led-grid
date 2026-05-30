#pragma once
#include <QObject>
#include <QVector2D>
#include "fluid_particles.h"

class FluidModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int   gridSize    READ gridSize    CONSTANT)
    Q_PROPERTY(float viscosity   READ viscosity   WRITE setViscosity   NOTIFY viscosityChanged)
    Q_PROPERTY(float sensitivity READ sensitivity WRITE setSensitivity NOTIFY sensitivityChanged)
    Q_PROPERTY(int   colorHue    READ colorHue    WRITE setColorHue    NOTIFY colorHueChanged)

public:
    explicit FluidModel(QObject *parent = nullptr);

    int   gridSize()    const { return FluidParticles::GRID; }
    float viscosity()   const { return m_sim.viscosity(); }
    float sensitivity() const { return m_sensitivity; }
    int   colorHue()    const { return m_colorHue; }

    void setViscosity(float v);
    void setSensitivity(float s);
    void setColorHue(int hue);

    Q_INVOKABLE float brightness(int x, int y) const;
    Q_INVOKABLE void  reset();

signals:
    void updated();
    void viscosityChanged(float v);
    void sensitivityChanged(float s);
    void colorHueChanged(int hue);

public slots:
    void tick();
    void onGravityChanged(QVector2D g);

private:
    FluidParticles m_sim;
    float          m_sensitivity = 1.0f;
    int            m_colorHue    = 200;   // 0-359: デフォルト水色
    QVector2D      m_rawGravity;
};
