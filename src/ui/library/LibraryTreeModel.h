#pragma once

#include <QStandardItemModel>

class ComponentFactory;

class LibraryTreeModel : public QStandardItemModel {
    Q_OBJECT
public:
    explicit LibraryTreeModel(ComponentFactory* factory, QObject* parent = nullptr);

    void retranslate();

    // Qt::UserRole + 1 stores the typeId string for leaf items
    enum {
        TypeIdRole = Qt::UserRole + 1
    };

private:
    void populate(ComponentFactory* factory);
    ComponentFactory* m_factory = nullptr;
};
