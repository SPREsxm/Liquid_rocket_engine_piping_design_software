#pragma once

#include "core/Types.h"
#include <QString>

struct PortDescriptor {
    QString id;
    QString displayName;
    PortDirection direction = PortDirection::Input;
    PortDataType dataType   = PortDataType::Fluid;

    bool operator==(const PortDescriptor& other) const {
        return id == other.id
            && direction == other.direction
            && dataType == other.dataType;
    }

    bool operator!=(const PortDescriptor& other) const {
        return !(*this == other);
    }
};
