#include "BlockScene.h"
#include "BlockItem.h"
#include "PortItem.h"
#include "ConnectionItem.h"
#include "components/ComponentDescriptor.h"
#include "components/ComponentFactory.h"
#include "components/ComponentInstance.h"
#include "core/Constants.h"
#include "ui/actions/UndoCommands.h"

#include <QDataStream>
#include <QGraphicsLineItem>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QPainter>
#include <QSet>
#include <QUndoStack>

BlockScene::BlockScene(ComponentFactory* factory, QObject* parent)
    : QGraphicsScene(parent)
    , m_factory(factory)
{
    setSceneRect(-5000, -5000, 10000, 10000);
}

// ─── Block Management ──────────────────────────────────────

BlockItem* BlockScene::addBlock(const ComponentDescriptor& descriptor, const QPointF& pos,
                                const QUuid& forcedUuid)
{
    auto inst = ComponentInstance::create(descriptor, snapToGrid(pos), forcedUuid);
    auto* block = new BlockItem(inst, descriptor);
    addItem(block);
    connectBlockSignals(block);
    emit blockAdded(block);
    emit sceneModified();
    return block;
}

void BlockScene::removeBlock(BlockItem* block)
{
    if (!block) return;

    // Remove all connections to/from this block
    const auto conns = connectionsForBlock(block);
    for (auto* conn : conns) {
        removeConnection(conn);
    }

    const QUuid id = block->uuid();
    removeItem(block);
    delete block;
    emit blockRemoved(id);
    emit sceneModified();
}

QList<BlockItem*> BlockScene::allBlocks() const
{
    QList<BlockItem*> result;
    for (auto* item : items()) {
        if (auto* block = qgraphicsitem_cast<BlockItem*>(item)) {
            result.append(block);
        }
    }
    return result;
}

BlockItem* BlockScene::blockByUuid(const QUuid& uuid) const
{
    for (auto* block : allBlocks()) {
        if (block->uuid() == uuid) return block;
    }
    return nullptr;
}

BlockItem* BlockScene::selectedBlock() const
{
    auto selected = selectedItems();
    if (selected.size() == 1) {
        return qgraphicsitem_cast<BlockItem*>(selected.first());
    }
    return nullptr;
}

QList<BlockItem*> BlockScene::selectedBlocks() const
{
    QList<BlockItem*> result;
    for (auto* item : selectedItems()) {
        if (auto* block = qgraphicsitem_cast<BlockItem*>(item)) {
            result.append(block);
        }
    }
    return result;
}

// ─── Connection Management ─────────────────────────────────

ConnectionItem* BlockScene::addConnection(PortItem* source, PortItem* dest)
{
    if (!canConnect(source, dest)) return nullptr;

    auto* conn = new ConnectionItem(source, dest);
    addItem(conn);

    // Set port tooltips showing connected component
    if (source->parentBlock() && dest->parentBlock()) {
        QString srcLabel = source->parentBlock()->customLabel();
        QString dstLabel = dest->parentBlock()->customLabel();
        if (srcLabel.isEmpty()) srcLabel = source->parentBlock()->typeId();
        if (dstLabel.isEmpty()) dstLabel = dest->parentBlock()->typeId();
        source->setToolTip(QObject::tr("%1 → %2").arg(srcLabel, dstLabel));
        dest->setToolTip(QObject::tr("%1 ← %2").arg(dstLabel, srcLabel));
    }

    emit connectionAdded(conn);
    emit sceneModified();
    return conn;
}

void BlockScene::removeConnection(ConnectionItem* conn)
{
    if (!conn) return;

    if (conn->sourcePort()) conn->sourcePort()->setConnection(nullptr);
    if (conn->destPort())   conn->destPort()->setConnection(nullptr);

    removeItem(conn);
    delete conn;
    emit connectionRemoved();
    emit sceneModified();
}

