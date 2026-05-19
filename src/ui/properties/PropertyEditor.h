#pragma once

#include <QDockWidget>
#include <QVariant>
#include <QList>

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
    void showBlocksProperties(const QList<BlockItem*>& blocks);
    void clearProperties();
    void retranslate();

private:
    void rebuildForm(BlockItem* block);
    void rebuildFormBatch(const QList<BlockItem*>& blocks);
    bool allSameType(const QList<BlockItem*>& blocks) const;
    QVariant commonValue(const QList<BlockItem*>& blocks, const QString& propId) const;

    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_contentWidget = nullptr;
    QFormLayout* m_formLayout = nullptr;
    QLabel* m_emptyLabel = nullptr;
    BlockItem* m_currentBlock = nullptr;
    QList<BlockItem*> m_currentBlocks;
};
