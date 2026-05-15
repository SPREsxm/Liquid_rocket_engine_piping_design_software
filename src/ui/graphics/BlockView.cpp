#include "BlockView.h"
#include "BlockScene.h"
#include "BlockItem.h"
#include "PortItem.h"
#include "ConnectionItem.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"
#include "core/Constants.h"

#include <QApplication>
#include <QClipboard>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSet>
#include <QWheelEvent>
#include <QtMath>

BlockView::BlockView(BlockScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_scene(scene)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    setMinimumSize(400, 300);
}

void BlockView::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    viewport()->update();
}

bool BlockView::isGridVisible() const
{
    return m_gridVisible;
}

void BlockView::zoomIn()
{
    applyZoom(AppConstants::ZOOM_STEP, viewport()->rect().center());
}

void BlockView::zoomOut()
{
    applyZoom(1.0 / AppConstants::ZOOM_STEP, viewport()->rect().center());
}

void BlockView::zoomToFit()
{
    const auto blocks = m_scene->allBlocks();
    if (blocks.isEmpty()) {
        resetTransform();
        m_currentZoom = 1.0;
        emit zoomChanged(m_currentZoom);
        return;
    }

    QRectF bounds;
    for (auto* block : blocks) {
        bounds = bounds.united(block->sceneBoundingRect());
    }
    bounds.adjust(-50, -50, 50, 50);

    fitInView(bounds, Qt::KeepAspectRatio);

    // Recalculate current zoom
    QTransform t = transform();
    m_currentZoom = t.m11();
    m_currentZoom = qBound(AppConstants::MIN_ZOOM, m_currentZoom, AppConstants::MAX_ZOOM);
    emit zoomChanged(m_currentZoom);
}

void BlockView::deleteSelected()
{
    auto selected = scene()->selectedItems();

    for (auto* item : selected) {
        if (auto* conn = qgraphicsitem_cast<ConnectionItem*>(item)) {
            m_scene->removeConnection(conn);
        }
    }
    for (auto* item : selected) {
        if (auto* block = qgraphicsitem_cast<BlockItem*>(item)) {
            m_scene->removeBlock(block);
        }
    }
}

void BlockView::copySelected()
{
    auto selected = scene()->selectedItems();

    QJsonArray blocksArr;
    QSet<QUuid> selectedUuids;

    for (auto* item : selected) {
        if (auto* block = qgraphicsitem_cast<BlockItem*>(item)) {
            QJsonObject b;
            b["uuid"]   = block->uuid().toString();
            b["typeId"] = block->typeId();
            b["x"]      = block->pos().x();
            b["y"]      = block->pos().y();
            b["label"]  = block->customLabel();

            QJsonObject props;
            for (const auto& pd : block->descriptor().properties) {
                props[pd.id] = QJsonValue::fromVariant(block->propertyValue(pd.id));
            }
            b["properties"] = props;
            blocksArr.append(b);
            selectedUuids.insert(block->uuid());
        }
    }

    // Also include connections where both endpoints are in the selected set
    QJsonArray connsArr;
    for (auto* conn : m_scene->allConnections()) {
        auto* sp = conn->sourcePort();
        auto* dp = conn->destPort();
        if (sp && dp && selectedUuids.contains(sp->parentBlock()->uuid())
            && selectedUuids.contains(dp->parentBlock()->uuid())) {
            QJsonObject c;
            c["srcUuid"] = sp->parentBlock()->uuid().toString();
            c["srcPort"] = sp->portId();
            c["dstUuid"] = dp->parentBlock()->uuid().toString();
            c["dstPort"] = dp->portId();
            connsArr.append(c);
        }
    }

    QJsonObject doc;
    doc["blocks"] = blocksArr;
    doc["connections"] = connsArr;

    auto* mime = new QMimeData;
    mime->setData(AppConstants::MIME_BLOCK_CLIPBOARD,
                  QJsonDocument(doc).toJson(QJsonDocument::Compact));
    QApplication::clipboard()->setMimeData(mime);
}

void BlockView::cutSelected()
{
    copySelected();
    deleteSelected();
}

