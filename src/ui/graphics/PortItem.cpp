#include "PortItem.h"
#include "BlockItem.h"
#include "core/Types.h"

#include <QCursor>
#include <QPainter>
#include <QGraphicsSceneHoverEvent>
#include <QPen>

PortItem::PortItem(int portIndex, const QString& portId, PortDirection direction,
                   PortDataType dataType, BlockItem* parent)
    : QGraphicsEllipseItem(parent)
    , m_portIndex(portIndex)
    , m_portId(portId)
    , m_direction(direction)
    , m_dataType(dataType)
    , m_parentBlock(parent)
{
    const qreal r = BlockAppearance::PORT_RADIUS;
    setRect(-r, -r, 2 * r, 2 * r);
    setAcceptHoverEvents(true);
    setCursor(QCursor(Qt::CrossCursor));
    setZValue(2);

    setPen(QPen(BlockAppearance::portBorderColor(), 1.5));
    setBrush(BlockAppearance::portFillColor());
}

QPointF PortItem::centerInScene() const
{
    return mapToScene(0, 0);
}

void PortItem::setConnected(bool connected)
{
    m_isConnected = connected;
}

void PortItem::setHighlighted(bool on)
{
    m_highlighted = on;
    update();
}

void PortItem::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
    setHighlighted(true);
}

void PortItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    setHighlighted(false);
}

void PortItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);

    QColor fill;
    switch (m_dataType) {
    case PortDataType::Fluid:       fill = QColor("#1565C0"); break; // Blue
    case PortDataType::Mechanical:  fill = QColor("#E65100"); break; // Orange
    case PortDataType::Signal:      fill = QColor("#2E7D32"); break; // Green
    default:                        fill = QColor("#FFFFFF"); break;
    }
    QColor border = BlockAppearance::portBorderColor();

    if (m_highlighted || m_isConnected) {
        const qreal r = BlockAppearance::PORT_HOVER_RADIUS;
        painter->setPen(QPen(BlockAppearance::portHighlightColor(), 2.5));
        painter->setBrush(BlockAppearance::portHighlightColor().lighter(160));
        painter->drawEllipse(QPointF(0, 0), r, r);
    } else {
        painter->setPen(QPen(border, 1.5));
        painter->setBrush(fill);
        const qreal r = BlockAppearance::PORT_RADIUS;
        painter->drawEllipse(QPointF(0, 0), r, r);
    }
}
