#include "PropertyEditor.h"
#include "ui/graphics/BlockItem.h"
#include "components/ComponentDescriptor.h"
#include "core/Types.h"
#include "utils/ExpressionEngine.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
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
    if (m_currentBlock == block) return;
    m_currentBlock = block;

    clearProperties();

    if (!block) return;
    rebuildForm(block);
}

void PropertyEditor::clearProperties()
{
    // Remove all rows except the empty label
    while (m_formLayout->rowCount() > 0) {
        m_formLayout->removeRow(0);
    }
    m_emptyLabel = new QLabel(tr("No component selected.\nSelect a block on the canvas to edit its properties."));
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("color: #757575; padding: 20px;");
    m_formLayout->addRow(m_emptyLabel);
    m_currentBlock = nullptr;
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
