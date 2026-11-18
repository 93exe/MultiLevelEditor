
#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <EditorTabAPI.hpp>

using namespace geode::prelude;

class $modify(MultiLevelEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer))
            return false;

        alpha::editor_tabs::addTab(
            "93exe.multilevel-editor/multi-level",
            alpha::editor_tabs::BUILD,

            // Содержимое вкладки
            []() -> CCNode* {
                auto label = CCLabelBMFont::create(
                    "MULTI-LEVEL EDITOR",
                    "bigFont.fnt"
                );

                if (!label)
                    return CCNode::create();

                label->setScale(0.45f);
                return label;
            },

            // Иконка вкладки
            []() -> CCNode* {
                auto icon = CCLabelBMFont::create(
                    "ML",
                    "bigFont.fnt"
                );

                if (icon)
                    icon->setScale(0.5f);

                return icon ? icon : CCNode::create();
            },

            // Срабатывает при открытии и закрытии вкладки
            [](bool state, CCNode*) {
                log::info(
                    "Multi-Level tab state: {}",
                    state ? "opened" : "closed"
                );
            }
        );

        log::info("Multi-Level Editor tab registered!");

        return true;
    }
};
