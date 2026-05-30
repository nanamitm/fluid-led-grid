#include "fluid_particles.h"
#include <algorithm>
#include <cmath>

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

FluidParticles::FluidParticles() {
    setViscosity(m_viscosity);
    reset();
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
    m_viscosity  = std::clamp(v, 0.0f, 1.0f);
    m_velDecay   = lerp(0.98f, 0.60f, m_viscosity);
    m_velMix     = lerp(0.95f, 0.55f, m_viscosity);
    m_wallBounce = lerp(0.92f, 0.20f, m_viscosity);
    m_wallScatter = lerp(0.50f, 0.02f, m_viscosity);
    m_elasticity  = lerp(0.85f, 0.05f, m_viscosity);
    m_cohesion    = lerp(0.00f, 0.04f, m_viscosity);
}

// ---- 空間グリッド構築 ----------------------------------------
void FluidParticles::buildSpatialGrid() {
    m_cellCount.fill(0);
    for (int i = 0; i < COUNT; ++i) {
        int cx = std::clamp(int(m_particles[i].pos.x()), 0, GRID - 1);
        int cy = std::clamp(int(m_particles[i].pos.y()), 0, GRID - 1);
        int ci = cy * GRID + cx;
        uint8_t &cnt = m_cellCount[ci];
        if (cnt < MAX_PER_CELL)
            m_cellData[ci][cnt++] = uint8_t(i);
    }
}

// ---- 粒子衝突 (空間グリッドで近傍のみチェック) ---------------
void FluidParticles::applyParticleCollisions() {
    for (int i = 0; i < COUNT; ++i) {
        int cx = std::clamp(int(m_particles[i].pos.x()), 0, GRID - 1);
        int cy = std::clamp(int(m_particles[i].pos.y()), 0, GRID - 1);

        for (int dy = -1; dy <= 1; ++dy) {
            int ncy = cy + dy;
            if (ncy < 0 || ncy >= GRID) continue;
            for (int dx = -1; dx <= 1; ++dx) {
                int ncx = cx + dx;
                if (ncx < 0 || ncx >= GRID) continue;

                int ci  = ncy * GRID + ncx;
                int cnt = m_cellCount[ci];
                for (int k = 0; k < cnt; ++k) {
                    int j = m_cellData[ci][k];
                    if (j <= i) continue;  // 重複ペア排除

                    QVector2D d = m_particles[j].pos - m_particles[i].pos;
                    float dist2 = d.x() * d.x() + d.y() * d.y();
                    if (dist2 >= 1.0f || dist2 < 1e-6f) continue;

                    float dist = std::sqrt(dist2);
                    QVector2D n = d / dist;

                    float overlap = 1.0f - dist;
                    m_particles[i].pos -= n * overlap * 0.5f;
                    m_particles[j].pos += n * overlap * 0.5f;

                    QVector2D relVel    = m_particles[j].vel - m_particles[i].vel;
                    float     relNormal = QVector2D::dotProduct(relVel, n);
                    if (relNormal < 0.0f) {
                        QVector2D impulse = n * relNormal * (1.0f + m_elasticity) * 0.5f;
                        m_particles[i].vel += impulse;
                        m_particles[j].vel -= impulse;
                    }
                }
            }
        }
    }
}

// ---- 凝集力 (空間グリッドで近傍のみチェック) -----------------
void FluidParticles::applyCohesion() {
    static constexpr float COHESION_R  = 3.0f;
    static constexpr float COHESION_R2 = COHESION_R * COHESION_R;
    const int cellR = int(COHESION_R) + 1;

    for (int i = 0; i < COUNT; ++i) {
        int cx = std::clamp(int(m_particles[i].pos.x()), 0, GRID - 1);
        int cy = std::clamp(int(m_particles[i].pos.y()), 0, GRID - 1);

        for (int dy = -cellR; dy <= cellR; ++dy) {
            int ncy = cy + dy;
            if (ncy < 0 || ncy >= GRID) continue;
            for (int dx = -cellR; dx <= cellR; ++dx) {
                int ncx = cx + dx;
                if (ncx < 0 || ncx >= GRID) continue;

                int ci  = ncy * GRID + ncx;
                int cnt = m_cellCount[ci];
                for (int k = 0; k < cnt; ++k) {
                    int j = m_cellData[ci][k];
                    if (j <= i) continue;

                    QVector2D d = m_particles[j].pos - m_particles[i].pos;
                    float dist2 = d.x() * d.x() + d.y() * d.y();
                    if (dist2 <= 1.0f || dist2 >= COHESION_R2) continue;

                    float dist = std::sqrt(dist2);
                    QVector2D n = d / dist;
                    float f = m_cohesion * (1.0f - dist2 / COHESION_R2);
                    m_particles[i].vel += n * f;
                    m_particles[j].vel -= n * f;
                }
            }
        }
    }
}

// ---- メインステップ -------------------------------------------
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

    buildSpatialGrid();
    applyParticleCollisions();
    if (m_cohesion > 0.001f) applyCohesion();
    applySettling();
    rebuildBrightness();
}

// ---- セルベース流動化 ----------------------------------------
// 重力方向の隣接セルが空なら速度ブーストを与える
// → 粒子が詰まりを回り込んで最低位置 (コーナー) に自然に集まる
void FluidParticles::applySettling() {
    if (m_accel.lengthSquared() < 0.1f) return;

    QVector2D gNorm = m_accel.normalized();
    static constexpr float BOOST = 0.25f;

    for (auto &p : m_particles) {
        // 重力方向 1.5 セル先のターゲットセル
        int tcx = std::clamp(int(p.pos.x() + gNorm.x() * 1.5f + 0.5f), 0, GRID - 1);
        int tcy = std::clamp(int(p.pos.y() + gNorm.y() * 1.5f + 0.5f), 0, GRID - 1);
        int ci  = tcy * GRID + tcx;

        // 自分が今いるセル
        int cx = std::clamp(int(p.pos.x() + 0.5f), 0, GRID - 1);
        int cy = std::clamp(int(p.pos.y() + 0.5f), 0, GRID - 1);

        if (tcx == cx && tcy == cy) continue; // 移動先なし

        if (m_cellCount[ci] == 0) {
            // 空きセルあり → 重力方向に速度ブースト
            p.vel += gNorm * BOOST;
        } else {
            // 直前が埋まっている場合、重力に垂直な方向(左右)への小さな揺らぎを加えて
            // 粒子が詰まりを迂回できるようにする
            QVector2D perp{-gNorm.y(), gNorm.x()};
            // 左右のどちらかに空きがあればそちらへ誘導
            auto tryPerp = [&](QVector2D dir) {
                int px = std::clamp(int(p.pos.x() + dir.x() + 0.5f), 0, GRID - 1);
                int py = std::clamp(int(p.pos.y() + dir.y() + 0.5f), 0, GRID - 1);
                if (m_cellCount[py * GRID + px] == 0) {
                    p.vel += dir * (BOOST * 0.5f);
                    return true;
                }
                return false;
            };
            if (!tryPerp(perp)) tryPerp(-perp);
        }
    }
}

void FluidParticles::rebuildBrightness() {
    for (auto &row : m_bright) row.fill(0.0f);

    for (const auto &p : m_particles) {
        float speed     = p.vel.lengthSquared();  // sqrt 不要: 輝度計算のみ
        float intensity = 0.7f + std::min(speed * 0.012f, 0.3f);

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
