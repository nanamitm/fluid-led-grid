#include "fluid_grid_item.h"
#include <QPainter>
#include <cmath>

FluidGridItem::FluidGridItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAntialiasing(true);
}

void FluidGridItem::setModel(FluidModel *m) {
    if (m_model == m) return;
    if (m_model)
        disconnect(m_model, &FluidModel::updated, this, nullptr);

    m_model = m;

    if (m_model)
        connect(m_model, &FluidModel::updated, this, [this]() { update(); });

    emit modelChanged();
    update();
}

void FluidGridItem::paint(QPainter *painter) {
    if (!m_model) return;

    const int   N    = m_model->gridSize();
    const float W    = float(width());
    const float H    = float(height());
    const float side = std::min(W, H);

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->translate(W / 2.0f, H / 2.0f);
    painter->rotate(45.0f);

    const float innerSide = side / float(M_SQRT2);
    const float cellSize  = innerSide / float(N);
    const float ledSize   = cellSize * 0.82f;
    const float offset    = (cellSize - ledSize) / 2.0f;

    painter->translate(-innerSide / 2.0f, -innerSide / 2.0f);

    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            float b = m_model->brightness(x, y);
            if (b < 0.02f) continue;

            // 色相は FluidModel.colorHue から取得
            float hue = float(m_model->colorHue());
            float sat = 1.0f - b * 0.4f; // 速い粒子ほど白っぽく
            float val = b;

            // HSV → RGB
            float c = val * sat;
            float h6 = hue / 60.0f;
            float x1 = c * (1.0f - std::abs(std::fmod(h6, 2.0f) - 1.0f));
            float r = 0, g = 0, bl = 0;
            if      (h6 < 1) { r = c;  g = x1; bl = 0; }
            else if (h6 < 2) { r = x1; g = c;  bl = 0; }
            else if (h6 < 3) { r = 0;  g = c;  bl = x1; }
            else if (h6 < 4) { r = 0;  g = x1; bl = c; }
            else if (h6 < 5) { r = x1; g = 0;  bl = c; }
            else              { r = c;  g = 0;  bl = x1; }
            float m2 = val - c;
            QColor ledColor(
                int((r  + m2) * 255),
                int((g  + m2) * 255),
                int((bl + m2) * 255),
                255
            );

            // 輝度が高い粒子にグロー
            if (b > 0.5f) {
                float glow = (b - 0.5f) * 2.0f;
                float gr = ledSize * (1.0f + glow * 0.6f);
                QColor glowC(120, 200, 255, int(glow * 50));
                painter->setPen(Qt::NoPen);
                painter->setBrush(glowC);
                float go = (cellSize - gr) / 2.0f;
                painter->drawEllipse(QRectF(
                    x * cellSize + go, y * cellSize + go, gr, gr));
            }

            // LED 本体
            painter->setPen(Qt::NoPen);
            painter->setBrush(ledColor);
            painter->drawRoundedRect(QRectF(
                x * cellSize + offset,
                y * cellSize + offset,
                ledSize, ledSize),
                ledSize * 0.3f, ledSize * 0.3f);
        }
    }
}
