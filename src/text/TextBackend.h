#pragma once
#include <QString>

class TextManager;

class TextBackend {
public:
    TextBackend() = default;

    // Lädt textClient.txt (IDS → Text)
    bool loadText(const QString& path, TextManager& mgr);

    // Lädt textClient.inc (TID → IDS)
    bool loadInc(const QString& path, TextManager& mgr);

    // Speichert textClient.txt
    bool saveText(const QString& path, const TextManager& mgr) const;

    // Speichert textClient.inc
    bool saveInc(const QString& path, const TextManager& mgr) const;
};
