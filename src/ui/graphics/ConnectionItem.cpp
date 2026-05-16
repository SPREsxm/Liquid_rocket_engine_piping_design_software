#include "ConnectionItem.h"
#include "PortItem.h"
#include "BlockItem.h"
#include "BlockScene.h"

#include <QAction>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QToolTip>
#include <QtMath>

ConnectionItem::ConnectionItem(PortItem* sourcePort, PortItem* destPort,
                               QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
    , m_sourcePort(sourcePort)
    , m_destPort(destPort)
{
    setPen(QPen(BlockAppearance::connectionColor(),
                BlockAppearance::CONNECTION_WIDTH,
                Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setZValue(0);
    setAcceptHoverEvents(true);
    setFlag(ItemIsSelectable, true);
    setCursor(QCursor(Qt::PointingHandCursor));

    if (m_sourcePort) m_sourcePort->setConnection(this);
    if (m_destPort)   m_destPort->setConnection(this);

    updatePath();
}

void ConnectionItem::setSourcePort(PortItem* port)
{
    if (m_sourcePort) m_sourcePort->setConnection(nullptr);
    m_sourcePort = port;
    if (m_sourcePort) m_sourcePort->setConnection(this);
    updatePath();
}

void ConnectionItem::setDestPort(PortItem* port)
{
    if (m_destPort) m_destPort->setConnection(nullptr);
    m_destPort = port;
    if (m_destPort) m_destPort->setConnection(this);
    updatePath();
}

void ConnectionItem::updatePath()
{
    if (!m_sourcePort || !m_destPort) return;

    QPointF src = m_sourcePort->centerInScene();
    QPointF dst = m_destPort->centerInScene();

    QPainterPath p;
    p.moveTo(src);

    qreal dx = qAbs(dst.x() - src.x()) * 0.5;
    dx = qMax(dx, 50.0);

    QPointF ctrl1(src.x() + dx, src.y());
    QPointF ctrl2(dst.x() - dx, dst.y());

    p.cubicTo(ctrl1, ctrl2, dst);
    setPath(p);
}

void ConnectionItem::setFlowData(double flowRate, double maxFlowRate)
{
    m_flowRate = flowRate;
    m_maxFlowRate = maxFlowRate;
    update();
}

void ConnectionItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);

    QColor color = BlockAppearance::connectionColor();
    qreal width = BlockAppearance::CONNECTION_WIDTH;

    // Flow-based coloring
    if (m_maxFlowRate > 0.0) {
        double ratio = qBound(0.0, m_flowRate / m_maxFlowRate, 1.0);
        color = BlockAppearance::flowColor(ratio);
        width = BlockAppearance::flowWidth(ratio);
    }

    if (isSelected()) {
        color = BlockAppearance::selectedBorderColor();
        width = qMax(width, 3.0);
    }

    QPainterPath p = path();
    painter->setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawPath(p);

    // Arrow head at destination
    if (!m_destPort) return;

    QPointF dst = m_destPort->centerInScene();
    // Use simple linear interpolation on the last segment for the arrow tangent
    // instead of expensive percentAtLength/pointAtPercent on the whole cubic path.
    qreal pathLen = p.length();
    qreal t = (pathLen > 0.001) ? (1.0 - (BlockAppearance::PORT_RADIUS + 4.0) / pathLen) : 0.0;
    t = qBound(0.0, t, 1.0);
    QPointF nearTip = p.pointAtPercent(t);
    QPointF tangent  = (t > 0.01) ? p.pointAtPercent(t - 0.02) : p.pointAtPercent(0.0);

    QPointF dir = nearTip - tangent;
    qreal len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (len < 0.001) return;
    dir /= len;

    qreal arrowSize = 8.0;
    QPointF normal(-dir.y(), dir.x());
    QPointF base = dst - dir * arrowSize * 1.2;

    QPolygonF arrowHead;
    arrowHead << dst;
    arrowHead << base + normal * arrowSize * 0.5;
    arrowHead << base - normal * arrowSize * 0.5;

    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawPolygon(arrowHead);
}

void ConnectionItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    setSelected(true);
    QMenu menu;
    auto* deleteAction = menu.addAction(QObject::tr("Delete Connection"));

    QAction* chosen = menu.exec(event->screenPos());
    if (chosen == deleteAction) {
        // Delegate to scene so it can use the undo stack
        if (auto* bs = qobject_cast<BlockScene*>(scene())) {
            bs->deleteSelectedConnections();
        } else if (auto* s = scene()) {
            // Fallback for non-BlockScene (should not happen in production)
            if (m_sourcePort) m_sourcePort->setConnection(nullptr);
            if (m_destPort)   m_destPort->setConnection(nullptr);
            s->removeItem(this);
            delete this;
        }
    }
}

void ConnectionItem::setAnalysisTooltip(double pressureDrop, double flowRate)
{
    m_hasFlowData = true;
    m_tooltipPressureDrop = pressureDrop;
    m_tooltipFlowRate = flowRate;
}

void ConnectionItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    if (m_hasFlowData) {
        QString tip = QStringLiteral("Flow: %1 kg/s\nΔP: %2 Pa")
            .arg(m_tooltipFlowRate, 0, 'f', 4)
            .arg(m_tooltipPressureDrop, 0, 'e', 3);
        QToolTip::showText(event->screenPos(), tip);
    }
}

void ConnectionItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (m_hasFlowData) {
        QString tip = QStringLiteral("Flow: %1 kg/s\nΔP: %2 Pa")
            .arg(m_tooltipFlowRate, 0, 'f', 4)
            .arg(m_tooltipPressureDrop, 0, 'e', 3);
        QToolTip::showText(event->screenPos(), tip);
    }
}

void ConnectionItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* /*event*/)
{
    QToolTip::hideText();
}
