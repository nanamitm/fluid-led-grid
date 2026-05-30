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
    static constexpr int COUNT = 120;  // 30% of 400

    FluidParticles();

    void setAcceleration(QVector2D a);
    void setViscosity(float v);
    float viscosity() const { return m_viscosity; }

    void reset();   // 粒子を下部に再配置
    void step(float dt);
    float brightness(int x, int y) const;

private:
    void applyParticleCollisions();
    void applyCohesion();
    void rebuildBrightness();

    std::array<Particle, COUNT> m_particles;
    QVector2D m_accel{0.0f, 0.0f};
    std::array<std::array<float, GRID>, GRID> m_bright{};
    std::mt19937 m_rng{42};

    float m_viscosity   = 0.2f;

    // setViscosity() で導出
    float m_velDecay    = 0.97f;
    float m_velMix      = 0.92f;
    float m_wallBounce  = 0.90f;  // 壁の弾性: 高=よく跳ねる
    float m_wallScatter = 0.40f;  // 壁衝突時の横方向飛び散り強度
    float m_elasticity  = 0.80f;  // 粒子衝突の弾性: 1=完全弾性, 0=完全非弾性
    float m_cohesion    = 0.00f;  // 近傍粒子への引力

    static constexpr float GRAVITY     = 0.08f;
    static constexpr float MAX_VEL     = 2.9f;
    static constexpr float INFLUENCE_R = 1.4f;
};
