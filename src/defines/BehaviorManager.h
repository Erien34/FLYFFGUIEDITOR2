#pragma once
#include <QMap>
#include <QVariant>
#include <QString>

struct BehaviorInfo {
    QString category;                      // optional
    QMap<QString, QVariant> attributes;    // optional
};

// Forward-Declarations
class FlagManager;
class TextManager;
class DefineManager;
class LayoutManager;
struct ControlData;

class BehaviorManager
{
public:
    BehaviorManager(FlagManager* flagMgr,
                    TextManager* textMgr,
                    DefineManager* defineMgr,
                    LayoutManager* layoutMgr);

    // vorerst: NO-OP – gibt leere Infos zurück
    BehaviorInfo resolveBehavior(const ControlData& ctrl) const;
    void applyBehavior(ControlData& ctrl) const;

private:
    FlagManager*   m_flagMgr;
    TextManager*   m_textMgr;
    DefineManager* m_defineMgr;
    LayoutManager* m_layoutMgr;
};
