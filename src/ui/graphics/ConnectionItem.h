#pragma once

#include "BlockAppearance.h"

#include <QGraphicsPathItem>
#include <QGraphicsSceneContextMenuEvent>

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
    void updatePath();
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    PortItem* m_sourcePort;
    PortItem* m_destPort;
    double m_flowRate = 0.0;
    double m_maxFlowRate = 0.0;
};
