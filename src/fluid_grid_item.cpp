#include "fluid_grid_item.h"
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>
#include <QSGTransformNode>
#include <cmath>

FluidGridItem::FluidGridItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void FluidGridItem::setModel(FluidModel *m) {
    if (m_model == m) return;
    if (m_model)
        disconnect(m_model, &FluidModel::updated, this, nullptr);
    m_model = m;
    if (m_model)
        connect(m_model, &FluidModel::updated, this, &QQuickItem::update);
    emit modelChanged();
    update();
}

// HSV → RGB (b=brightness=value, s derived from brightness)
static void ledColor(float hue, float bright,
                     uint8_t &r, uint8_t &g, uint8_t &b)
{
    float sat = 1.0f - bright * 0.4f;   // 明るいほど白っぽく
    float c   = bright * sat;
    float h6  = hue / 60.0f;
    float x   = c * (1.0f - std::abs(std::fmod(h6, 2.0f) - 1.0f));
    float r1 = 0, g1 = 0, b1 = 0;
    if      (h6 < 1) { r1 = c;  g1 = x;  b1 = 0; }
    else if (h6 < 2) { r1 = x;  g1 = c;  b1 = 0; }
    else if (h6 < 3) { r1 = 0;  g1 = c;  b1 = x; }
    else if (h6 < 4) { r1 = 0;  g1 = x;  b1 = c; }
    else if (h6 < 5) { r1 = x;  g1 = 0;  b1 = c; }
    else             { r1 = c;  g1 = 0;  b1 = x; }
    float m = bright - c;
    r = uint8_t(std::min((r1 + m) * 255.0f, 255.0f));
    g = uint8_t(std::min((g1 + m) * 255.0f, 255.0f));
    b = uint8_t(std::min((b1 + m) * 255.0f, 255.0f));
}

// 頂点 6 個でクワッドを書く (triangle list)
static inline void writeQuad(QSGGeometry::ColoredPoint2D *v,
                              float x0, float y0, float x1, float y1,
                              uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    v[0].set(x0, y0, r, g, b, a);
    v[1].set(x1, y0, r, g, b, a);
    v[2].set(x0, y1, r, g, b, a);
    v[3].set(x1, y0, r, g, b, a);
    v[4].set(x1, y1, r, g, b, a);
    v[5].set(x0, y1, r, g, b, a);
}

QSGNode *FluidGridItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (!m_model) { delete oldNode; return nullptr; }

    const int   N         = m_model->gridSize();
    const float W         = float(width());
    const float H         = float(height());
    const float side      = std::min(W, H);
    const float innerSide = side / float(M_SQRT2);
    const float cell      = innerSide / float(N);
    const float led       = cell * 0.82f;
    const float off       = (cell - led) / 2.0f;
    const float hue       = float(m_model->colorHue());

    // ---- ノード構成: TransformNode → (GlowNode, LedNode) ----
    QSGTransformNode *txNode   = nullptr;
    QSGGeometryNode  *glowNode = nullptr;
    QSGGeometryNode  *ledNode  = nullptr;

    if (oldNode) {
        txNode   = static_cast<QSGTransformNode  *>(oldNode);
        glowNode = static_cast<QSGGeometryNode   *>(txNode->firstChild());
        ledNode  = static_cast<QSGGeometryNode   *>(glowNode->nextSibling());
    } else {
        txNode = new QSGTransformNode;

        // グロー用ノード (半透明・大きめクワッド)
        glowNode = new QSGGeometryNode;
        {
            auto *mat = new QSGVertexColorMaterial;
            mat->setFlag(QSGMaterial::Blending);
            glowNode->setMaterial(mat);
            glowNode->setFlag(QSGNode::OwnsMaterial);
            auto *geo = new QSGGeometry(
                QSGGeometry::defaultAttributes_ColoredPoint2D(), N * N * 6);
            geo->setDrawingMode(QSGGeometry::DrawTriangles);
            glowNode->setGeometry(geo);
            glowNode->setFlag(QSGNode::OwnsGeometry);
        }

        // LED 本体ノード (不透明)
        ledNode = new QSGGeometryNode;
        {
            auto *mat = new QSGVertexColorMaterial;
            ledNode->setMaterial(mat);
            ledNode->setFlag(QSGNode::OwnsMaterial);
            auto *geo = new QSGGeometry(
                QSGGeometry::defaultAttributes_ColoredPoint2D(), N * N * 6);
            geo->setDrawingMode(QSGGeometry::DrawTriangles);
            ledNode->setGeometry(geo);
            ledNode->setFlag(QSGNode::OwnsGeometry);
        }

        txNode->appendChildNode(glowNode);
        txNode->appendChildNode(ledNode);
    }

    // 変換行列: 中央寄せ + 45° 回転
    QMatrix4x4 mx;
    mx.translate(W / 2.0f, H / 2.0f);
    mx.rotate(45.0f, 0, 0, 1);
    mx.translate(-innerSide / 2.0f, -innerSide / 2.0f);
    txNode->setMatrix(mx);

    // 頂点バッファ更新
    auto *gv = static_cast<QSGGeometry::ColoredPoint2D *>(
        glowNode->geometry()->vertexData());
    auto *lv = static_cast<QSGGeometry::ColoredPoint2D *>(
        ledNode->geometry()->vertexData());

    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            int  idx = (y * N + x) * 6;
            float b  = m_model->brightness(x, y);

            float lx0 = x * cell + off,  ly0 = y * cell + off;
            float lx1 = lx0 + led,       ly1 = ly0 + led;

            if (b < 0.02f) {
                // 消灯: 透明クワッドで埋める (頂点数は固定)
                writeQuad(lv + idx, lx0, ly0, lx1, ly1, 0, 0, 0, 0);
                writeQuad(gv + idx, lx0, ly0, lx1, ly1, 0, 0, 0, 0);
                continue;
            }

            // LED 本体
            uint8_t r, g, bl;
            ledColor(hue, b, r, g, bl);
            writeQuad(lv + idx, lx0, ly0, lx1, ly1, r, g, bl, 255);

            // グロー (液面付近のみ、わずかに大きい半透明クワッド)
            float glowT = std::max(0.0f, 1.0f - std::abs(b - 0.55f) * 4.0f);
            if (glowT > 0.05f) {
                float gr  = led * (1.0f + glowT * 0.7f);
                float go  = (cell - gr) / 2.0f;
                uint8_t ga = uint8_t(glowT * 60.0f);
                uint8_t gr2 = uint8_t(std::min(int(r) + 40, 255));
                uint8_t gb2 = 255;
                writeQuad(gv + idx,
                          x * cell + go, y * cell + go,
                          x * cell + go + gr, y * cell + go + gr,
                          gr2, g, gb2, ga);
            } else {
                writeQuad(gv + idx, lx0, ly0, lx1, ly1, 0, 0, 0, 0);
            }
        }
    }

    glowNode->geometry()->markVertexDataDirty();
    glowNode->markDirty(QSGNode::DirtyGeometry);
    ledNode->geometry()->markVertexDataDirty();
    ledNode->markDirty(QSGNode::DirtyGeometry);

    return txNode;
}
