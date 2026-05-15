#include "LibraryTreeModel.h"
#include "components/ComponentFactory.h"
#include "components/ComponentDescriptor.h"

#include <QIcon>

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
            if (!desc.iconName.isEmpty()) {
                item->setIcon(QIcon(desc.iconName));
            }
            categoryItem->appendRow(item);
        }
        appendRow(categoryItem);
    }
}
