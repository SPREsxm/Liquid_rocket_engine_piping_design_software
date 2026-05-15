#pragma once

#include "BlockAppearance.h"
#include "components/ComponentDescriptor.h"
#include "components/ComponentInstance.h"
#include "core/Types.h"

#include <QGraphicsObject>
#include <QGraphicsSceneContextMenuEvent>
#include <QUuid>
#include <QMap>
#include <QString>

class PortItem;

class BlockItem : public QGraphicsObject {
    Q_OBJECT
public:
    enum { Type = QGraphicsItem::UserType + 11 };

    BlockItem(const ComponentInstance& instance,
              const ComponentDescriptor& descriptor,
              QGraphicsItem* parent = nullptr);

    int type() const override { return Type; }

    QUuid uuid() const { return m_uuid; }
    QString typeId() const { return m_descriptor.typeId; }
    QString category() const { return m_descriptor.category; }
    QString displayName() const { return m_descriptor.displayName; }

    QList<PortItem*> inputPorts() const { return m_inputPorts; }
    QList<PortItem*> outputPorts() const { return m_outputPorts; }
    QList<PortItem*> allPorts() const;
    PortItem* portById(const QString& portId) const;

    QVariant propertyValue(const QString& propertyId) const;
    void setPropertyValue(const QString& propertyId, const QVariant& value);

    QString customLabel() const { return m_customLabel; }
    void setCustomLabel(const QString& label);

    void setPressure(double pressure);

    ComponentDescriptor descriptor() const { return m_descriptor; }
    ComponentInstance toInstance() const;

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override;

signals:
    void propertyChanged(const QString& propertyId, const QVariant& newValue);
    void positionChanged(const QUuid& id, const QPointF& newPos);
    void portsChanged();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    void createPorts();
    void updatePortPositions();
    qreal blockHeight() const;

    QUuid m_uuid;
    ComponentDescriptor m_descriptor;
    QMap<QString, QVariant> m_propertyValues;
    QList<PortItem*> m_inputPorts;
    QList<PortItem*> m_outputPorts;
    QString m_customLabel;
    double m_pressure = -1.0; // negative = not set / hidden
};
