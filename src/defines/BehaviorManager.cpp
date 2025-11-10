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
    BehaviorInfo info;
    if (ctrl.type == "WTYPE_TABCTRL") {
        bool vertical = (ctrl.flagsMask & 0x20260000);
        info.attributes["orientation"] = vertical ? "vertical" : "horizontal";
        qInfo() << "[Behavior]" << ctrl.id << "->" << info.attributes["orientation"];
    }
    return info;
}

void BehaviorManager::applyBehavior(ControlData& ctrl) const
{
    // fürs erste: nichts tun
    Q_UNUSED(ctrl);
}
