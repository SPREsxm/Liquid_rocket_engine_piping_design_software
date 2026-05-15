#include "LibraryTreeModel.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"

#include <QIcon>
#include <QPainter>

namespace {

QIcon generateCategoryIcon(const QString& category)
{
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    QColor color;
    if (category == "Pipes")        color = QColor("#1565C0");
    else if (category == "Valves")  color = QColor("#C62828");
    else if (category == "Pumps")   color = QColor("#2E7D32");
    else if (category == "Sensors") color = QColor("#E65100");
    else if (category == "Tanks")   color = QColor("#6A1B9A");
    else                            color = QColor("#546E7A");

    QRectF r(2, 2, 28, 28);

    if (category == "Pipes") {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(r, 4, 4);
    } else if (category == "Valves") {
        QPolygonF diamond;
        diamond << QPointF(16, 2) << QPointF(30, 16) << QPointF(16, 30) << QPointF(2, 16);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPolygon(diamond);
    } else if (category == "Pumps") {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawEllipse(r);
    } else if (category == "Sensors") {
        QPolygonF tri;
        tri << QPointF(16, 2) << QPointF(30, 28) << QPointF(2, 28);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPolygon(tri);
    } else if (category == "Tanks") {
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRect(r);
    } else {
        p.setPen(QPen(color.darker(120), 2));
        p.setBrush(color.lighter(140));
        p.drawRoundedRect(r, 6, 6);
    }

    p.end();
    return QIcon(pm);
}

} // anonymous namespace

LibraryTreeModel::LibraryTreeModel(ComponentFactory* factory, QObject* parent)
    : QStandardItemModel(parent)
{
    setColumnCount(1);
    setHorizontalHeaderLabels({tr("Component Library")});
    populate(factory);
}

void LibraryTreeModel::populate(ComponentFactory* factory)
{
    for (const QString& category : factory->allCategories()) {
        auto* categoryItem = new QStandardItem(category);
        categoryItem->setFlags(Qt::ItemIsEnabled);
        categoryItem->setSelectable(false);
        QFont font = categoryItem->font();
        font.setBold(true);
        categoryItem->setFont(font);

        for (const auto& desc : factory->componentsInCategory(category)) {
            auto* item = new QStandardItem(desc.displayName);
            item->setData(desc.typeId, TypeIdRole);
            item->setToolTip(desc.description);
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
            if (!desc.iconName.isEmpty())
                item->setIcon(QIcon(desc.iconName));
            else
                item->setIcon(generateCategoryIcon(desc.category));
            categoryItem->appendRow(item);
        }
        appendRow(categoryItem);
    }
}
