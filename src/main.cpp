
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class $modify(MultiLevelEditorUI, EditorUI) {
    bool init(LevelEditorLayer* level) {
        if (!EditorUI::init(level)) {
            return false;
        }

        auto label = CCLabelBMFont::create("LEVEL 1", "bigFont.fnt");
        if (!label) {
            return true;
        }

        auto item = CCMenuItemLabel::create(
            label,
            this,
            menu_selector(MultiLevelEditorUI::onTabPressed)
        );

        auto menu = CCMenu::create();
        menu->addChild(item);

        auto size = CCDirector::sharedDirector()->getWinSize();

        menu->setPosition(
            size.width / 2,
            size.height - 24
        );

        this->addChild(menu, 999);

        log::info("Multi-Level Editor: tab UI created");
        return true;
    }

    void onTabPressed(CCObject*) {
        FLAlertLayer::create(
            "Multi-Level Editor",
            "This is the first editor tab. Tab switching is the next step.",
            "OK"
        )->show();
    }
};
