#pragma once

#include <QUuid>

// Simple wrapper around QUuid for consistent ID generation.
// All IDs in the system are UUIDs to guarantee uniqueness without a central counter.

namespace IdGenerator {

inline QUuid generate() {
    return QUuid::createUuid();
}

inline QUuid fromString(const QString& str) {
    return QUuid::fromString(str);
}

} // namespace IdGenerator
