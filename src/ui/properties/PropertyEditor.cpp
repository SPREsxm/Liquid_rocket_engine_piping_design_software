#include "PropertyEditor.h"
#include "ui/graphics/BlockItem.h"
#include "components/ComponentDescriptor.h"
#include "core/Types.h"
#include "utils/ExpressionEngine.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QWidget>

PropertyEditor::PropertyEditor(QWidget* parent)
    : QDockWidget(tr("Properties"), parent)
{
    setMinimumWidth(220);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);

    m_contentWidget = new QWidget;
    m_formLayout = new QFormLayout(m_contentWidget);
    m_formLayout->setContentsMargins(8, 8, 8, 8);
    m_formLayout->setSpacing(6);

    // Empty-state label
    m_emptyLabel = new QLabel(tr("No component selected.\nSelect a block on the canvas to edit its properties."));
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #757575; padding: 20px;");
    m_formLayout->addRow(m_emptyLabel);

    m_scrollArea->setWidget(m_contentWidget);
    setWidget(m_scrollArea);
}

void PropertyEditor::showBlockProperties(BlockItem* block)
{
    if (block)
        showBlocksProperties({block});
    else
        clearProperties();
}

void PropertyEditor::showBlocksProperties(const QList<BlockItem*>& blocks)
{
    m_currentBlocks = blocks;
    clearProperties();

    if (blocks.isEmpty()) return;

    if (blocks.size() == 1) {
        m_currentBlock = blocks.first();
        rebuildForm(blocks.first());
    } else {
        m_currentBlock = nullptr;
        rebuildFormBatch(blocks);
    }
}

void PropertyEditor::clearProperties()
{
    while (m_formLayout->rowCount() > 0) {
        m_formLayout->removeRow(0);
    }
    m_emptyLabel = new QLabel(tr("No component selected.\nSelect a block on the canvas to edit its properties."));
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #757575; padding: 20px;");
    m_formLayout->addRow(m_emptyLabel);
    m_currentBlock = nullptr;
    m_currentBlocks.clear();
}

bool PropertyEditor::allSameType(const QList<BlockItem*>& blocks) const
{
    if (blocks.isEmpty()) return false;
    const QString& firstType = blocks.first()->typeId();
    for (const auto* b : blocks) {
        if (b->typeId() != firstType) return false;
    }
    return true;
}

QVariant PropertyEditor::commonValue(const QList<BlockItem*>& blocks, const QString& propId) const
{
    if (blocks.isEmpty()) return {};
    QVariant first = blocks.first()->propertyValue(propId);
    for (const auto* b : blocks) {
        if (b->propertyValue(propId) != first)
            return {}; // invalid QVariant = "varies"
    }
    return first;
}

