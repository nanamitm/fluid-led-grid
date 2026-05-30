#include "fluid_particles.h"
#include <algorithm>
#include <cmath>

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

FluidParticles::FluidParticles() {
    setViscosity(m_viscosity);
    int idx = 0;
    for (int y = GRID - 6; y < GRID && idx < COUNT; ++y)
        for (int x = 0; x < GRID && idx < COUNT; ++x) {
            m_particles[idx].pos = {float(x), float(y)};
            m_particles[idx].vel = {0.0f, 0.0f};
            ++idx;
        }
}

void FluidParticles::setAcceleration(QVector2D a) { m_accel = a; }

void FluidParticles::reset() {
    int idx = 0;
    for (int y = GRID - 6; y < GRID && idx < COUNT; ++y)
        for (int x = 0; x < GRID && idx < COUNT; ++x) {
            m_particles[idx].pos = {float(x), float(y)};
            m_particles[idx].vel = {0.0f, 0.0f};
            ++idx;
        }
}

void FluidParticles::setViscosity(float v) {
    m_viscosity = std::clamp(v, 0.0f, 1.0f);

    // ---- 粘度 0(水) → 1(ハチミツ) ----
    // 速度減衰: 水=ほぼなし、ハチミツ=強い
    m_velDecay   = lerp(0.98f, 0.60f, m_viscosity);
    // 加速度取り込み率: 水=素直に加速、ハチミツ=もったり
    m_velMix     = lerp(0.95f, 0.55f, m_viscosity);
    // 壁の弾性: 水=よく跳ねる、ハチミツ=吸収
    m_wallBounce = lerp(0.92f, 0.20f, m_viscosity);
    // 壁衝突の横飛び散り: 水=大きい、ハチミツ=ほぼゼロ
    m_wallScatter = lerp(0.50f, 0.02f, m_viscosity);
    // 粒子衝突の弾性: 水=弾む、ハチミツ=速度平均化
    m_elasticity  = lerp(0.85f, 0.05f, m_viscosity);
    // 凝集引力: 水=なし、ハチミツ=まとまる
    m_cohesion    = lerp(0.00f, 0.04f, m_viscosity);
}

void FluidParticles::step(float dt) {
    QVector2D scaled = m_accel * 0.3f;
    std::uniform_real_distribution<float> scatter(-1.0f, 1.0f);

    for (auto &p : m_particles) {
        p.vel = p.vel * m_velMix + scaled * GRAVITY;

        float vx = std::clamp(p.vel.x(), -MAX_VEL, MAX_VEL);
        float vy = std::clamp(p.vel.y(), -MAX_VEL, MAX_VEL);
        p.vel = {vx, vy};

        float nx = p.pos.x() + p.vel.x();
        float ny = p.pos.y() + p.vel.y();

        // 壁反射: 弾性 + 低粘度時の横飛び散り
        if (nx < 0.0f) {
            nx = 0.0f;
            float impact = std::abs(p.vel.x());
            p.vel.setX(impact * m_wallBounce);
            p.vel.setY(p.vel.y() + scatter(m_rng) * impact * m_wallScatter);
        } else if (nx >= GRID - 1) {
            nx = float(GRID - 1);
            float impact = std::abs(p.vel.x());
            p.vel.setX(-impact * m_wallBounce);
            p.vel.setY(p.vel.y() + scatter(m_rng) * impact * m_wallScatter);
        }
        if (ny < 0.0f) {
            ny = 0.0f;
            float impact = std::abs(p.vel.y());
            p.vel.setY(impact * m_wallBounce);
            p.vel.setX(p.vel.x() + scatter(m_rng) * impact * m_wallScatter);
        } else if (ny >= GRID - 1) {
            ny = float(GRID - 1);
            float impact = std::abs(p.vel.y());
            p.vel.setY(-impact * m_wallBounce);
            p.vel.setX(p.vel.x() + scatter(m_rng) * impact * m_wallScatter);
        }

        p.pos = {nx, ny};
        p.vel *= m_velDecay;
    }

    applyParticleCollisions();
    if (m_cohesion > 0.001f) applyCohesion();
    rebuildBrightness();
}

void FluidParticles::applyParticleCollisions() {
    for (int i = 0; i < COUNT; ++i) {
        for (int j = i + 1; j < COUNT; ++j) {
            QVector2D d = m_particles[j].pos - m_particles[i].pos;
            float dist2 = d.x() * d.x() + d.y() * d.y();

            if (dist2 < 1.0f && dist2 > 1e-6f) {
                float dist  = std::sqrt(dist2);
                QVector2D n = d / dist;  // 正規化方向

                // 重なり解消
                float overlap = 1.0f - dist;
                m_particles[i].pos -= n * overlap * 0.5f;
                m_particles[j].pos += n * overlap * 0.5f;

                // 衝突速度成分
                QVector2D relVel = m_particles[j].vel - m_particles[i].vel;
                float relNormal  = QVector2D::dotProduct(relVel, n);

                // 近づいている場合のみ衝突処理
                if (relNormal < 0.0f) {
                    // elasticity=1: 完全弾性(運動量交換), 0: 完全非弾性(速度平均化)
                    QVector2D impulse = n * relNormal;
                    m_particles[i].vel += impulse * (1.0f + m_elasticity) * 0.5f;
                    m_particles[j].vel -= impulse * (1.0f + m_elasticity) * 0.5f;
                }
            }
        }
    }
}

void FluidParticles::applyCohesion() {
    // 近傍粒子に弱い引力 (ハチミツのまとまり感)
    static constexpr float COHESION_R2 = 9.0f;  // 影響半径3グリッド

    for (int i = 0; i < COUNT; ++i) {
        for (int j = i + 1; j < COUNT; ++j) {
            QVector2D d = m_particles[j].pos - m_particles[i].pos;
            float dist2 = d.x() * d.x() + d.y() * d.y();

            if (dist2 > 1.0f && dist2 < COHESION_R2) {
                float dist  = std::sqrt(dist2);
                QVector2D n = d / dist;
                float f = m_cohesion * (1.0f - dist2 / COHESION_R2);
                m_particles[i].vel += n * f;
                m_particles[j].vel -= n * f;
            }
        }
    }
}

void FluidParticles::rebuildBrightness() {
    for (auto &row : m_bright) row.fill(0.0f);

    for (const auto &p : m_particles) {
        float speed     = p.vel.length();
        float intensity = 0.7f + std::min(speed * 0.06f, 0.3f);

        int x0 = std::max(0,        int(p.pos.x()) - 2);
        int x1 = std::min(GRID - 1, int(p.pos.x()) + 2);
        int y0 = std::max(0,        int(p.pos.y()) - 2);
        int y1 = std::min(GRID - 1, int(p.pos.y()) + 2);

        for (int cy = y0; cy <= y1; ++cy)
            for (int cx = x0; cx <= x1; ++cx) {
                float dx = cx - p.pos.x();
                float dy = cy - p.pos.y();
                float r2 = dx * dx + dy * dy;
                float w  = std::exp(-r2 / (INFLUENCE_R * INFLUENCE_R)) * intensity;
                m_bright[cy][cx] = std::min(m_bright[cy][cx] + w, 1.0f);
            }
    }
}

float FluidParticles::brightness(int x, int y) const {
    return m_bright[y][x];
}
