#include "defines/BehaviorManager.h"
#include "defines/FlagManager.h"
#include "defines/DefineManager.h"
#include "texts/TextManager.h"
#include "layout/LayoutManager.h"
#include "layout/model/ControlData.h"

BehaviorManager::BehaviorManager(FlagManager* flagMgr,
                                 TextManager* textMgr,
                                 DefineManager* defineMgr,
                                 LayoutManager* layoutMgr)
    : m_flagMgr(flagMgr)
    , m_textMgr(textMgr)
    , m_defineMgr(defineMgr)
    , m_layoutMgr(layoutMgr)
{
}

BehaviorInfo BehaviorManager::resolveBehavior(const ControlData& ctrl) const
{
    Q_UNUSED(ctrl);
    BehaviorInfo info;
    // absichtlich leer → NO-OP
    return info;
}

void BehaviorManager::applyBehavior(ControlData& ctrl) const
{
    // fürs erste: nichts tun
    Q_UNUSED(ctrl);
}