void PropertyEditor::rebuildForm(BlockItem* block)
{
    // Remove empty label
    while (m_formLayout->rowCount() > 0) {
        m_formLayout->removeRow(0);
    }

    // Block info header
    auto* nameLabel = new QLabel(
        QString("<b>%1</b><br><span style='color:#757575'>%2</span>")
            .arg(block->customLabel()).arg(block->typeId()));
    nameLabel->setWordWrap(true);
    m_formLayout->addRow(nameLabel);

    // Separator
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    m_formLayout->addRow(sep);

    // Custom label field
    auto* labelEdit = new QLineEdit(block->customLabel());
    connect(labelEdit, &QLineEdit::textChanged, block, &BlockItem::setCustomLabel);
    m_formLayout->addRow(tr("Label:"), labelEdit);

    // Properties from descriptor
    const auto& props = block->descriptor().properties;
    for (const auto& prop : props) {
        QWidget* editor = nullptr;

        switch (prop.type) {
        case PropertyType::Double: {
            auto* spin = new QDoubleSpinBox;
            spin->setDecimals(6);
            spin->setRange(prop.minValue.toDouble(), prop.maxValue.toDouble());
            spin->setValue(block->propertyValue(prop.id).toDouble());
            if (!prop.unit.isEmpty()) spin->setSuffix(" " + prop.unit);
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    block, [block, id = prop.id](double v) {
                block->setPropertyValue(id, v);
            });
            editor = spin;
            break;
        }
        case PropertyType::Int: {
            auto* spin = new QSpinBox;
            spin->setRange(prop.minValue.toInt(), prop.maxValue.toInt());
            spin->setValue(block->propertyValue(prop.id).toInt());
            if (!prop.unit.isEmpty()) spin->setSuffix(" " + prop.unit);
            connect(spin, QOverload<int>::of(&QSpinBox::valueChanged),
                    block, [block, id = prop.id](int v) {
                block->setPropertyValue(id, v);
            });
            editor = spin;
            break;
        }
        case PropertyType::Bool: {
            auto* check = new QCheckBox;
            check->setChecked(block->propertyValue(prop.id).toBool());
            connect(check, &QCheckBox::toggled,
                    block, [block, id = prop.id](bool v) {
                block->setPropertyValue(id, v);
            });
            editor = check;
            break;
        }
        case PropertyType::String: {
            auto* edit = new QLineEdit;
            edit->setText(block->propertyValue(prop.id).toString());
            connect(edit, &QLineEdit::textChanged,
                    block, [block, id = prop.id](const QString& v) {
                block->setPropertyValue(id, v);
            });
            editor = edit;
            break;
        }
        case PropertyType::Enum: {
            auto* combo = new QComboBox;
            combo->addItems(prop.enumOptions);
            const int idx = combo->findText(block->propertyValue(prop.id).toString());
            if (idx >= 0) combo->setCurrentIndex(idx);
            connect(combo, &QComboBox::currentTextChanged,
                    block, [block, id = prop.id](const QString& v) {
                block->setPropertyValue(id, v);
            });
            editor = combo;
            break;
        }
        case PropertyType::Expression: {
            auto* container = new QWidget;
            auto* hLayout = new QHBoxLayout(container);
            hLayout->setContentsMargins(0, 0, 0, 0);
            hLayout->setSpacing(4);

            auto* edit = new QLineEdit;
            edit->setText(block->propertyValue(prop.id).toString());
            edit->setPlaceholderText(tr("e.g., 2*P+rho*g*h"));

            auto* statusLabel = new QLabel;
            statusLabel->setFixedWidth(16);

            auto* compileBtn = new QPushButton(tr("Check"));
            compileBtn->setFixedWidth(50);

            // Compile on button click
            connect(compileBtn, &QPushButton::clicked,
                    edit, [edit, statusLabel]() {
                if (!ExpressionEngine::isAvailable()) {
                    statusLabel->setText(QStringLiteral("✘"));
                    statusLabel->setStyleSheet("color: #E53935; font-weight: bold;");
                    statusLabel->setToolTip(QStringLiteral("ExprTk not available"));
                    return;
                }
                ExpressionEngine::Script script;
                if (script.compile(edit->text())) {
                    statusLabel->setText(QStringLiteral("✔"));
                    statusLabel->setStyleSheet("color: #43A047; font-weight: bold;");
                    statusLabel->setToolTip(QStringLiteral("Expression OK"));
                } else {
                    statusLabel->setText(QStringLiteral("✘"));
                    statusLabel->setStyleSheet("color: #E53935; font-weight: bold;");
                    statusLabel->setToolTip(script.errorString());
                }
            });

            // Auto-store to block on text change
            connect(edit, &QLineEdit::textChanged,
                    block, [block, id = prop.id](const QString& v) {
                block->setPropertyValue(id, v);
            });

            hLayout->addWidget(edit, 1);
            hLayout->addWidget(compileBtn);
            hLayout->addWidget(statusLabel);
            editor = container;
            break;
        }
        }

        if (editor) {
            m_formLayout->addRow(prop.displayName + ":", editor);
        }
    }

    // Spacer
    m_formLayout->addRow(new QWidget); // stretches to fill remaining space
}

