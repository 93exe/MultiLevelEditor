
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class $modify(MultiLevelEditorUI, EditorUI) {
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer))
            return false;

        auto label = CCLabelBMFont::create("+", "bigFont.fnt");
        auto button = CCMenuItemLabel::create(
            label,
            this,
            menu_selector(MultiLevelEditorUI::onAddTab)
        );

        auto menu = CCMenu::create();
        menu->addChild(button);

        auto size = CCDirector::sharedDirector()->getWinSize();
        menu->setPosition(size.width / 2, size.height - 25);

        this->addChild(menu, 999);

        return true;
    }

    void onAddTab(CCObject*) {
        auto levels = LocalLevelManager::sharedState()->m_localLevels;

        if (!levels || levels->count() == 0) {
            FLAlertLayer::create(
                "Multi-Level Editor",
                "No saved levels found.",
                "OK"
            )->show();
            return;
        }

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto panel = CCNode::create();
        panel->setID("level-list-panel");

        auto menu = CCMenu::create();
        menu->setPosition(winSize.width / 2, winSize.height / 2);

        for (unsigned int i = 0; i < levels->count(); i++) {
            auto level = static_cast<GJGameLevel*>(
                levels->objectAtIndex(i)
            );

            if (!level)
                continue;

            auto text = CCLabelBMFont::create(
                level->m_levelName.c_str(),
                "bigFont.fnt"
            );

            text->setScale(0.35f);

            auto item = CCMenuItemLabel::create(
                text,
                this,
                menu_selector(MultiLevelEditorUI::onSelectLevel)
            );

            item->setTag(i);
            menu->addChild(item);

            item->setPosition(
                0,
                120 - static_cast<float>(i) * 35
            );
        }

        auto closeLabel = CCLabelBMFont::create(
            "CLOSE",
            "bigFont.fnt"
        );

        auto closeButton = CCMenuItemLabel::create(
            closeLabel,
            this,
            menu_selector(MultiLevelEditorUI::onCloseList)
        );

        closeButton->setPosition(0, -150);
        menu->addChild(closeButton);

        panel->addChild(menu);
        this->addChild(panel, 1000);
    }

    void onSelectLevel(CCObject* sender) {
        auto levels = LocalLevelManager::sharedState()->m_localLevels;
        auto index = sender->getTag();

        if (!levels || index < 0 || index >= levels->count())
            return;

        auto source = static_cast<GJGameLevel*>(
            levels->objectAtIndex(index)
        );

        if (!source)
            return;

        auto copy = GJGameLevel::create();

        if (!copy)
            return;

        copy->m_levelName = source->m_levelName;
        copy->m_levelName += " (Tab)";

        copy->m_levelString = source->m_levelString;
        copy->m_audioTrack = source->m_audioTrack;
        copy->m_songID = source->m_songID;
        copy->m_levelLength = source->m_levelLength;
        copy->m_twoPlayerMode = source->m_twoPlayerMode;
        copy->m_lastBuildTab = source->m_lastBuildTab;
        copy->m_lastBuildPage = source->m_lastBuildPage;
        copy->m_lastCameraPos = source->m_lastCameraPos;

        auto scene = LevelEditorLayer::scene(copy, false);

        if (scene) {
            CCDirector::sharedDirector()->replaceScene(
                CCTransitionFade::create(0.3f, scene)
            );
        }
    }

    void onCloseList(CCObject*) {
        auto panel = this->getChildByID("level-list-panel");

        if (panel)
            panel->removeFromParent();
    }
};
