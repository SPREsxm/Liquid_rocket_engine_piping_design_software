#include "BlockItem.h"
#include "PortItem.h"
#include "ConnectionItem.h"
#include "BlockView.h"

#include <QCursor>
#include <QFont>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QPen>
#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>
#include <QToolTip>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsDropShadowEffect>

BlockItem::BlockItem(const ComponentInstance& instance,
                     const ComponentDescriptor& descriptor,
                     QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_uuid(instance.uuid)
    , m_descriptor(descriptor)
    , m_propertyValues(instance.propertyValues)
    , m_customLabel(instance.customLabel.isEmpty()
                    ? descriptor.displayName : instance.customLabel)
{
    setFlag(ItemIsMovable);
    setFlag(ItemIsSelectable);
    setFlag(ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);
    setCursor(QCursor(Qt::OpenHandCursor));
    setPos(instance.position);
    setZValue(1);

    createPorts();
    updatePortPositions();
}

// ─── Ports ──────────────────────────────────────────────────

void BlockItem::createPorts()
{
    int idx = 0;
    for (const auto& pd : m_descriptor.inputPorts) {
        auto* port = new PortItem(idx++, pd.id, pd.direction, pd.dataType, this);
        m_inputPorts.append(port);
    }
    for (const auto& pd : m_descriptor.outputPorts) {
        auto* port = new PortItem(idx++, pd.id, pd.direction, pd.dataType, this);
        m_outputPorts.append(port);
    }
}

QList<PortItem*> BlockItem::allPorts() const
{
    QList<PortItem*> result;
    result.append(m_inputPorts);
    result.append(m_outputPorts);
    return result;
}

PortItem* BlockItem::portById(const QString& portId) const
{
    for (auto* p : allPorts()) {
        if (p->portId() == portId) return p;
    }
    return nullptr;
}

void BlockItem::updatePortPositions()
{
    const qreal h = blockHeight();
    const qreal w = BlockAppearance::BLOCK_WIDTH;

    // Inputs along left edge
    {
        const int n = m_inputPorts.size();
        for (int i = 0; i < n; ++i) {
            const qreal y = BlockAppearance::HEADER_HEIGHT
                            + (h - BlockAppearance::HEADER_HEIGHT) / (n + 1) * (i + 1);
            m_inputPorts[i]->setPos(0, y);
        }
    }

    // Outputs along right edge
    {
        const int n = m_outputPorts.size();
        for (int i = 0; i < n; ++i) {
            const qreal y = BlockAppearance::HEADER_HEIGHT
                            + (h - BlockAppearance::HEADER_HEIGHT) / (n + 1) * (i + 1);
            m_outputPorts[i]->setPos(w, y);
        }
    }
}

qreal BlockItem::blockHeight() const
{
    const int totalPorts = m_inputPorts.size() + m_outputPorts.size();
    return qMax(BlockAppearance::BLOCK_MIN_HEIGHT,
                totalPorts * 20.0 + BlockAppearance::HEADER_HEIGHT + 10.0);
}

// ─── Properties ─────────────────────────────────────────────

QVariant BlockItem::propertyValue(const QString& propertyId) const
{
    return m_propertyValues.value(propertyId);
}

void BlockItem::setPropertyValue(const QString& propertyId, const QVariant& value)
{
    if (m_propertyValues.contains(propertyId)) {
        m_propertyValues[propertyId] = value;
        emit propertyChanged(propertyId, value);
    }
}

void BlockItem::setCustomLabel(const QString& label)
{
    m_customLabel = label;
    update();
}

void BlockItem::setPressure(double pressure)
{
    m_pressure = pressure;
    update();
}

ComponentInstance BlockItem::toInstance() const
{
    ComponentInstance inst;
    inst.uuid = m_uuid;
    inst.typeId = m_descriptor.typeId;
    inst.position = pos();
    inst.propertyValues = m_propertyValues;
    inst.customLabel = m_customLabel;
    return inst;
}

// ─── Bounding Box ───────────────────────────────────────────

QRectF BlockItem::boundingRect() const
{
    const qreal h = blockHeight();
    const qreal w = BlockAppearance::BLOCK_WIDTH;

    return QRectF(-BlockAppearance::PORT_RADIUS,
                  -BlockAppearance::PORT_RADIUS,
                  w + 2 * BlockAppearance::PORT_RADIUS,
                  h + 2 * BlockAppearance::PORT_RADIUS);
}

// ─── Paint ─────────────────────────────────────────────────

void BlockItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    painter->setRenderHint(QPainter::Antialiasing);

    const qreal w = BlockAppearance::BLOCK_WIDTH;
    const qreal h = blockHeight();
    const qreal r = BlockAppearance::BLOCK_CORNER;
    const qreal hh = BlockAppearance::HEADER_HEIGHT;

    const QRectF bodyRect(0, 0, w, h);
    const QRectF headerRect(0, 0, w, hh);

    QColor border = isSelected()
        ? BlockAppearance::selectedBorderColor()
        : BlockAppearance::borderColor();
    const qreal borderW = isSelected() ? 2.5 : 1.5;

    // Drop shadow (offset 3px, 45% opacity black)
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 45));
    painter->drawRoundedRect(bodyRect.translated(3, 3), r, r);

    // Body — pressure gradient if analysis data available
    if (m_pressure >= 0.0) {
        double ratio = qBound(0.0, m_pressure / 20.0e6, 1.0);  // 0→20 MPa maps to blue→red
        QColor pc(static_cast<int>(21 + ratio * (198 - 21)),
                  static_cast<int>(101 + (1.0 - ratio) * (101 - 40)),
                  static_cast<int>(192 + (1.0 - ratio) * (192 - 40)));
        QLinearGradient grad(bodyRect.topLeft(), bodyRect.bottomRight());
        grad.setColorAt(0.0, pc.lighter(130));
        grad.setColorAt(1.0, pc);
        painter->setBrush(grad);
    } else {
        painter->setBrush(BlockAppearance::bodyColor());
    }
    painter->drawRoundedRect(bodyRect, r, r);

    // Header
    {
        QPainterPath headerPath;
        headerPath.addRoundedRect(headerRect, r, r);
        headerPath.addRect(QRectF(0, hh - r, r, r));
        headerPath.addRect(QRectF(w - r, hh - r, r, r));
        painter->setBrush(BlockAppearance::headerColor(m_descriptor.category));
        painter->drawPath(headerPath);
    }

    // Header text
    painter->setPen(BlockAppearance::headerTextColor());
    QFont headerFont("Segoe UI", 9, QFont::Bold);
    painter->setFont(headerFont);
    painter->drawText(QRectF(6, 0, w - 12, hh), Qt::AlignVCenter | Qt::AlignLeft,
                      m_customLabel);

    // Body text (category label)
    painter->setPen(BlockAppearance::textColor());
    QFont bodyFont("Segoe UI", 8);
    painter->setFont(bodyFont);
    const QRectF textRect(6, hh + 4, w - 12, h - hh - 8);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop,
                      m_descriptor.category);

    // Pressure readout at bottom (when set)
    if (m_pressure >= 0.0) {
        painter->setPen(BlockAppearance::pressureTextColor());
        QFont pressureFont("Segoe UI", 7, QFont::Bold);
        painter->setFont(pressureFont);
        double pressureMPa = m_pressure / 1.0e6;
        QString pressureText = QStringLiteral("P: %1 MPa")
            .arg(pressureMPa, 0, 'f', 3);
        const QRectF pressureRect(6, h - 18, w - 12, 14);
        painter->drawText(pressureRect, Qt::AlignRight | Qt::AlignVCenter,
                          pressureText);
    }

    // Border
    painter->setPen(QPen(border, borderW));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(bodyRect, r, r);
}

