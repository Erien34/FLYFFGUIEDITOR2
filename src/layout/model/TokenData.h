#pragma once
#include <QString>
#include <QList>
#include <QMap>
#include <mutex>

// ------------------------------------------------------------
// Token-Struktur
// ------------------------------------------------------------
struct Token {
    QString type;        // WindowHeader / ControlHeader / Text / Other
    QString value;       // Originalzeile
    QString windowName;  // Zugehöriges Fenster
    QString controlId;   // Zugehöriges Control (falls vorhanden)
    int orderIndex = -1; // Reihenfolge
    QString comment;     // Letzter Kommentar
};

// ------------------------------------------------------------
// TokenData (Singleton)
// ------------------------------------------------------------
// Zentraler globaler Tokenspeicher, thread-sicher.
// Alle Manager greifen hierauf zu.
// ------------------------------------------------------------
class TokenData
{
public:
    static TokenData& instance() {
        static TokenData inst;
        return inst;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tokens.clear();
    }

    void addTokens(const QString& windowName, const QList<Token>& tokens) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tokens[windowName] = tokens;
    }

    QList<Token> getTokens(const QString& windowName) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_tokens.value(windowName);
    }

    QMap<QString, QList<Token>> all() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_tokens;
    }

private:
    TokenData() = default;
    mutable std::mutex m_mutex;
    QMap<QString, QList<Token>> m_tokens;
};