void PropertyEditor::rebuildFormBatch(const QList<BlockItem*>& blocks)
{
    // Remove empty label
    while (m_formLayout->rowCount() > 0) {
        m_formLayout->removeRow(0);
    }

    // Header: count and type
    QString headerText;
    if (allSameType(blocks)) {
        headerText = QString("<b>%1</b> &times; %2<br><span style='color:#757575'>%3</span>")
            .arg(blocks.first()->displayName())
            .arg(blocks.size())
            .arg(blocks.first()->typeId());
    } else {
        headerText = QString("<b>%1 blocks</b> selected<br><span style='color:#757575'>Mixed types</span>")
            .arg(blocks.size());
    }
    auto* nameLabel = new QLabel(headerText);
    nameLabel->setWordWrap(true);
    m_formLayout->addRow(nameLabel);

    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    m_formLayout->addRow(sep);

    // Custom label — check if all same
    QVariant commonLabel = commonValue(blocks, QString()); // special: use customLabel
    // Actually, check custom labels
    QString firstLabel = blocks.first()->customLabel();
    bool sameLabel = true;
    for (const auto* b : blocks) {
        if (b->customLabel() != firstLabel) { sameLabel = false; break; }
    }
    if (sameLabel) {
        auto* labelEdit = new QLineEdit(firstLabel);
        connect(labelEdit, &QLineEdit::textChanged, this, [blocks](const QString& v) {
            for (auto* b : blocks)
                b->setCustomLabel(v);
        });
        m_formLayout->addRow(tr("Label:"), labelEdit);
    } else {
        auto* variesLabel = new QLabel(tr("— varies —"));
        variesLabel->setStyleSheet("color: #9E9E9E; font-style: italic;");
        m_formLayout->addRow(tr("Label:"), variesLabel);
    }

    // If mixed types, only show label
    if (!allSameType(blocks)) {
        auto* note = new QLabel(tr("Different component types selected.\nOnly common properties are shown."));
        note->setWordWrap(true);
        note->setStyleSheet("color: #757575; padding: 8px 0;");
        m_formLayout->addRow(note);
        m_formLayout->addRow(new QWidget); // spacer
        return;
    }

    // Properties from descriptor (use first block's descriptor as template)
    const auto& props = blocks.first()->descriptor().properties;
    for (const auto& prop : props) {
        QVariant common = commonValue(blocks, prop.id);

        if (!common.isValid()) {
            // Values differ — show "varies" placeholder
            auto* variesLabel = new QLabel(tr("— varies —"));
            variesLabel->setStyleSheet("color: #9E9E9E; font-style: italic;");
            m_formLayout->addRow(prop.displayName + ":", variesLabel);
            continue;
        }

        QWidget* editor = nullptr;

        switch (prop.type) {
        case PropertyType::Double: {
            auto* spin = new QDoubleSpinBox;
            spin->setDecimals(6);
            spin->setRange(prop.minValue.toDouble(), prop.maxValue.toDouble());
            spin->setValue(common.toDouble());
            if (!prop.unit.isEmpty()) spin->setSuffix(" " + prop.unit);
            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, [blocks, id = prop.id](double v) {
                for (auto* b : blocks)
                    b->setPropertyValue(id, v);
            });
            editor = spin;
            break;
        }
        case PropertyType::Int: {
            auto* spin = new QSpinBox;
            spin->setRange(prop.minValue.toInt(), prop.maxValue.toInt());
            spin->setValue(common.toInt());
            if (!prop.unit.isEmpty()) spin->setSuffix(" " + prop.unit);
            connect(spin, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [blocks, id = prop.id](int v) {
                for (auto* b : blocks)
                    b->setPropertyValue(id, v);
            });
            editor = spin;
            break;
        }
        case PropertyType::Bool: {
            auto* check = new QCheckBox;
            check->setChecked(common.toBool());
            connect(check, &QCheckBox::toggled,
                    this, [blocks, id = prop.id](bool v) {
                for (auto* b : blocks)
                    b->setPropertyValue(id, v);
            });
            editor = check;
            break;
        }
        case PropertyType::String: {
            auto* edit = new QLineEdit;
            edit->setText(common.toString());
            connect(edit, &QLineEdit::textChanged,
                    this, [blocks, id = prop.id](const QString& v) {
                for (auto* b : blocks)
                    b->setPropertyValue(id, v);
            });
            editor = edit;
            break;
        }
        case PropertyType::Enum: {
            auto* combo = new QComboBox;
            combo->addItems(prop.enumOptions);
            const int idx = combo->findText(common.toString());
            if (idx >= 0) combo->setCurrentIndex(idx);
            connect(combo, &QComboBox::currentTextChanged,
                    this, [blocks, id = prop.id](const QString& v) {
                for (auto* b : blocks)
                    b->setPropertyValue(id, v);
            });
            editor = combo;
            break;
        }
        case PropertyType::Expression: {
            auto* container = new QWidget;
            auto* hLayout = new QHBoxLayout(container);
            hLayout->setContentsMargins(0, 0, 0, 0);
            hLayout->setSpacing(4);

            auto* edit = new QLineEdit;
            edit->setText(common.toString());
            edit->setPlaceholderText(tr("e.g., 2*P+rho*g*h"));

            auto* statusLabel = new QLabel;
            statusLabel->setFixedWidth(16);

            auto* compileBtn = new QPushButton(tr("Check"));
            compileBtn->setFixedWidth(50);

            connect(compileBtn, &QPushButton::clicked,
                    edit, [edit, statusLabel]() {
                if (!ExpressionEngine::isAvailable()) {
                    statusLabel->setText(QStringLiteral("✘"));
                    statusLabel->setStyleSheet("color: #E53935; font-weight: bold;");
                    statusLabel->setToolTip(QStringLiteral("ExprTk not available"));
                    return;
                }
                ExpressionEngine::Script script;
                if (script.compile(edit->text())) {
                    statusLabel->setText(QStringLiteral("✔"));
                    statusLabel->setStyleSheet("color: #43A047; font-weight: bold;");
                    statusLabel->setToolTip(QStringLiteral("Expression OK"));
                } else {
                    statusLabel->setText(QStringLiteral("✘"));
                    statusLabel->setStyleSheet("color: #E53935; font-weight: bold;");
                    statusLabel->setToolTip(script.errorString());
                }
            });

            connect(edit, &QLineEdit::textChanged,
                    this, [blocks, id = prop.id](const QString& v) {
                for (auto* b : blocks)
                    b->setPropertyValue(id, v);
            });

            hLayout->addWidget(edit, 1);
            hLayout->addWidget(compileBtn);
            hLayout->addWidget(statusLabel);
            editor = container;
            break;
        }
        }

        if (editor) {
            m_formLayout->addRow(prop.displayName + ":", editor);
        }
    }

    m_formLayout->addRow(new QWidget); // spacer
}
