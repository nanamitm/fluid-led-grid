#pragma once
#include <QQuickItem>
#include "fluid_model.h"

// QSGGeometryNode ベースの GPU 描画 (QPainter より大幅に高速)
class FluidGridItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(FluidModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
    explicit FluidGridItem(QQuickItem *parent = nullptr);

    FluidModel *model() const { return m_model; }
    void setModel(FluidModel *m);

signals:
    void modelChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *) override;

private:
    FluidModel *m_model = nullptr;
};
