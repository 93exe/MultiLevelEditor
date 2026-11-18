
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class $modify(MultiLevelEditorUI, EditorUI) {
    struct Fields {
        CCMenu* m_tabsMenu = nullptr;
        int m_tabCount = 1;
        int m_activeTab = 0;
    };

    bool init(LevelEditorLayer* level) {
        if (!EditorUI::init(level)) {
            return false;
        }

        auto size = CCDirector::sharedDirector()->getWinSize();

        m_fields->m_tabsMenu = CCMenu::create();
        m_fields->m_tabsMenu->setPosition(
            ccp(size.width / 2, size.height - 24)
        );
        this->addChild(m_fields->m_tabsMenu, 999);

        rebuildTabs();
        return true;
    }

    void rebuildTabs() {
        auto menu = m_fields->m_tabsMenu;
        if (!menu) return;

        menu->removeAllChildrenWithCleanup(true);

        constexpr float spacing = 76.f;
        float firstX = -((m_fields->m_tabCount - 1) * spacing) / 2.f;

        for (int i = 0; i < m_fields->m_tabCount; i++) {
            std::string text = i == m_fields->m_activeTab
                ? "[" + std::to_string(i + 1) + "]"
                : std::to_string(i + 1);

            auto label = CCLabelBMFont::create(
                text.c_str(), "bigFont.fnt"
            );
            if (!label) continue;

            auto item = CCMenuItemLabel::create(
                label,
                this,
                menu_selector(MultiLevelEditorUI::onTabPressed)
            );

            item->setTag(i);
            item->setScale(0.45f);
            item->setPosition(ccp(firstX + i * spacing, 0));
            menu->addChild(item);
        }

        auto plusLabel = CCLabelBMFont::create("+", "bigFont.fnt");
        if (plusLabel) {
            auto plus = CCMenuItemLabel::create(
                plusLabel,
                this,
                menu_selector(MultiLevelEditorUI::onAddTab)
            );

            plus->setScale(0.55f);
            plus->setPosition(
                ccp(firstX + m_fields->m_tabCount * spacing, 0)
            );
            menu->addChild(plus);
        }
    }

    void onTabPressed(CCObject* sender) {
        auto item = static_cast<CCNode*>(sender);
        m_fields->m_activeTab = item->getTag();
        rebuildTabs();
    }

    void onAddTab(CCObject*) {
        m_fields->m_tabCount++;
        m_fields->m_activeTab = m_fields->m_tabCount - 1;
        rebuildTabs();
    }
};
