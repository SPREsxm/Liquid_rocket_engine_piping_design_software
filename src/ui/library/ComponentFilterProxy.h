#pragma once

#include <QSortFilterProxyModel>

// Filters component library leaf items by display name.
// Category rows (top-level) are always shown.
class ComponentFilterProxy : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit ComponentFilterProxy(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override
    {
        if (filterRegularExpression().pattern().isEmpty())
            return true;

        QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);

        // Top-level items (categories) — accept if any child matches
        if (!sourceParent.isValid()) {
            int childCount = sourceModel()->rowCount(idx);
            for (int i = 0; i < childCount; ++i) {
                QModelIndex child = sourceModel()->index(i, 0, idx);
                if (child.data(Qt::DisplayRole).toString()
                        .contains(filterRegularExpression()))
                    return true;
            }
            // Also accept if category name itself matches
            return idx.data(Qt::DisplayRole).toString()
                       .contains(filterRegularExpression());
        }

        // Leaf items — match by display name
        return idx.data(Qt::DisplayRole).toString()
                   .contains(filterRegularExpression());
    }
};
