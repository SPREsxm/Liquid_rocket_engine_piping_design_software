#pragma once

#include "BlockAppearance.h"
#include "core/Types.h"

#include <QGraphicsEllipseItem>
#include <QString>

class BlockItem;

class PortItem : public QGraphicsEllipseItem {
public:
    enum { Type = QGraphicsItem::UserType + 10 };

    PortItem(int portIndex, const QString& portId, PortDirection direction,
             PortDataType dataType, BlockItem* parent);

    int type() const override { return Type; }

    int portIndex() const { return m_portIndex; }
    QString portId() const { return m_portId; }
    PortDirection direction() const { return m_direction; }
    PortDataType dataType() const { return m_dataType; }
    BlockItem* parentBlock() const { return m_parentBlock; }
    QPointF centerInScene() const;

    bool isConnected() const { return m_isConnected; }
    void setConnected(bool connected);

    void setHighlighted(bool on);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
    int m_portIndex;
    QString m_portId;
    PortDirection m_direction;
    PortDataType m_dataType;
    BlockItem* m_parentBlock;
    bool m_isConnected = false;
    bool m_highlighted = false;
};
