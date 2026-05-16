#pragma once

#include <QGraphicsScene>
#include <QUuid>

class BlockItem;
class PortItem;
class ConnectionItem;
class QGraphicsLineItem;
struct ComponentDescriptor;
class ComponentFactory;
class QUndoStack;

class BlockScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit BlockScene(ComponentFactory* factory, QObject* parent = nullptr);

    void setUndoStack(QUndoStack* undoStack) { m_undoStack = undoStack; }
    QUndoStack* undoStack() const { return m_undoStack; }
    ComponentFactory* factory() const { return m_factory; }

    // Block management
    BlockItem* addBlock(const ComponentDescriptor& descriptor, const QPointF& pos,
                        const QUuid& forcedUuid = {});
    void removeBlock(BlockItem* block);
    QList<BlockItem*> allBlocks() const;
    BlockItem* blockByUuid(const QUuid& uuid) const;

    // Connection management
    ConnectionItem* addConnection(PortItem* source, PortItem* dest);
    void removeConnection(ConnectionItem* conn);
    void deleteSelectedConnections();
    QList<ConnectionItem*> allConnections() const;
    bool canConnect(PortItem* source, PortItem* dest) const;

    // Selection
    BlockItem* selectedBlock() const;
    QList<BlockItem*> selectedBlocks() const;

    // Grid
    void setSnapEnabled(bool enabled) { m_snapEnabled = enabled; }
    bool isSnapEnabled() const { return m_snapEnabled; }
    QPointF snapToGrid(const QPointF& pt) const;

    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
    void clearScene();

signals:
    void blockAdded(BlockItem* block);
    void blockRemoved(const QUuid& uuid);
    void connectionAdded(ConnectionItem* conn);
    void connectionRemoved();
    void blockSelectionChanged(BlockItem* block);
    void multiSelectionChanged();
    void sceneModified();

protected:
    void dropEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    PortItem* portAtPos(const QPointF& scenePos) const;
    void connectBlockSignals(BlockItem* block);
    QList<ConnectionItem*> connectionsForBlock(BlockItem* block) const;
    QList<ConnectionItem*> connectionsForPort(PortItem* port) const;

    ComponentFactory* m_factory;
    QUndoStack* m_undoStack = nullptr;

    // Connection drawing state machine
    bool m_drawingConnection = false;
    PortItem* m_connectionSource = nullptr;
    ConnectionItem* m_reconnectConnection = nullptr; // existing conn being reconnected
    QGraphicsPathItem* m_tempConnection = nullptr;

    void clearAlignmentLines();
    void updateAlignmentLines(const QList<QGraphicsItem*>& draggedItems);

    bool m_snapEnabled = true;
    bool m_alignmentEnabled = true;
    QList<QGraphicsLineItem*> m_alignmentLines;
    static constexpr qreal ALIGN_THRESHOLD = 5.0;
};
