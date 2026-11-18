#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/LocalLevelManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

// ---------------------------------------------------------------------------
// TabManager
//
// A plain C++ singleton (NOT a cocos2d node), so it survives scene replacement.
// Each "tab" owns its own independent GJGameLevel*. We never keep two
// LevelEditorLayer instances alive at once: switching tabs means
//   1) snapshot the currently running editor's level string into the tab
//      that is being left,
//   2) fully recreate the editor scene for the tab being entered via
//      LevelEditorLayer::scene(level, false) + CCDirector::replaceScene.
//
// This mirrors exactly what the game itself does when you exit an editor and
// re-enter one (full teardown + full rebuild), which is why it should not
// hit the half-initialized-state crashes (shouldBlend / updateBlendMode /
// updateVisibility) that happen when you try to swap data on a *running*
// LevelEditorLayer instead of rebuilding it.
// ---------------------------------------------------------------------------
class TabManager {
protected:
    TabManager() = default;

public:
    struct Tab {
        Ref<GJGameLevel> level;
        bool dirty = false;
    };

    std::vector<Tab> m_tabs;
    int m_activeIndex = -1;

    static TabManager& get() {
        static TabManager instance;
        return instance;
    }

    // Called once, the first time the editor opens in a session that has no
    // tabs yet, so the level the player opened normally becomes "Tab 1"
    // instead of being invisible to the tab system.
    void ensureInitialTab(GJGameLevel* currentLevel) {
        if (!m_tabs.empty()) return;
        Tab tab;
        tab.level = currentLevel;
        m_tabs.push_back(tab);
        m_activeIndex = 0;
    }

    int addTab(GJGameLevel* level) {
        Tab tab;
        tab.level = level;
        m_tabs.push_back(tab);
        return static_cast<int>(m_tabs.size()) - 1;
    }

    // Pulls the live editor's current level string into whichever tab is
    // currently marked active, BEFORE we tear the editor down. Without this
    // step, switching away from a tab would silently discard unsaved edits
    // made since the tab was opened.
    void snapshotActiveTabFromRunningEditor() {
        if (m_activeIndex < 0 || m_activeIndex >= static_cast<int>(m_tabs.size())) {
            return;
        }
        auto* editorLayer = LevelEditorLayer::get();
        if (!editorLayer) return;

        auto& tab = m_tabs[m_activeIndex];
        auto currentString = editorLayer->getLevelString();
        if (currentString != tab.level->m_levelString) {
            tab.level->m_levelString = currentString;
            tab.dirty = true;
        }
    }

    bool hasTabs() const {
        return !m_tabs.empty();
    }

    void reset() {
        m_tabs.clear();
        m_activeIndex = -1;
    }
};

// ---------------------------------------------------------------------------
// LevelPickPopup
//
// Minimal list popup over the player's local levels (LocalLevelManager).
// Deliberately NOT using a scroll layer for this first stage: with a lot of
// local levels the list will simply overflow visually rather than crash,
// which is the safer failure mode while this is unverified on-device. A
// scrollable version is the natural follow-up once this baseline works.
// ---------------------------------------------------------------------------
class LevelPickPopup : public geode::Popup<> {
protected:
    bool setup() override {
        this->setTitle("Open Level In New Tab");

        auto winSize = m_mainLayer->getContentSize();
        auto* llm = LocalLevelManager::sharedState();
        auto* levels = llm->m_localLevels;

        auto listMenu = CCMenu::create();
        listMenu->setLayout(
            ColumnLayout::create()
                ->setAxisReverse(true)
                ->setAutoScale(false)
                ->setGap(6.f)
        );

        int count = levels ? levels->count() : 0;
        for (int i = 0; i < count; i++) {
            auto* lvl = static_cast<GJGameLevel*>(levels->objectAtIndex(i));
            if (!lvl) continue;

            auto name = lvl->m_levelName.empty() ? std::string("Unnamed") : lvl->m_levelName;
            auto label = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
            label->setScale(0.5f);

            auto btn = CCMenuItemSpriteExtra::create(
                label, this, menu_selector(LevelPickPopup::onPickLevel)
            );
            btn->setTag(i);
            listMenu->addChild(btn);
        }

        listMenu->updateLayout();
        listMenu->setPosition({ winSize.width / 2.f, winSize.height / 2.f });
        m_mainLayer->addChild(listMenu);

        return true;
    }

