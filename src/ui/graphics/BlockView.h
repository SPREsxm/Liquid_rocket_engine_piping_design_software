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

    void setGridVisible(bool visible);
    bool isGridVisible() const;

public slots:
    void deleteSelected();
    void copySelected();
    void cutSelected();
    void pasteClipboard();

signals:
    void zoomChanged(double scaleFactor);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    void applyZoom(double factor, QPointF centerPoint);
    double m_currentZoom = 1.0;
    bool m_gridVisible = true;
    bool m_isPanning = false;
    QPointF m_lastPanPoint;
    BlockScene* m_scene;
};