// ─── Item Change ────────────────────────────────────────────

QVariant BlockItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene()) {
        // Snap to grid if enabled
        QPointF newPos = value.toPointF();
        // Grid snapping is handled by BlockScene — just emit
        emit positionChanged(m_uuid, newPos);
    }
    if (change == ItemPositionHasChanged) {
        emit positionChanged(m_uuid, pos());
    }
    if (change == ItemSelectedHasChanged) {
        update();
    }
    return QGraphicsObject::itemChange(change, value);
}

void BlockItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent*)
{
    bool ok = false;
    const QString newLabel = QInputDialog::getText(
        nullptr, tr("Rename Block"), tr("New label:"),
        QLineEdit::Normal, m_customLabel, &ok);
    if (ok && !newLabel.isEmpty()) {
        setCustomLabel(newLabel);
    }
}

void BlockItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    setSelected(true);
    QMenu menu;
    auto* renameAction = menu.addAction(tr("Rename..."));
    menu.addSeparator();
    auto* copyAction = menu.addAction(tr("Copy"));
    auto* deleteAction = menu.addAction(tr("Delete"));

    QAction* chosen = menu.exec(event->screenPos());
    if (!chosen) return;

    if (chosen == renameAction) {
        mouseDoubleClickEvent(nullptr);
    } else if (chosen == copyAction) {
        auto views = scene()->views();
        if (!views.isEmpty()) {
            if (auto* bv = qobject_cast<BlockView*>(views.first()))
                bv->copySelected();
        }
    } else if (chosen == deleteAction) {
        auto views = scene()->views();
        if (!views.isEmpty()) {
            if (auto* bv = qobject_cast<BlockView*>(views.first()))
                bv->deleteSelected();
        }
    }
}

void BlockItem::setAnalysisTooltip(double pressure, double inletFlow, double outletFlow)
{
    m_hasAnalysisData = true;
    m_tooltipPressure = pressure;
    m_tooltipInletFlow = inletFlow;
    m_tooltipOutletFlow = outletFlow;
    setAcceptHoverEvents(true);
}

QString BlockItem::makeTooltipText() const
{
    if (!m_hasAnalysisData) return {};
    return QStringLiteral("Type: %1\nPressure: %2 MPa\nInlet Flow: %3 kg/s\nOutlet Flow: %4 kg/s")
        .arg(m_descriptor.displayName)
        .arg(m_tooltipPressure / 1.0e6, 0, 'f', 4)
        .arg(m_tooltipInletFlow, 0, 'f', 4)
        .arg(m_tooltipOutletFlow, 0, 'f', 4);
}

void BlockItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    if (m_hasAnalysisData)
        QToolTip::showText(event->screenPos(), makeTooltipText());
    update();
}

void BlockItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (m_hasAnalysisData)
        QToolTip::showText(event->screenPos(), makeTooltipText());
}

void BlockItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    update();
}
