#pragma once

#include <QGraphicsView>

class BlockScene;

class BlockView : public QGraphicsView {
    Q_OBJECT
public:
    explicit BlockView(BlockScene* scene, QWidget* parent = nullptr);

    void zoomIn();
    void zoomOut();
    void zoomToFit();

public slots:
    void deleteSelected();
    void copySelected();
    void cutSelected();
    void pasteClipboard();

signals:
    void zoomChanged(double scaleFactor);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    void applyZoom(double factor, QPointF centerPoint);
    double m_currentZoom = 1.0;
};
