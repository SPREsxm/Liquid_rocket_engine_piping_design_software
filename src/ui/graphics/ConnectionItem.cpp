#include "ConnectionItem.h"
#include "PortItem.h"
#include "BlockItem.h"

#include <QAction>
#include <QCursor>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QPen>
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

    if (m_sourcePort) m_sourcePort->setConnected(true);
    if (m_destPort)   m_destPort->setConnected(true);

    updatePath();
}

void ConnectionItem::updatePath()
{
    if (!m_sourcePort || !m_destPort) return;

    QPointF src = m_sourcePort->centerInScene();
    QPointF dst = m_destPort->centerInScene();

    QPainterPath path;
    path.moveTo(src);

    qreal dx = qAbs(dst.x() - src.x()) * 0.5;
    dx = qMax(dx, 50.0);

    QPointF ctrl1(src.x() + dx, src.y());
    QPointF ctrl2(dst.x() - dx, dst.y());

    path.cubicTo(ctrl1, ctrl2, dst);
    setPath(path);
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

    painter->setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawPath(path());

    // Arrow head at destination
    if (!m_destPort) return;

    QPointF dst = m_destPort->centerInScene();
    QPainterPath pathData = path();
    qreal t = pathData.percentAtLength(pathData.length() - BlockAppearance::PORT_RADIUS - 4);
    QPointF tip = dst;
    QPointF tangent = pathData.pointAtPercent(qMax(0.0, t - 0.02)); // point slightly before tip

    QPointF dir = tip - tangent;
    qreal len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y());
    if (len < 0.001) return;
    dir /= len;

    qreal arrowSize = 8.0;
    QPointF normal(-dir.y(), dir.x());
    QPointF base = tip - dir * arrowSize * 1.2;

    QPolygonF arrowHead;
    arrowHead << tip;
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
        if (m_sourcePort) m_sourcePort->setConnected(false);
        if (m_destPort)   m_destPort->setConnected(false);
        if (auto* s = scene()) {
            s->removeItem(this);
            delete this;
        }
    }
}