void BlockScene::deleteSelectedConnections()
{
    QList<ConnectionItem*> selectedConns;
    for (auto* item : selectedItems()) {
        if (auto* conn = qgraphicsitem_cast<ConnectionItem*>(item))
            selectedConns.append(conn);
    }

    if (selectedConns.isEmpty()) return;

    if (m_undoStack) {
        if (selectedConns.size() == 1) {
            auto* conn = selectedConns.first();
            auto* sp = conn->sourcePort();
            auto* dp = conn->destPort();
            if (sp && dp && sp->parentBlock() && dp->parentBlock()) {
                m_undoStack->push(new RemoveConnectionCommand(
                    this,
                    sp->parentBlock()->uuid(), sp->portId(),
                    dp->parentBlock()->uuid(), dp->portId()));
            }
        } else {
            m_undoStack->beginMacro(tr("Delete connections"));
            for (auto* conn : selectedConns) {
                auto* sp = conn->sourcePort();
                auto* dp = conn->destPort();
                if (sp && dp && sp->parentBlock() && dp->parentBlock()) {
                    m_undoStack->push(new RemoveConnectionCommand(
                        this,
                        sp->parentBlock()->uuid(), sp->portId(),
                        dp->parentBlock()->uuid(), dp->portId()));
                }
            }
            m_undoStack->endMacro();
        }
    } else {
        for (auto* conn : selectedConns)
            removeConnection(conn);
    }
}

bool BlockScene::canConnect(PortItem* source, PortItem* dest) const
{
    if (!source || !dest) return false;
    if (source == dest) return false;

    // Source must be output, dest must be input
    if (source->direction() != PortDirection::Output ||
        dest->direction()   != PortDirection::Input) {
        return false;
    }

    // Cannot connect to the same block
    if (source->parentBlock() == dest->parentBlock()) return false;

    // Each port can have at most one connection
    if (source->isConnected() || dest->isConnected()) return false;

    return true;
}

QList<ConnectionItem*> BlockScene::allConnections() const
{
    QList<ConnectionItem*> result;
    for (auto* item : items()) {
        if (auto* conn = qgraphicsitem_cast<ConnectionItem*>(item)) {
            result.append(conn);
        }
    }
    return result;
}

QList<ConnectionItem*> BlockScene::connectionsForPort(PortItem* port) const
{
    QList<ConnectionItem*> result;
    if (port && port->connection())
        result.append(port->connection());
    return result;
}

QList<ConnectionItem*> BlockScene::connectionsForBlock(BlockItem* block) const
{
    QList<ConnectionItem*> result;
    if (!block) return result;
    for (auto* port : block->allPorts()) {
        if (auto* conn = port->connection()) {
            if (!result.contains(conn))
                result.append(conn);
        }
    }
    return result;
}

PortItem* BlockScene::portAtPos(const QPointF& scenePos) const
{
    QList<QGraphicsItem*> itemsAtPos = items(scenePos, Qt::IntersectsItemBoundingRect,
                                              Qt::DescendingOrder);
    for (auto* item : itemsAtPos) {
        if (auto* port = qgraphicsitem_cast<PortItem*>(item)) {
            // Check actual hit
            if (port->contains(port->mapFromScene(scenePos))) {
                return port;
            }
        }
    }
    return nullptr;
}

// ─── Mouse Handling (Connection Drawing) ────────────────────

void BlockScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        PortItem* port = portAtPos(event->scenePos());
        if (port && port->direction() == PortDirection::Output) {
            m_drawingConnection = true;
            m_connectionSource = port;
            // If port already has a connection, remember it for reconnect
            m_reconnectConnection = port->connection();
            m_tempConnection = new QGraphicsPathItem();
            m_tempConnection->setPen(QPen(BlockAppearance::tempConnectionColor(),
                                          BlockAppearance::CONNECTION_WIDTH,
                                          Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
            m_tempConnection->setZValue(0);
            addItem(m_tempConnection);
            event->accept();
            return;
        }

        // Multi-select with Shift/Ctrl modifier on BlockItem click
        bool hasMod = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier);
        if (hasMod) {
            QList<QGraphicsItem*> itemsAt = items(event->scenePos(), Qt::IntersectsItemShape,
                                                   Qt::DescendingOrder);
            BlockItem* clickedBlock = nullptr;
            for (auto* item : itemsAt) {
                if ((clickedBlock = qgraphicsitem_cast<BlockItem*>(item)))
                    break;
            }

            if (clickedBlock) {
                if (event->modifiers() & Qt::ControlModifier) {
                    clickedBlock->setSelected(!clickedBlock->isSelected());
                } else {
                    clickedBlock->setSelected(true);
                }
                emit blockSelectionChanged(selectedBlock());
                emit multiSelectionChanged();
                event->accept();
                return;
            }
        }
    }
    QGraphicsScene::mousePressEvent(event);
}

void BlockScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_drawingConnection && m_tempConnection) {
        QPainterPath path;
        QPointF src = m_connectionSource->centerInScene();
        QPointF dst = event->scenePos();
        path.moveTo(src);

        qreal dx = qAbs(dst.x() - src.x()) * 0.5;
        dx = qMax(dx, 50.0);
        path.cubicTo(QPointF(src.x() + dx, src.y()),
                     QPointF(dst.x() - dx, dst.y()), dst);

        m_tempConnection->setPath(path);

        // Highlight valid target ports
        PortItem* target = portAtPos(dst);
        for (auto* item : items()) {
            if (auto* p = qgraphicsitem_cast<PortItem*>(item)) {
                p->setHighlighted(p == target && canConnect(m_connectionSource, p));
            }
        }
        event->accept();
        return;
    }

    // During block drag, show alignment guide lines
    if (m_alignmentEnabled) {
        auto sel = selectedItems();
        bool draggingBlock = false;
        for (auto* item : sel) {
            if (qgraphicsitem_cast<BlockItem*>(item)) {
                draggingBlock = true;
                break;
            }
        }
        if (draggingBlock)
            updateAlignmentLines(sel);
        else
            clearAlignmentLines();
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void BlockScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    // Clear alignment guide lines on any mouse release
    clearAlignmentLines();

    if (m_drawingConnection) {
        m_drawingConnection = false;

        // Clean up temp line
        if (m_tempConnection) {
            removeItem(m_tempConnection);
            delete m_tempConnection;
            m_tempConnection = nullptr;
        }

        // Clear port highlights
        for (auto* item : items()) {
            if (auto* p = qgraphicsitem_cast<PortItem*>(item)) {
                p->setHighlighted(false);
            }
        }

        // Try to connect
        if (m_connectionSource) {
            PortItem* target = portAtPos(event->scenePos());
            bool valid = target && target->direction() == PortDirection::Input
                        && m_connectionSource != target
                        && m_connectionSource->parentBlock() != target->parentBlock();

            if (valid) {
                if (m_undoStack) {
                    m_undoStack->beginMacro(tr("Reconnect"));
                    // Remove old connection from source port (reconnect case)
                    if (m_reconnectConnection) {
                        auto* rc = m_reconnectConnection;
                        m_undoStack->push(new RemoveConnectionCommand(
                            this,
                            rc->sourcePort()->parentBlock()->uuid(), rc->sourcePort()->portId(),
                            rc->destPort()->parentBlock()->uuid(), rc->destPort()->portId()));
                    }
                    // Remove old connection from target port if occupied
                    if (auto* oldConn = target->connection()) {
                        m_undoStack->push(new RemoveConnectionCommand(
                            this,
                            oldConn->sourcePort()->parentBlock()->uuid(), oldConn->sourcePort()->portId(),
                            oldConn->destPort()->parentBlock()->uuid(), oldConn->destPort()->portId()));
                    }
                    m_undoStack->push(new AddConnectionCommand(
                        this,
                        m_connectionSource->parentBlock()->uuid(), m_connectionSource->portId(),
                        target->parentBlock()->uuid(), target->portId()));
                    m_undoStack->endMacro();
                } else {
                    if (m_reconnectConnection)
                        removeConnection(m_reconnectConnection);
                    if (auto* oldConn = target->connection())
                        removeConnection(oldConn);
                    addConnection(m_connectionSource, target);
                }
            }
            m_connectionSource = nullptr;
            m_reconnectConnection = nullptr;
        }

        event->accept();
        return;
    }
    QGraphicsScene::mouseReleaseEvent(event);

    // Selection change signals
    auto blocks = selectedBlocks();
    if (blocks.size() == 1)
        emit blockSelectionChanged(blocks.first());
    else
        emit blockSelectionChanged(nullptr);
    emit multiSelectionChanged();
}

// ─── Drag-and-Drop from Library ────────────────────────────

