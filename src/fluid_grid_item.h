#pragma once
#include <QQuickPaintedItem>
#include "fluid_model.h"

// LED グリッドを直接描画する QQuick アイテム
class FluidGridItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(FluidModel* model READ model WRITE setModel NOTIFY modelChanged)

public:
    explicit FluidGridItem(QQuickItem *parent = nullptr);

    FluidModel *model() const { return m_model; }
    void setModel(FluidModel *m);

    void paint(QPainter *painter) override;

signals:
    void modelChanged();

private:
    FluidModel *m_model = nullptr;
};
