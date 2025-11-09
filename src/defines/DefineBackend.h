#pragma once
#include "DefineManager.h"
#include <QString>

class DefineManager;
class DefineBackend {
public:
    DefineBackend() = default;
    bool load(const QString& path, DefineManager& mgr);
    bool saveDefines(const QString& path, const DefineManager& mgr) const;
};
