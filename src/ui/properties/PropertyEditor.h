#pragma once

#include <QDockWidget>
#include <QVariant>

class QFormLayout;
class QScrollArea;
class QLabel;
class BlockItem;

class PropertyEditor : public QDockWidget {
    Q_OBJECT
public:
    explicit PropertyEditor(QWidget* parent = nullptr);

public slots:
    void showBlockProperties(BlockItem* block);
    void clearProperties();

private:
    void rebuildForm(BlockItem* block);

    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_contentWidget = nullptr;
    QFormLayout* m_formLayout = nullptr;
    QLabel* m_emptyLabel = nullptr;
    BlockItem* m_currentBlock = nullptr;
};
