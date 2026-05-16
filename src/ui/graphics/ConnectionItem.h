#pragma once

#include "BlockAppearance.h"

#include <QGraphicsPathItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>

class PortItem;

class ConnectionItem : public QGraphicsPathItem {
public:
    enum { Type = QGraphicsItem::UserType + 12 };

    ConnectionItem(PortItem* sourcePort, PortItem* destPort,
                   QGraphicsItem* parent = nullptr);

    int type() const override { return Type; }

    PortItem* sourcePort() const { return m_sourcePort; }
    PortItem* destPort() const   { return m_destPort; }

    void setFlowData(double flowRate, double maxFlowRate);
    void setAnalysisTooltip(double pressureDrop, double flowRate);

    // Reconnect support
    void setSourcePort(PortItem* port);
    void setDestPort(PortItem* port);

    void updatePath();
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    PortItem* m_sourcePort;
    PortItem* m_destPort;
    double m_flowRate = 0.0;
    double m_maxFlowRate = 0.0;

    // Tooltip data
    bool m_hasFlowData = false;
    double m_tooltipFlowRate = 0.0;
    double m_tooltipPressureDrop = 0.0;
};
