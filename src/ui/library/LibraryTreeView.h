#pragma once

#include <QTreeView>

class LibraryTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit LibraryTreeView(QWidget* parent = nullptr);

protected:
    void mouseMoveEvent(QMouseEvent* event) override;
};
