#pragma once

#include <QUndoCommand>
#include <QUuid>
#include <QPointF>
#include <QJsonObject>
#include <QVariant>
#include <QString>

#include "components/ComponentDescriptor.h"

class BlockScene;

// ─── Add / Remove Block ─────────────────────────────────────

class AddBlockCommand : public QUndoCommand {
public:
    AddBlockCommand(BlockScene* scene, const ComponentDescriptor& desc,
                    const QPointF& pos, QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

private:
    BlockScene* m_scene;
    ComponentDescriptor m_descriptor;
    QPointF m_position;
    QUuid m_blockUuid;
    bool m_firstRedo = true;
};

class RemoveBlockCommand : public QUndoCommand {
public:
    RemoveBlockCommand(BlockScene* scene, const QUuid& blockUuid,
                       QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

private:
    BlockScene* m_scene;
    QUuid m_blockUuid;
    QJsonObject m_savedState;   // full scene snapshot for undo
};

// ─── Move Block ─────────────────────────────────────────────

class MoveBlockCommand : public QUndoCommand {
public:
    MoveBlockCommand(BlockScene* scene, const QUuid& blockUuid,
                     const QPointF& oldPos, const QPointF& newPos,
                     QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    int id() const override { return 1001; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    BlockScene* m_scene;
    QUuid m_blockUuid;
    QPointF m_oldPos;
    QPointF m_newPos;
};

// ─── Add / Remove Connection ────────────────────────────────

class AddConnectionCommand : public QUndoCommand {
public:
    AddConnectionCommand(BlockScene* scene, const QUuid& srcBlockUuid,
                         const QString& srcPortId,
                         const QUuid& dstBlockUuid, const QString& dstPortId,
                         QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;

private:
    BlockScene* m_scene;
    QUuid m_srcBlockUuid, m_dstBlockUuid;
    QString m_srcPortId, m_dstPortId;
};

// ─── Change Property ────────────────────────────────────────

class ChangePropertyCommand : public QUndoCommand {
public:
    ChangePropertyCommand(BlockScene* scene, const QUuid& blockUuid,
                          const QString& propertyId,
                          const QVariant& oldValue, const QVariant& newValue,
                          QUndoCommand* parent = nullptr);
    void undo() override;
    void redo() override;
    int id() const override { return 1002; }
    bool mergeWith(const QUndoCommand* other) override;

private:
    BlockScene* m_scene;
    QUuid m_blockUuid;
    QString m_propertyId;
    QVariant m_oldValue;
    QVariant m_newValue;
};
