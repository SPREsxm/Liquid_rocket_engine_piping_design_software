#pragma once

#include <QString>
#include <QStringList>

namespace AppConstants {

inline constexpr const char* MIME_COMPONENT_TYPE     = "application/x-lrep-component";
inline constexpr const char* MIME_BLOCK_CLIPBOARD   = "application/x-lrep-blocks";

inline constexpr qreal DEFAULT_GRID_SIZE  = 20.0;
inline constexpr qreal MIN_ZOOM           = 0.1;
inline constexpr qreal MAX_ZOOM           = 5.0;
inline constexpr qreal ZOOM_STEP          = 1.15;

inline const QString APP_NAME    = "LiquidRocketPipingDesigner";
inline const QString APP_VERSION = "0.1.0";
inline const QString ORG_NAME    = "LRE";

// Semantic version parsing and comparison
struct SemVer {
    int major = 0;
    int minor = 1;
    int patch = 0;
    QString preRelease;

    static SemVer fromString(const QString& version) {
        SemVer v;
        QStringList parts = version.split('.');
        if (parts.size() > 0) v.major = parts[0].toInt();
        if (parts.size() > 1) v.minor = parts[1].toInt();
        if (parts.size() > 2) {
            // Handle pre-release suffix (e.g. "0-alpha")
            QString patchStr = parts[2];
            int dashIdx = patchStr.indexOf('-');
            if (dashIdx > 0) {
                v.patch = patchStr.left(dashIdx).toInt();
                v.preRelease = patchStr.mid(dashIdx + 1);
            } else {
                v.patch = patchStr.toInt();
            }
        }
        return v;
    }

    QString toString() const {
        QString s = QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(patch);
        if (!preRelease.isEmpty())
            s += QStringLiteral("-%1").arg(preRelease);
        return s;
    }

    bool operator==(const SemVer& o) const {
        return major == o.major && minor == o.minor
            && patch == o.patch && preRelease == o.preRelease;
    }
    bool operator<(const SemVer& o) const {
        if (major != o.major) return major < o.major;
        if (minor != o.minor) return minor < o.minor;
        if (patch != o.patch) return patch < o.patch;
        if (preRelease.isEmpty() != o.preRelease.isEmpty())
            return !preRelease.isEmpty(); // pre-release < release
        return preRelease < o.preRelease;
    }
};

inline const SemVer APP_SEMVER = SemVer::fromString(APP_VERSION);

inline bool isCompatibleVersion(const SemVer& pluginMinVersion) {
    return APP_SEMVER.major == pluginMinVersion.major
        && !(APP_SEMVER < pluginMinVersion);
}

} // namespace AppConstants
