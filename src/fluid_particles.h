#pragma once
#include <QVector2D>
#include <array>
#include <random>

struct Particle {
    QVector2D pos;
    QVector2D vel;
};

class FluidParticles {
public:
    static constexpr int GRID  = 20;
    static constexpr int COUNT = 120;

    FluidParticles();

    void setAcceleration(QVector2D a);
    void setViscosity(float v);
    float viscosity() const { return m_viscosity; }

    void reset();
    void step(float dt);
    float brightness(int x, int y) const;

private:
    void buildSpatialGrid();
    void applyParticleCollisions();
    void applyCohesion();
    void rebuildBrightness();

    std::array<Particle, COUNT> m_particles;
    QVector2D m_accel{0.0f, 0.0f};
    std::array<std::array<float, GRID>, GRID> m_bright{};
    std::mt19937 m_rng{42};

    // 空間グリッド: O(N²) → O(N) 近傍探索
    static constexpr int MAX_PER_CELL = 20;
    std::array<std::array<uint8_t, MAX_PER_CELL>, GRID * GRID> m_cellData{};
    std::array<uint8_t, GRID * GRID>                            m_cellCount{};

    float m_viscosity    = 0.2f;
    float m_velDecay     = 0.97f;
    float m_velMix       = 0.92f;
    float m_wallBounce   = 0.90f;
    float m_wallScatter  = 0.40f;
    float m_elasticity   = 0.80f;
    float m_cohesion     = 0.00f;

    static constexpr float GRAVITY     = 0.08f;
    static constexpr float MAX_VEL     = 2.9f;
    static constexpr float INFLUENCE_R = 1.4f;
};
