#include "LibraryTreeView.h"
#include "LibraryTreeModel.h"
#include "components/ComponentFactory.h"
#include "ui/graphics/BlockAppearance.h"
#include "core/Constants.h"

#include <QDataStream>
#include <QDrag>
#include <QMouseEvent>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>

LibraryTreeView::LibraryTreeView(QWidget* parent)
    : QTreeView(parent)
{
    setHeaderHidden(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setExpandsOnDoubleClick(false);
}

void LibraryTreeView::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return QTreeView::mouseMoveEvent(event);

    QModelIndex idx = indexAt(event->pos());
    if (!idx.isValid()) return;

    QString typeId = idx.data(LibraryTreeModel::TypeIdRole).toString();
    if (typeId.isEmpty()) return;

    QByteArray byteData;
    QDataStream stream(&byteData, QIODevice::WriteOnly);
    stream << typeId;

    auto* mimeData = new QMimeData();
    mimeData->setData(AppConstants::MIME_COMPONENT_TYPE, byteData);

    auto* drag = new QDrag(this);
    drag->setMimeData(mimeData);

    // Generate drag thumbnail
    const auto* desc = ComponentFactory::instance().descriptorForType(typeId);
    if (desc) {
        QPixmap pixmap(120, 42);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(BlockAppearance::headerColor(desc->category));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(0, 0, 120, 42), 5, 5);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Segoe UI", 8, QFont::Bold));
        painter.drawText(QRectF(0, 0, 120, 42), Qt::AlignCenter, desc->displayName);
        painter.end();
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(60, 21));
    }

    drag->exec(Qt::CopyAction);
}