    void onPickLevel(CCObject* sender) {
        int idx = static_cast<CCNode*>(sender)->getTag();
        auto* llm = LocalLevelManager::sharedState();
        auto* levels = llm->m_localLevels;
        if (!levels || idx < 0 || idx >= levels->count()) return;

        auto* original = static_cast<GJGameLevel*>(levels->objectAtIndex(idx));
        if (!original) return;

        // duplicate() performs the same deep copy the game's own "Copy Level"
        // button uses, including m_levelString. The copy is NOT written back
        // into LocalLevelManager, so the original local level on disk is
        // never touched just by opening it in a new tab.
        auto* copy = original->duplicate();
        if (!copy) return;

        this->onClose(nullptr);

        auto& mgr = TabManager::get();
        int newIndex = mgr.addTab(copy);
        mgr.snapshotActiveTabFromRunningEditor();
        mgr.m_activeIndex = newIndex;

        auto scene = LevelEditorLayer::scene(copy, false);
        CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
    }

public:
    static LevelPickPopup* create() {
        auto ret = new LevelPickPopup();
        if (ret && ret->initAnchored(320.f, 240.f)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ---------------------------------------------------------------------------
// EditorUI hook: draws the tab bar ("1", "2", ... "+") and wires it up.
// ---------------------------------------------------------------------------
class $modify(MultiLevelEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) {
            return false;
        }

        auto& mgr = TabManager::get();
        mgr.ensureInitialTab(editorLayer->m_level);

        this->rebuildTabBar();
        return true;
    }

    void onSelectTab(CCObject* sender) {
        int idx = static_cast<CCNode*>(sender)->getTag();
        auto& mgr = TabManager::get();

        if (idx == mgr.m_activeIndex) return;
        if (idx < 0 || idx >= static_cast<int>(mgr.m_tabs.size())) return;

        mgr.snapshotActiveTabFromRunningEditor();
        mgr.m_activeIndex = idx;

        auto* targetLevel = mgr.m_tabs[idx].level;
        auto scene = LevelEditorLayer::scene(targetLevel, false);
        CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(0.5f, scene));
    }

    void onAddTab(CCObject*) {
        LevelPickPopup::create()->show();
    }

    void rebuildTabBar() {
        // Remove a previous bar if this EditorUI instance is somehow
        // re-initialized without a fresh node (defensive; should not
        // normally happen since init() only runs once per instance).
        if (auto* existing = this->getChildByID("multi-level-tab-menu"_spr)) {
            existing->removeFromParent();
        }

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto& mgr = TabManager::get();

        auto menu = CCMenu::create();
        menu->setID("multi-level-tab-menu"_spr);
        menu->setLayout(
            RowLayout::create()
                ->setGap(4.f)
                ->setAutoScale(false)
        );

        for (int i = 0; i < static_cast<int>(mgr.m_tabs.size()); i++) {
            auto labelText = std::to_string(i + 1);
            auto label = CCLabelBMFont::create(labelText.c_str(), "bigFont.fnt");
            label->setScale(0.6f);

            auto btn = CCMenuItemSpriteExtra::create(
                label, this, menu_selector(MultiLevelEditorUI::onSelectTab)
            );
            btn->setTag(i);

            // Simple visual highlight for the active tab: slightly larger.
            if (i == mgr.m_activeIndex) {
                btn->setScale(1.15f);
            }

            menu->addChild(btn);
        }

        auto plusLabel = CCLabelBMFont::create("+", "bigFont.fnt");
        plusLabel->setScale(0.6f);
        auto plusBtn = CCMenuItemSpriteExtra::create(
            plusLabel, this, menu_selector(MultiLevelEditorUI::onAddTab)
        );
        menu->addChild(plusBtn);

        menu->updateLayout();
        menu->setPosition({ winSize.width / 2.f, winSize.height - 20.f });
        menu->setZOrder(100);

        this->addChild(menu);
    }
};

$on_mod(Loaded) {
    log::info("Multi-Level Editor loaded (tab bar stage 1).");
}