void BlockView::pasteClipboard()
{
    const QMimeData* mime = QApplication::clipboard()->mimeData();
    if (!mime || !mime->hasFormat(AppConstants::MIME_BLOCK_CLIPBOARD)) return;

    QJsonDocument doc = QJsonDocument::fromJson(
        mime->data(AppConstants::MIME_BLOCK_CLIPBOARD));
    if (!doc.isObject()) return;

    ComponentFactory* cf = m_scene->factory();

    // Clear current selection
    for (auto* item : scene()->selectedItems())
        item->setSelected(false);

    QJsonObject root = doc.object();
    QJsonArray blocksArr = root["blocks"].toArray();

    // Offset for paste
    const QPointF offset(30, 30);

    // Build UUID mapping (old → new) since blocks get new UUIDs on creation
    QHash<QString, QUuid> uuidMap;

    for (int i = 0; i < blocksArr.size(); ++i) {
        QJsonObject b = blocksArr[i].toObject();
        QString typeId = b["typeId"].toString();
        const ComponentDescriptor* desc = cf->descriptorForType(typeId);
        if (!desc) continue;

        QPointF pos(b["x"].toDouble() + offset.x(),
                    b["y"].toDouble() + offset.y());

        BlockItem* block = m_scene->addBlock(*desc, pos);
        if (!block) continue;

        QString oldUuid = b["uuid"].toString();
        uuidMap[oldUuid] = block->uuid();

        block->setCustomLabel(b["label"].toString(desc->displayName));

        QJsonObject props = b["properties"].toObject();
        for (auto it = props.begin(); it != props.end(); ++it) {
            block->setPropertyValue(it.key(), it.value().toVariant());
        }

        block->setSelected(true);
    }

    // Restore internal connections
    QJsonArray connsArr = root["connections"].toArray();
    for (int i = 0; i < connsArr.size(); ++i) {
        QJsonObject c = connsArr[i].toObject();
        QUuid newSrc = uuidMap.value(c["srcUuid"].toString());
        QUuid newDst = uuidMap.value(c["dstUuid"].toString());
        if (newSrc.isNull() || newDst.isNull()) continue;

        BlockItem* srcBlock = m_scene->blockByUuid(newSrc);
        BlockItem* dstBlock = m_scene->blockByUuid(newDst);
        if (!srcBlock || !dstBlock) continue;

        PortItem* srcPort = srcBlock->portById(c["srcPort"].toString());
        PortItem* dstPort = dstBlock->portById(c["dstPort"].toString());
        if (srcPort && dstPort) {
            m_scene->addConnection(srcPort, dstPort);
        }
    }
}

void BlockView::applyZoom(double factor, QPointF centerPoint)
{
    double newZoom = m_currentZoom * factor;
    newZoom = qBound(AppConstants::MIN_ZOOM, newZoom, AppConstants::MAX_ZOOM);
    double actualFactor = newZoom / m_currentZoom;
    m_currentZoom = newZoom;

    // Zoom centered on the given point
    QPointF scenePt = mapToScene(centerPoint.toPoint());
    scale(actualFactor, actualFactor);
    centerOn(scenePt);

    emit zoomChanged(m_currentZoom);
}

void BlockView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void BlockView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        QPointF delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void BlockView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void BlockView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = (event->angleDelta().y() > 0)
            ? AppConstants::ZOOM_STEP
            : 1.0 / AppConstants::ZOOM_STEP;
        applyZoom(factor, event->position());
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void BlockView::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
        deleteSelected();
        break;
    case Qt::Key_A:
        if (event->modifiers() & Qt::ControlModifier) {
            for (auto* item : scene()->items()) {
                if (qgraphicsitem_cast<BlockItem*>(item) ||
                    qgraphicsitem_cast<ConnectionItem*>(item)) {
                    item->setSelected(true);
                }
            }
            event->accept();
            return;
        }
        QGraphicsView::keyPressEvent(event);
        break;
    case Qt::Key_C:
        if (event->modifiers() & Qt::ControlModifier) {
            copySelected();
            event->accept();
            return;
        }
        QGraphicsView::keyPressEvent(event);
        break;
    case Qt::Key_X:
        if (event->modifiers() & Qt::ControlModifier) {
            cutSelected();
            event->accept();
            return;
        }
        QGraphicsView::keyPressEvent(event);
        break;
    case Qt::Key_V:
        if (event->modifiers() & Qt::ControlModifier) {
            pasteClipboard();
            event->accept();
            return;
        }
        QGraphicsView::keyPressEvent(event);
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down: {
        // Nudge selected items
        QPointF delta;
        if (event->key() == Qt::Key_Left)  delta = QPointF(-1, 0);
        if (event->key() == Qt::Key_Right) delta = QPointF(1, 0);
        if (event->key() == Qt::Key_Up)    delta = QPointF(0, -1);
        if (event->key() == Qt::Key_Down)  delta = QPointF(0, 1);

        qreal step = BlockAppearance::GRID_SIZE;
        if (event->modifiers() & Qt::ShiftModifier) step = 1.0;

        for (auto* item : scene()->selectedItems()) {
            if (qgraphicsitem_cast<BlockItem*>(item)) {
                item->moveBy(delta.x() * step, delta.y() * step);
            }
        }
        event->accept();
        return;
    }
    default:
        QGraphicsView::keyPressEvent(event);
    }
}

void BlockView::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawBackground(painter, rect);

    if (!m_gridVisible) return;

    painter->setPen(QPen(QColor("#E0E0E0"), 0.5));

    const qreal g = BlockAppearance::GRID_SIZE;
    qreal left = qFloor(rect.left() / g) * g;
    qreal top  = qFloor(rect.top()  / g) * g;

    for (qreal x = left; x < rect.right(); x += g) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    for (qreal y = top; y < rect.bottom(); y += g) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
}