void BlockScene::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    if (event->mimeData()->hasFormat(AppConstants::MIME_COMPONENT_TYPE)) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void BlockScene::dragMoveEvent(QGraphicsSceneDragDropEvent* event)
{
    if (event->mimeData()->hasFormat(AppConstants::MIME_COMPONENT_TYPE)) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void BlockScene::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    if (!event->mimeData()->hasFormat(AppConstants::MIME_COMPONENT_TYPE)) {
        event->ignore();
        return;
    }

    QByteArray data = event->mimeData()->data(AppConstants::MIME_COMPONENT_TYPE);
    QDataStream stream(&data, QIODevice::ReadOnly);
    QString typeId;
    stream >> typeId;

    const auto* desc = m_factory->descriptorForType(typeId);
    if (desc) {
        addBlock(*desc, event->scenePos());
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

// ─── Grid ──────────────────────────────────────────────────

QPointF BlockScene::snapToGrid(const QPointF& pt) const
{
    if (!m_snapEnabled) return pt;
    const qreal g = BlockAppearance::GRID_SIZE;
    return QPointF(qRound(pt.x() / g) * g, qRound(pt.y() / g) * g);
}

// ─── Signal forwarding ─────────────────────────────────────

void BlockScene::connectBlockSignals(BlockItem* block)
{
    connect(block, &BlockItem::positionChanged, this, [this, block](const QUuid&, const QPointF&) {
        for (auto* port : block->allPorts()) {
            if (auto* conn = port->connection())
                conn->updatePath();
        }
        emit sceneModified();
    });

    connect(block, &BlockItem::propertyChanged, this, [this]() {
        emit sceneModified();
    });
}

// ─── Selection ─────────────────────────────────────────────

// BlockScene overrides mouseReleaseEvent to emit blockSelectionChanged above

// ─── Alignment Guide Lines ─────────────────────────────────

void BlockScene::clearAlignmentLines()
{
    for (auto* line : m_alignmentLines) {
        removeItem(line);
        delete line;
    }
    m_alignmentLines.clear();
}

void BlockScene::updateAlignmentLines(const QList<QGraphicsItem*>& draggedItems)
{
    clearAlignmentLines();

    // Collect dragged block centers/edges
    struct Rect { qreal left, top, right, bottom, cx, cy; };
    QList<Rect> dragged;
    QSet<BlockItem*> draggedSet;
    for (auto* item : draggedItems) {
        if (auto* block = qgraphicsitem_cast<BlockItem*>(item)) {
            auto br = block->sceneBoundingRect();
            dragged.append({br.left(), br.top(), br.right(), br.bottom(),
                           br.center().x(), br.center().y()});
            draggedSet.insert(block);
        }
    }
    if (dragged.isEmpty()) return;

    // Collect non-dragged block centers/edges
    QList<Rect> anchors;
    for (auto* block : allBlocks()) {
        if (draggedSet.contains(block)) continue;
        auto br = block->sceneBoundingRect();
        anchors.append({br.left(), br.top(), br.right(), br.bottom(),
                       br.center().x(), br.center().y()});
    }

    // Check alignment for each dragged block against anchors
    QPen guidePen(QColor("#42A5F5"), 1.0, Qt::DashLine);
    QRectF sceneRect = this->sceneRect();

    for (const auto& d : dragged) {
        for (const auto& a : anchors) {
            // Horizontal alignment: center X
            if (qAbs(d.cx - a.cx) < ALIGN_THRESHOLD) {
                auto* line = new QGraphicsLineItem(a.cx, sceneRect.top(), a.cx, sceneRect.bottom());
                line->setPen(guidePen);
                line->setZValue(100);
                addItem(line);
                m_alignmentLines.append(line);
            }
            // Horizontal alignment: left edges
            if (qAbs(d.left - a.left) < ALIGN_THRESHOLD) {
                auto* line = new QGraphicsLineItem(a.left, sceneRect.top(), a.left, sceneRect.bottom());
                line->setPen(guidePen);
                line->setZValue(100);
                addItem(line);
                m_alignmentLines.append(line);
            }
            // Horizontal alignment: right edges
            if (qAbs(d.right - a.right) < ALIGN_THRESHOLD) {
                auto* line = new QGraphicsLineItem(a.right, sceneRect.top(), a.right, sceneRect.bottom());
                line->setPen(guidePen);
                line->setZValue(100);
                addItem(line);
                m_alignmentLines.append(line);
            }
            // Vertical alignment: center Y
            if (qAbs(d.cy - a.cy) < ALIGN_THRESHOLD) {
                auto* line = new QGraphicsLineItem(sceneRect.left(), a.cy, sceneRect.right(), a.cy);
                line->setPen(guidePen);
                line->setZValue(100);
                addItem(line);
                m_alignmentLines.append(line);
            }
            // Vertical alignment: top edges
            if (qAbs(d.top - a.top) < ALIGN_THRESHOLD) {
                auto* line = new QGraphicsLineItem(sceneRect.left(), a.top, sceneRect.right(), a.top);
                line->setPen(guidePen);
                line->setZValue(100);
                addItem(line);
                m_alignmentLines.append(line);
            }
            // Vertical alignment: bottom edges
            if (qAbs(d.bottom - a.bottom) < ALIGN_THRESHOLD) {
                auto* line = new QGraphicsLineItem(sceneRect.left(), a.bottom, sceneRect.right(), a.bottom);
                line->setPen(guidePen);
                line->setZValue(100);
                addItem(line);
                m_alignmentLines.append(line);
            }
        }
    }
}

// ─── Serialization ─────────────────────────────────────────

QJsonObject BlockScene::toJson() const
{
    QJsonObject root;
    root["version"] = 1;

    QJsonArray blocksArray;
    for (auto* block : allBlocks()) {
        QJsonObject b;
        QPointF p = block->pos();
        b["uuid"]     = block->uuid().toString();
        b["typeId"]   = block->typeId();
        b["position"] = QJsonObject{{"x", p.x()}, {"y", p.y()}};
        b["label"]    = block->customLabel();

        QJsonObject props;
        const auto desc = block->descriptor();
        for (const auto& prop : desc.properties) {
            props[prop.id] = QJsonValue::fromVariant(block->propertyValue(prop.id));
        }
        b["properties"] = props;
        blocksArray.append(b);
    }
    root["blocks"] = blocksArray;

    QJsonArray connsArray;
    for (auto* conn : allConnections()) {
        QJsonObject c;
        if (conn->sourcePort() && conn->sourcePort()->parentBlock()) {
            c["sourceBlockUuid"] = conn->sourcePort()->parentBlock()->uuid().toString();
            c["sourcePortId"]    = conn->sourcePort()->portId();
        }
        if (conn->destPort() && conn->destPort()->parentBlock()) {
            c["destBlockUuid"] = conn->destPort()->parentBlock()->uuid().toString();
            c["destPortId"]    = conn->destPort()->portId();
        }
        connsArray.append(c);
    }
    root["connections"] = connsArray;

    return root;
}

void BlockScene::fromJson(const QJsonObject& json)
{
    clearScene();

    QMap<QString, BlockItem*> uuidMap;

    // Restore blocks
    const QJsonArray blocksArray = json["blocks"].toArray();
    for (const auto& val : blocksArray) {
        QJsonObject b = val.toObject();
        QString typeId = b["typeId"].toString();
        const auto* desc = m_factory->descriptorForType(typeId);
        if (!desc) continue;

        QJsonObject posObj = b["position"].toObject();
        QPointF pos(posObj["x"].toDouble(), posObj["y"].toDouble());

        auto inst = ComponentInstance::create(*desc, pos);
        inst.uuid = QUuid::fromString(b["uuid"].toString());
        inst.customLabel = b["label"].toString(desc->displayName);

        // Restore property values
        QJsonObject props = b["properties"].toObject();
        for (auto it = props.begin(); it != props.end(); ++it) {
            inst.propertyValues[it.key()] = it.value().toVariant();
        }

        auto* block = new BlockItem(inst, *desc);
        addItem(block);
        connectBlockSignals(block);
        uuidMap[block->uuid().toString()] = block;
    }

    // Restore connections
    const QJsonArray connsArray = json["connections"].toArray();
    for (const auto& val : connsArray) {
        QJsonObject c = val.toObject();
        auto* srcBlock = uuidMap.value(c["sourceBlockUuid"].toString());
        auto* dstBlock = uuidMap.value(c["destBlockUuid"].toString());
        if (srcBlock && dstBlock) {
            auto* srcPort = srcBlock->portById(c["sourcePortId"].toString());
            auto* dstPort = dstBlock->portById(c["destPortId"].toString());
            if (srcPort && dstPort) {
                addConnection(srcPort, dstPort);
            }
        }
    }

    emit sceneModified();
}

void BlockScene::clearScene()
{
    // Qt's clear() removes and deletes all items safely
    clear();
    m_tempConnection = nullptr;
    m_connectionSource = nullptr;
    m_reconnectConnection = nullptr;
    m_drawingConnection = false;
    emit sceneModified();
}
