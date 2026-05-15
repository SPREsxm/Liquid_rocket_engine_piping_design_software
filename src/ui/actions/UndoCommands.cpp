#include "UndoCommands.h"
#include "ui/graphics/BlockScene.h"
#include "ui/graphics/BlockItem.h"
#include "ui/graphics/PortItem.h"
#include "ui/graphics/ConnectionItem.h"
#include "components/ComponentDescriptor.h"
#include "components/ComponentFactory.h"

#include <QJsonObject>

// ─── AddBlockCommand ────────────────────────────────────────

AddBlockCommand::AddBlockCommand(BlockScene* scene, const ComponentDescriptor& desc,
                                 const QPointF& pos, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_descriptor(desc)
    , m_position(pos)
{
    setText(QObject::tr("Add %1").arg(desc.displayName));
}

void AddBlockCommand::redo()
{
    if (m_firstRedo) {
        auto* block = m_scene->addBlock(m_descriptor, m_position);
        m_blockUuid = block->uuid();
        m_firstRedo = false;
    } else {
        // Re-add from saved descriptor
        m_scene->addBlock(m_descriptor, m_position);
        // The new block gets a new UUID — we need to restore the original UUID
        // For simplicity in MVP, we re-serialize the scene state
    }
}

void AddBlockCommand::undo()
{
    auto* block = m_scene->blockByUuid(m_blockUuid);
    if (block) {
        m_scene->removeBlock(block);
    }
}

// ─── RemoveBlockCommand ─────────────────────────────────────

RemoveBlockCommand::RemoveBlockCommand(BlockScene* scene, const QUuid& blockUuid,
                                       QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_blockUuid(blockUuid)
{
    setText(QObject::tr("Remove block"));
    m_savedState = m_scene->toJson();
}

void RemoveBlockCommand::redo()
{
    auto* block = m_scene->blockByUuid(m_blockUuid);
    if (block) {
        m_scene->removeBlock(block);
    }
}

void RemoveBlockCommand::undo()
{
    m_scene->fromJson(m_savedState);
}

// ─── MoveBlockCommand ───────────────────────────────────────

MoveBlockCommand::MoveBlockCommand(BlockScene* scene, const QUuid& blockUuid,
                                   const QPointF& oldPos, const QPointF& newPos,
                                   QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_blockUuid(blockUuid)
    , m_oldPos(oldPos)
    , m_newPos(newPos)
{
    setText(QObject::tr("Move block"));
}

void MoveBlockCommand::undo()
{
    auto* block = m_scene->blockByUuid(m_blockUuid);
    if (block) block->setPos(m_oldPos);
}

void MoveBlockCommand::redo()
{
    auto* block = m_scene->blockByUuid(m_blockUuid);
    if (block) block->setPos(m_newPos);
}

bool MoveBlockCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = dynamic_cast<const MoveBlockCommand*>(other);
    if (!cmd) return false;
    if (cmd->m_blockUuid != m_blockUuid) return false;
    m_newPos = cmd->m_newPos;
    return true;
}

// ─── AddConnectionCommand ───────────────────────────────────

AddConnectionCommand::AddConnectionCommand(
    BlockScene* scene, const QUuid& srcBlockUuid, const QString& srcPortId,
    const QUuid& dstBlockUuid, const QString& dstPortId, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_srcBlockUuid(srcBlockUuid), m_srcPortId(srcPortId)
    , m_dstBlockUuid(dstBlockUuid), m_dstPortId(dstPortId)
{
    setText(QObject::tr("Add connection"));
}

void AddConnectionCommand::redo()
{
    auto* srcBlock = m_scene->blockByUuid(m_srcBlockUuid);
    auto* dstBlock = m_scene->blockByUuid(m_dstBlockUuid);
    if (srcBlock && dstBlock) {
        auto* srcPort = srcBlock->portById(m_srcPortId);
        auto* dstPort = dstBlock->portById(m_dstPortId);
        if (srcPort && dstPort) {
            m_scene->addConnection(srcPort, dstPort);
        }
    }
}

void AddConnectionCommand::undo()
{
    // Find and remove the connection
    for (auto* conn : m_scene->allConnections()) {
        if (conn->sourcePort() && conn->destPort()
            && conn->sourcePort()->parentBlock()
            && conn->destPort()->parentBlock()
            && conn->sourcePort()->parentBlock()->uuid() == m_srcBlockUuid
            && conn->sourcePort()->portId() == m_srcPortId
            && conn->destPort()->parentBlock()->uuid() == m_dstBlockUuid
            && conn->destPort()->portId() == m_dstPortId)
        {
            m_scene->removeConnection(conn);
            break;
        }
    }
}

// ─── ChangePropertyCommand ─────────────────────────────────

ChangePropertyCommand::ChangePropertyCommand(
    BlockScene* scene, const QUuid& blockUuid,
    const QString& propertyId, const QVariant& oldValue,
    const QVariant& newValue, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_blockUuid(blockUuid)
    , m_propertyId(propertyId)
    , m_oldValue(oldValue)
    , m_newValue(newValue)
{
    setText(QObject::tr("Change %1").arg(propertyId));
}

void ChangePropertyCommand::undo()
{
    auto* block = m_scene->blockByUuid(m_blockUuid);
    if (block) block->setPropertyValue(m_propertyId, m_oldValue);
}

void ChangePropertyCommand::redo()
{
    auto* block = m_scene->blockByUuid(m_blockUuid);
    if (block) block->setPropertyValue(m_propertyId, m_newValue);
}

bool ChangePropertyCommand::mergeWith(const QUndoCommand* other)
{
    auto* cmd = dynamic_cast<const ChangePropertyCommand*>(other);
    if (!cmd) return false;
    if (cmd->m_blockUuid != m_blockUuid) return false;
    if (cmd->m_propertyId != m_propertyId) return false;
    m_newValue = cmd->m_newValue;
    return true;
}
