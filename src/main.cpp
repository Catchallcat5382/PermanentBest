#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/NodeIDs.hpp>
#include <Geode/modify/IDManager.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/cocos/extensions/GUI/CCControlExtension/CCScale9Sprite.h>

#if __has_include(<legowiifun.cheat_api/include/cheatAPI.hpp>)
#include <legowiifun.cheat_api/include/cheatAPI.hpp>
#define BESTBAR_HAS_CHEAT_API 1
#else
#define BESTBAR_HAS_CHEAT_API 0
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <cmath>
#include <functional>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

#ifdef GEODE_IS_WINDOWS
#include <windows.h>
#endif

using namespace geode::prelude;
using cocos2d::extension::CCScale9Sprite;

namespace {
    constexpr char const* MOD_ID = "catchallcat5382.best-bar";
    constexpr float BAR_TEXTURE_WIDTH = 1024.0f;
    constexpr float BAR_TEXTURE_HEIGHT = 128.0f;
    constexpr float BAR_FILL_WIDTH = 1024.0f;
    constexpr float BAR_FILL_HEIGHT = 128.0f;

    bool settingEnabled(char const* key, bool fallback = true) {
        auto mod = Mod::get();
        if (!mod || !mod->hasSetting(key)) return fallback;
        return mod->getSettingValue<bool>(key);
    }

    std::string settingString(char const* key, std::string fallback = "") {
        auto mod = Mod::get();
        if (!mod || !mod->hasSetting(key)) return fallback;
        return mod->getSettingValue<std::string>(key);
    }

    std::string normalizeKeyName(std::string value) {
        std::string out;
        out.reserve(value.size());
        for (unsigned char c : value) {
            if (std::isalnum(c)) out.push_back(static_cast<char>(std::toupper(c)));
        }
        return out;
    }

    int virtualKeyFromName(std::string const& rawName) {
        auto name = normalizeKeyName(rawName);
        if (name.empty() || name == "NONE" || name == "DISABLED") return 0;
        if (name.size() == 1) {
            char c = name[0];
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return static_cast<int>(c);
        }
        static const std::unordered_map<std::string, int> map = {
            {"SHIFT", VK_SHIFT}, {"LSHIFT", VK_LSHIFT}, {"LEFTSHIFT", VK_LSHIFT},
            {"RSHIFT", VK_RSHIFT}, {"RIGHTSHIFT", VK_RSHIFT},
            {"CTRL", VK_CONTROL}, {"CONTROL", VK_CONTROL}, {"LCTRL", VK_LCONTROL}, {"RCTRL", VK_RCONTROL},
            {"ALT", VK_MENU}, {"LALT", VK_LMENU}, {"RALT", VK_RMENU},
            {"SPACE", VK_SPACE}, {"TAB", VK_TAB}, {"ENTER", VK_RETURN}, {"RETURN", VK_RETURN},
            {"ESC", VK_ESCAPE}, {"ESCAPE", VK_ESCAPE}, {"BACKSPACE", VK_BACK},
            {"UP", VK_UP}, {"DOWN", VK_DOWN}, {"LEFT", VK_LEFT}, {"RIGHT", VK_RIGHT},
            {"HOME", VK_HOME}, {"END", VK_END}, {"PGUP", VK_PRIOR}, {"PAGEUP", VK_PRIOR},
            {"PGDN", VK_NEXT}, {"PAGEDOWN", VK_NEXT}, {"INSERT", VK_INSERT}, {"DELETE", VK_DELETE},
            {"CAPSLOCK", VK_CAPITAL},
            {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4}, {"F5", VK_F5}, {"F6", VK_F6},
            {"F7", VK_F7}, {"F8", VK_F8}, {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12}
        };
        auto it = map.find(name);
        return it == map.end() ? 0 : it->second;
    }

    std::string keyNameForVirtualKey(int vk) {
        if (!vk) return "None";
        if (vk >= 'A' && vk <= 'Z') return std::string(1, static_cast<char>(vk));
        if (vk >= '0' && vk <= '9') return std::string(1, static_cast<char>(vk));
        static const std::unordered_map<int, std::string> reverse = {
            {VK_SHIFT, "Shift"}, {VK_LSHIFT, "Left Shift"}, {VK_RSHIFT, "Right Shift"},
            {VK_CONTROL, "Ctrl"}, {VK_LCONTROL, "Left Ctrl"}, {VK_RCONTROL, "Right Ctrl"},
            {VK_MENU, "Alt"}, {VK_LMENU, "Left Alt"}, {VK_RMENU, "Right Alt"},
            {VK_SPACE, "Space"}, {VK_TAB, "Tab"}, {VK_RETURN, "Enter"}, {VK_ESCAPE, "Escape"},
            {VK_UP, "Up"}, {VK_DOWN, "Down"}, {VK_LEFT, "Left"}, {VK_RIGHT, "Right"},
            {VK_F1, "F1"}, {VK_F2, "F2"}, {VK_F3, "F3"}, {VK_F4, "F4"}, {VK_F5, "F5"}, {VK_F6, "F6"},
            {VK_F7, "F7"}, {VK_F8, "F8"}, {VK_F9, "F9"}, {VK_F10, "F10"}, {VK_F11, "F11"}, {VK_F12, "F12"}
        };
        auto it = reverse.find(vk);
        return it == reverse.end() ? fmt::format("VK {}", vk) : it->second;
    }

    std::string lowerCopy(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    bool containsAny(std::string const& value, std::initializer_list<char const*> terms) {
        for (auto term : terms) {
            if (value.find(term) != std::string::npos) return true;
        }
        return false;
    }

    std::string cleanField(std::string value) {
        for (auto& ch : value) {
            if (ch == '|' || ch == ';' || ch == '\n' || ch == '\r') ch = ' ';
        }
        return value;
    }

    std::string leaderboardPlayerName() {
        auto name = cleanField(settingString("leaderboard-name", "PLAYER"));
        if (name.empty()) name = "PLAYER";
        if (name.size() > 24) name.resize(24);
        return name;
    }

    std::string leaderboardApiBase() {
        auto url = settingString("leaderboard-api-url", "");
        while (!url.empty() && std::isspace(static_cast<unsigned char>(url.front()))) url.erase(url.begin());
        while (!url.empty() && std::isspace(static_cast<unsigned char>(url.back()))) url.pop_back();
        while (!url.empty() && url.back() == '/') url.pop_back();
        return url;
    }

    std::string shorten(std::string value, size_t limit) {
        if (value.size() <= limit) return value;
        if (limit <= 3) return value.substr(0, limit);
        return value.substr(0, limit - 3) + "...";
    }

    std::string installID() {
        auto mod = Mod::get();
        if (!mod) return "unknown";
        auto id = mod->getSavedValue<std::string>("leaderboard-install-id", "");
        if (!id.empty()) return id;

        auto seed = static_cast<std::uint64_t>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()
        );
        std::mt19937_64 rng(seed ^ static_cast<std::uint64_t>(std::random_device{}()));
        id = fmt::format("{:016x}{:016x}", rng(), rng());
        mod->setSavedValue<std::string>("leaderboard-install-id", id);
        return id;
    }

    struct IntegrityReport {
        bool majorCheatDetected = false;
        std::string reason;
    };

    bool cheatApiReportsCheating() {
#if BESTBAR_HAS_CHEAT_API
        auto loader = Loader::get();
        if (!loader) return false;

        bool apiLoaded = false;
        for (auto loaded : loader->getAllMods()) {
            if (loaded && loaded->isLoaded() && loaded->getID() == "legowiifun.cheat_api") {
                apiLoaded = true;
                break;
            }
        }
        if (!apiLoaded) return false;

        return cheatAPI::isCheating(ROBTOP) || cheatAPI::isCheating();
#else
        return false;
#endif
    }

    IntegrityReport inspectLoadedMods() {
        IntegrityReport report;
        auto loader = Loader::get();
        if (!loader) return report;

        bool apiCheating = cheatApiReportsCheating();
        std::vector<std::string> gameplayMenus;
        std::vector<std::string> exactMatches;

        for (auto loaded : loader->getAllMods()) {
            if (!loaded || loaded == Mod::get() || !loaded->isLoaded()) continue;

            auto text = lowerCopy(fmt::format("{} {}", loaded->getID(), loaded->getName()));

            // A loaded menu by itself is not proof that a disallowed option is active.
            // Keep its name for diagnostics, but do not invalidate only because it exists.
            if (containsAny(text, {
                "eclipse", "prism menu", "gd mega overlay", "openhack"
            })) {
                gameplayMenus.push_back(loaded->getName());
                continue;
            }

            // These are explicitly allowed for ranked runs.
            if (containsAny(text, {
                "hitbox", "show hitboxes", "respawn delay", "restart delay",
                "death delay", "practice music"
            })) {
                continue;
            }

            // Standalone mods whose actual mod name identifies a major assist.
            if (containsAny(text, {
                "noclip", "no-clip", "speedhack", "speed hack", "slowmo",
                "slow motion", "timewarp", "time warp", "frame step",
                "framestep", "autoclick", "auto click", "macro", "teleport",
                "checkpoint bypass", "clickbot", "replay bot"
            })) {
                exactMatches.push_back(loaded->getName());
            }
        }

        if (!exactMatches.empty()) {
            report.majorCheatDetected = true;
            std::ostringstream joined;
            for (size_t i = 0; i < exactMatches.size(); ++i) {
                if (i) joined << ", ";
                joined << exactMatches[i];
            }
            report.reason = "Active major assist mod: " + joined.str();
            return report;
        }

        if (apiCheating) {
            report.majorCheatDetected = true;
            std::ostringstream reason;
            reason << "Cheat API reports an active disallowed gameplay cheat";
            if (!gameplayMenus.empty()) {
                reason << " from ";
                for (size_t i = 0; i < gameplayMenus.size(); ++i) {
                    if (i) reason << ", ";
                    reason << gameplayMenus[i];
                }
            }
            reason << "; the API does not expose the individual toggle name";
            report.reason = reason.str();
        }
        return report;
    }

    std::string difficultyName(GJGameLevel* level) {
        if (!level) return "UNKNOWN";
        if (level->m_autoLevel) return "AUTO";

        bool isDemon = level->m_demon.value() != 0;
        if (isDemon) {
            switch (level->m_demonDifficulty) {
                case 3: return "EASY DEMON";
                case 4: return "MEDIUM DEMON";
                case 5: return "INSANE DEMON";
                case 6: return "EXTREME DEMON";
                default: return "HARD DEMON";
            }
        }

        switch (static_cast<int>(level->m_difficulty)) {
            case 1: return "EASY";
            case 2: return "NORMAL";
            case 3: return "HARD";
            case 4: return "HARDER";
            case 5: return "INSANE";
            case 6: return "DEMON";
            default: break;
        }

        int rewards = std::max(0, level->m_stars.value());
        if (rewards <= 0) return "UNRATED";
        if (rewards <= 2) return "EASY";
        if (rewards <= 3) return "NORMAL";
        if (rewards <= 5) return "HARD";
        if (rewards <= 7) return "HARDER";
        if (rewards <= 9) return "INSANE";
        return "DEMON";
    }

    std::string numericLevelID(GJGameLevel* level) {
        if (!level) return "UNKNOWN";
        auto id = level->m_levelID.value();
        return id != 0 ? fmt::format("{}", id) : "LOCAL";
    }

    struct LocalLeaderboardRun {
        std::string level;
        std::string player;
        int timeMs = 0;
        bool valid = false;
        std::string status;
        std::string difficulty = "UNKNOWN";
        std::string levelId = "UNKNOWN";
        int rewards = 0;
        bool platformer = true;
    };

    std::string runDetails(LocalLeaderboardRun const& run) {
        auto rewardName = run.platformer ? "MOONS" : "STARS";
        if (run.rewards > 0) {
            return fmt::format(
                "{}  |  {} {}  |  ID {}",
                run.difficulty.empty() ? "UNKNOWN" : run.difficulty,
                run.rewards,
                rewardName,
                run.levelId.empty() ? "UNKNOWN" : run.levelId
            );
        }
        return fmt::format(
            "{}  |  {}  |  ID {}",
            run.difficulty.empty() ? "UNKNOWN" : run.difficulty,
            run.platformer ? "PLATFORMER" : "CLASSIC",
            run.levelId.empty() ? "UNKNOWN" : run.levelId
        );
    }

    std::vector<LocalLeaderboardRun> readLocalRuns() {
        std::vector<LocalLeaderboardRun> runs;
        auto mod = Mod::get();
        if (!mod) return runs;

        auto raw = mod->getSavedValue<std::string>("leaderboard-local-runs", "");
        std::stringstream stream(raw);
        std::string row;
        while (std::getline(stream, row, ';')) {
            if (row.empty()) continue;

            std::vector<std::string> values;
            std::stringstream fields(row);
            std::string value;
            while (std::getline(fields, value, '|')) values.push_back(value);
            if (values.size() < 5) continue;

            LocalLeaderboardRun run;
            run.level = values[0];
            run.player = values[1];
            try {
                run.timeMs = std::max(0, std::stoi(values[2]));
            }
            catch (...) {
                continue;
            }
            run.valid = values[3] == "1";
            run.status = values[4];
            if (values.size() >= 6 && !values[5].empty()) run.difficulty = values[5];
            if (values.size() >= 7 && !values[6].empty()) run.levelId = values[6];
            if (values.size() >= 8) {
                try {
                    run.rewards = std::max(0, std::stoi(values[7]));
                }
                catch (...) {
                    run.rewards = 0;
                }
            }
            if (values.size() >= 9) run.platformer = values[8] != "0";
            runs.push_back(std::move(run));
        }

        std::stable_sort(runs.begin(), runs.end(), [](auto const& a, auto const& b) {
            if (a.valid != b.valid) return a.valid > b.valid;
            if (a.timeMs <= 0) return false;
            if (b.timeMs <= 0) return true;
            return a.timeMs < b.timeMs;
        });
        return runs;
    }

    void appendLocalRun(
        GJGameLevel* level,
        int timeMs,
        bool valid,
        std::string const& status
    ) {
        if (!level) return;
        auto mod = Mod::get();
        if (!mod) return;

        auto runs = readLocalRuns();
        runs.push_back({
            cleanField(level->m_levelName.c_str()),
            leaderboardPlayerName(),
            std::max(timeMs, 0),
            valid,
            cleanField(status),
            difficultyName(level),
            numericLevelID(level),
            std::max(0, level->m_stars.value()),
            level->isPlatformer()
        });

        if (runs.size() > 60) {
            runs.erase(runs.begin(), runs.begin() + static_cast<std::ptrdiff_t>(runs.size() - 60));
        }

        std::ostringstream out;
        for (size_t i = 0; i < runs.size(); ++i) {
            if (i) out << ';';
            out << cleanField(runs[i].level) << '|'
                << cleanField(runs[i].player) << '|'
                << runs[i].timeMs << '|'
                << (runs[i].valid ? "1" : "0") << '|'
                << cleanField(runs[i].status) << '|'
                << cleanField(runs[i].difficulty) << '|'
                << cleanField(runs[i].levelId) << '|'
                << runs[i].rewards << '|'
                << (runs[i].platformer ? "1" : "0");
        }
        mod->setSavedValue<std::string>("leaderboard-local-runs", out.str());
    }

    ccColor3B bestColor() {
        auto mod = Mod::get();
        if (!mod) return {255, 215, 64};
        return mod->getSettingValue<ccColor3B>("best-color");
    }

    void playBestSound() {
        if (!settingEnabled("best-sound")) return;
        auto mod = Mod::get();
        if (!mod) return;

        auto sound = (mod->getResourcesDir() / "bestbar-shine.wav").string();
        FMODAudioEngine::sharedEngine()->playEffect(sound.c_str());
    }


    std::string stableLevelSuffix(GJGameLevel* level) {
        if (!level) return "unknown";

        auto id = level->m_levelID.value();
        if (id != 0) return fmt::format("id-{}", id);

        std::string material = level->m_levelName.c_str();
        material += "|";
        material += level->m_levelString.c_str();
        return fmt::format("local-{:016x}", std::hash<std::string>{}(material));
    }

    std::string percentKey(GJGameLevel* level) {
        return fmt::format("best-percent-{}", stableLevelSuffix(level));
    }

    std::string normalTimeKey(GJGameLevel* level) {
        return fmt::format("best-time-{}", stableLevelSuffix(level));
    }

    std::string speedrunBestKey(GJGameLevel* level) {
        return fmt::format("gold-run-best-{}", stableLevelSuffix(level));
    }

    std::string speedrunLastKey(GJGameLevel* level) {
        return fmt::format("gold-run-last-{}", stableLevelSuffix(level));
    }

    std::string speedrunRunsKey(GJGameLevel* level) {
        return fmt::format("gold-run-count-{}", stableLevelSuffix(level));
    }

    std::string speedrunFailuresKey(GJGameLevel* level) {
        return fmt::format("gold-run-failures-{}", stableLevelSuffix(level));
    }

    std::string speedrunFinishesKey(GJGameLevel* level) {
        return fmt::format("gold-run-finishes-{}", stableLevelSuffix(level));
    }

    std::string speedrunHistoryKey(GJGameLevel* level) {
        return fmt::format("gold-run-history-{}", stableLevelSuffix(level));
    }

    std::string formatTime(int totalMilliseconds) {
        if (totalMilliseconds <= 0) return "--:--.---";

        auto hours = totalMilliseconds / 3'600'000;
        auto minutes = (totalMilliseconds / 60'000) % 60;
        auto seconds = (totalMilliseconds / 1'000) % 60;
        auto milliseconds = totalMilliseconds % 1'000;

        if (hours > 0) {
            return fmt::format("{}:{:02}:{:02}.{:03}", hours, minutes, seconds, milliseconds);
        }
        return fmt::format("{}:{:02}.{:03}", minutes, seconds, milliseconds);
    }

    void appendSpeedrunHistory(GJGameLevel* level, bool completed, int timeMs) {
        if (!level || !settingEnabled("save-run-history")) return;

        auto mod = Mod::get();
        auto key = speedrunHistoryKey(level);
        auto history = mod->getSavedValue<std::string>(key, "");
        auto entry = fmt::format("{}:{}", completed ? "C" : "F", std::max(timeMs, 0));

        if (!history.empty()) history += ";";
        history += entry;

        int separators = static_cast<int>(std::count(history.begin(), history.end(), ';'));
        while (separators >= 20) {
            auto split = history.find(';');
            if (split == std::string::npos) break;
            history.erase(0, split + 1);
            --separators;
        }

        mod->setSavedValue<std::string>(key, history);
    }

    CCLabelBMFont* findLabelContaining(CCNode* root, std::string const& needle) {
        if (!root) return nullptr;

        if (auto label = typeinfo_cast<CCLabelBMFont*>(root)) {
            auto value = std::string(label->getString() ? label->getString() : "");
            if (value.find(needle) != std::string::npos) return label;
        }

        auto children = root->getChildren();
        if (!children) return nullptr;
        for (unsigned int i = 0; i < children->count(); ++i) {
            auto child = typeinfo_cast<CCNode*>(children->objectAtIndex(i));
            if (!child) continue;
            if (auto found = findLabelContaining(child, needle)) return found;
        }
        return nullptr;
    }

    void centerNodeOnScreenX(CCNode* node) {
        if (!node || !node->getParent()) return;
        auto win = CCDirector::get()->getWinSize();
        auto world = node->getParent()->convertToWorldSpace(node->getPosition());
        world.x = win.width * 0.5f;
        node->setPosition(node->getParent()->convertToNodeSpace(world));
    }

    constexpr char const* PENDING_GOLD_RUN_KEY = "pending-platformer-gold-run";
    constexpr char const* LAYOUT_DEBUG_FLAG = "BestBarLayoutDebug.flag";

    bool layoutDebugEnabled() {
#ifdef GEODE_IS_WINDOWS
        char* tempValue = nullptr;
        size_t tempLength = 0;
        if (_dupenv_s(&tempValue, &tempLength, "TEMP") != 0 || !tempValue || !*tempValue) {
            if (tempValue) std::free(tempValue);
            tempValue = nullptr;
            tempLength = 0;
            _dupenv_s(&tempValue, &tempLength, "TMP");
        }
        if (!tempValue || !*tempValue) {
            if (tempValue) std::free(tempValue);
            return false;
        }
        auto enabled = std::filesystem::exists(std::filesystem::path(tempValue) / LAYOUT_DEBUG_FLAG);
        std::free(tempValue);
        return enabled;
#else
        auto temp = std::getenv("TMPDIR");
        if (!temp || !*temp) temp = "/tmp";
        return std::filesystem::exists(std::filesystem::path(temp) / LAYOUT_DEBUG_FLAG);
#endif
    }

}

class BestBarDebugDragLayer : public CCLayer {
protected:
    std::function<CCRect()> m_rectProvider;
    std::function<void(CCPoint, CCPoint)> m_moveCallback;
    CCPoint m_lastPoint;
    bool m_dragging = false;
    CCLabelBMFont* m_label = nullptr;

public:
    static BestBarDebugDragLayer* create(
        std::function<CCRect()> rectProvider,
        std::function<void(CCPoint, CCPoint)> moveCallback
    ) {
        auto ret = new BestBarDebugDragLayer();
        if (ret && ret->init()) {
            ret->m_rectProvider = std::move(rectProvider);
            ret->m_moveCallback = std::move(moveCallback);
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        setID("best-bar-layout-debug-layer");
        setTouchEnabled(true);
        setTouchMode(kCCTouchesOneByOne);
        setTouchPriority(-5000);

        auto win = CCDirector::get()->getWinSize();
        m_label = CCLabelBMFont::create(
            "LAYOUT DEBUG: DRAG THE GOLD BAR / DIAMOND",
            "bigFont.fnt"
        );
        if (m_label) {
            m_label->setScale(0.24f);
            m_label->setColor({255, 235, 85});
            m_label->setPosition({win.width * 0.5f, win.height - 48.0f});
            addChild(m_label, 2);
        }
        return true;
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!layoutDebugEnabled() || !touch || !m_rectProvider) return false;
        auto point = touch->getLocation();
        auto rect = m_rectProvider();
        rect.origin.x -= 16.0f;
        rect.origin.y -= 16.0f;
        rect.size.width += 32.0f;
        rect.size.height += 32.0f;
        if (!rect.containsPoint(point)) return false;
        m_dragging = true;
        m_lastPoint = point;
        if (m_label) m_label->setString("DRAGGING - RELEASE TO SAVE");
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging || !touch || !m_moveCallback) return;
        auto point = touch->getLocation();
        m_moveCallback(m_lastPoint, point);
        m_lastPoint = point;
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        m_dragging = false;
        if (m_label) m_label->setString("SAVED - DRAG AGAIN OR CLOSE GD");
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_dragging = false;
    }
};

class $modify(BestBarPlayLayer, PlayLayer) {
public:
    struct Fields {
        bool setupFinished = false;
        bool uiReady = false;
        bool flashedThisAttempt = false;
        bool completionHandled = false;
        bool bestChaseActive = false;
        bool goldVisualActive = false;
        bool pendingGoldShine = false;
        bool diamondWasGold = false;
        bool bestStatusCompletedShown = false;
        bool progressFillOpacityCaptured = false;

        bool goldRunMode = false;
        bool goldRunStarted = false;
        bool goldRunCompleted = false;
        bool runCheated = false;
        bool runSafeMode = true;
        float nextIntegrityScanSeconds = 0.0f;
        std::string runIntegrityReason;

        int attemptStartBestPercent = 0;
        int shownBestPercent = 0;
        int currentPercent = 0;
        float currentProgressRatio = 0.0f;
        int lastAnimatedPercent = -1;
        int savedBestTimeMs = 0;
        int currentTimeMs = 0;

        int goldRunBestTimeMs = 0;
        int goldRunLastTimeMs = 0;
        int goldRunRuns = 0;
        int goldRunFailures = 0;
        int goldRunFinishes = 0;

        float bestBarWidth = 0.0f;
        float bestBarHeight = 0.0f;
        float bestBarY = 0.0f;
        float goldFillSourceWidth = BAR_TEXTURE_WIDTH;
        float goldFillSourceHeight = BAR_TEXTURE_HEIGHT;
        float goldGlowSourceWidth = BAR_TEXTURE_WIDTH;
        float goldGlowSourceHeight = BAR_TEXTURE_HEIGHT;
        float goldMaskSourceWidth = BAR_FILL_WIDTH;
        float goldMaskSourceHeight = BAR_FILL_HEIGHT;
        float goldFillBaseScaleX = 1.0f;
        float goldFillBaseScaleY = 1.0f;
        float goldGlowBaseScaleX = 1.0f;
        float goldGlowBaseScaleY = 1.0f;
        float debugOffsetX = 0.0f;
        float debugOffsetY = 0.0f;
        float currentGoldWidth = 0.0f;
        float currentGoldHeight = 0.0f;
        ccColor3B originalPercentColor = ccWHITE;
        GLubyte originalProgressFillOpacity = 255;
        float originalPercentScale = 0.5f;
        std::string pendingBanner;

        CCNode* currentGoldRoot = nullptr;
        CCClippingNode* currentGoldClip = nullptr;
        CCSprite* currentGoldFill = nullptr;
        CCSprite* currentGoldShine = nullptr;
        CCSprite* currentGoldGlow = nullptr;
        float currentGoldRatio = 0.0f;

        CCNode* currentDiamondRoot = nullptr;
        CCSprite* currentDiamondNormal = nullptr;
        CCSprite* currentDiamondGold = nullptr;
        CCSprite* currentDiamondGlow = nullptr;

        CCNode* bestMarkerRoot = nullptr;
        CCSprite* bestMarker = nullptr;
        CCSprite* completionCheck = nullptr;
        CCSprite* markerSpark = nullptr;
        int lastMarkerPercent = -1;
        bool completionCheckShown = false;

        CCNode* bestStatusRoot = nullptr;
        CCSprite* bestStatusX = nullptr;
        CCSprite* bestStatusCheck = nullptr;
        CCSprite* bestStatusSpark = nullptr;

        CCNodeRGBA* bestBarRoot = nullptr;
        CCNode* sparkleRoot = nullptr;
        CCClippingNode* bestBarClip = nullptr;
        CCSprite* bestBarGlow = nullptr;
        CCSprite* bestBarFill = nullptr;
        CCSprite* bestBarShine = nullptr;
        CCSprite* bestBarMask = nullptr;
        CCSprite* bestBarTrail = nullptr;
        CCSprite* tipSpark = nullptr;
        std::array<CCSprite*, 8> ambientSparkles {};

        CCLayerColor* flashLayer = nullptr;
        CCLayerColor* shineBand = nullptr;
        CCSprite* screenShine = nullptr;
        CCNode* burstRoot = nullptr;
        std::array<CCSprite*, 8> burstSparkles {};

        bool settingsKeyDownLast = false;

        CCLabelBMFont* bestLabel = nullptr;
        CCLabelBMFont* goldRunTimerLabel = nullptr;
        CCLabelBMFont* goldRunBestLabel = nullptr;
        CCLabelBMFont* goldRunStatsLabel = nullptr;
        CCLabelBMFont* bannerLabel = nullptr;

        async::TaskHolder<web::WebResponse> leaderboardSubmitTask;
    };

    bool isPlatformerLevel() const {
        return m_level && m_level->isPlatformer();
    }

    bool supportsGoldRun() const {
        return isPlatformerLevel();
    }

    bool isGoldRunMode() {
        return m_fields->goldRunMode;
    }

    int getConfiguredSettingsUIKey() const {
        return virtualKeyFromName(settingString("settings-ui-key", ""));
    }

    void openQuickSettingsUI() {
        auto body = fmt::format(
            "<cy>Settings UI key:</c> {}\n\n<cg>Gold Run:</c> {}\n\nChange settings from the <cy>Geode mod settings</c> page for Best Bar.",
            keyNameForVirtualKey(getConfiguredSettingsUIKey()),
            m_fields->goldRunMode ? "ON" : "OFF"
        );
        FLAlertLayer::create("Best Bar Settings", body.c_str(), "OK")->show();
    }

    void toggleGoldRunMode() {
        if (!isPlatformerLevel() || !settingEnabled("gold-run-mode")) return;

        if (m_fields->goldRunMode) {
            finalizeGoldRunFailureIfNeeded();
            m_fields->goldRunMode = false;
            m_fields->pendingBanner = "GOLD RUN OFF";
            resetLevel();
            return;
        }

        auto integrity = inspectLoadedMods();
        bool safeMode = settingEnabled("leaderboard-safe-mode", true);

        m_fields->runSafeMode = safeMode;
        m_fields->runCheated = integrity.majorCheatDetected;
        m_fields->runIntegrityReason = m_fields->runCheated
            ? integrity.reason
            : (safeMode ? "SAFE MODE" : "VALID");
        m_fields->goldRunMode = true;
        m_fields->pendingBanner = m_fields->runCheated
            ? fmt::format("INVALID: {}", shorten(m_fields->runIntegrityReason, 34))
            : "GOLD RUN ON";
        resetLevel();
    }

private:
    bool shouldShow() const {
        if (!settingEnabled("enabled")) return false;
        if (m_isPracticeMode && !settingEnabled("practice-mode", false)) return false;
        if (m_isTestMode && !settingEnabled("test-mode", false)) return false;
        return true;
    }

    bool canSaveRecord() const {
        return !m_isPracticeMode && !m_isTestMode;
    }

    void restorePercentLabel() {
        if (!m_percentageLabel) return;
        m_percentageLabel->setColor(m_fields->originalPercentColor);
        m_percentageLabel->setScale(m_fields->originalPercentScale);
    }

    void colorCurrentLabel(bool gold) {
        if (!m_percentageLabel) return;
        if (!settingEnabled("gold-current-label")) {
            restorePercentLabel();
            return;
        }
        m_percentageLabel->setColor(gold ? bestColor() : m_fields->originalPercentColor);
    }

    float readLiveProgressRatio() {
        float ratio = static_cast<float>(std::clamp(PlayLayer::getCurrentPercentInt(), 0, 100)) / 100.0f;
        if (!m_percentageLabel || !m_percentageLabel->getString()) return ratio;

        auto text = std::string(m_percentageLabel->getString());
        if (text.find('%') == std::string::npos) return ratio;

        char* end = nullptr;
        float value = std::strtof(text.c_str(), &end);
        if (end == text.c_str() || !std::isfinite(value)) return ratio;
        return std::clamp(value / 100.0f, 0.0f, 1.0f);
    }

    void syncLiveProgress() {
        m_fields->currentProgressRatio = readLiveProgressRatio();
        m_fields->currentPercent = std::clamp(
            static_cast<int>(std::lround(m_fields->currentProgressRatio * 100.0f)),
            0,
            100
        );
    }

    void startAmbientSparkle(CCSprite* sparkle, float delay, float scale) {
        if (!sparkle) return;
        sparkle->setOpacity(0);
        sparkle->setScale(scale * 0.55f);
        sparkle->runAction(CCRepeatForever::create(CCSequence::create(
            CCDelayTime::create(delay),
            CCSpawn::create(
                CCFadeIn::create(0.16f),
                CCScaleTo::create(0.16f, scale),
                CCRotateBy::create(0.34f, 55.0f),
                nullptr
            ),
            CCDelayTime::create(0.20f),
            CCSpawn::create(
                CCFadeOut::create(0.28f),
                CCScaleTo::create(0.28f, scale * 0.45f),
                CCRotateBy::create(0.28f, 35.0f),
                nullptr
            ),
            CCDelayTime::create(0.55f),
            nullptr
        )));
    }

    void startBarShineLoop(CCSprite* shine, float delay = 0.55f) {
        if (!shine) return;
        shine->setPosition({-650.0f, 0.0f});
        shine->setOpacity(0);
        shine->runAction(CCRepeatForever::create(CCSequence::create(
            CCDelayTime::create(delay),
            CCFadeIn::create(0.07f),
            CCMoveTo::create(0.58f, {650.0f, 0.0f}),
            CCFadeOut::create(0.10f),
            CCMoveTo::create(0.0f, {-650.0f, 0.0f}),
            CCDelayTime::create(0.72f),
            nullptr
        )));
    }

    void updateBestBarLayout() {
        if (!m_progressBar) return;

        auto parent = m_progressBar->getParent();
        if (!parent) return;

        // IMPORTANT: m_progressFill changes width and center while the level is
        // being played, so it must never be used as the layout anchor. Anchor
        // everything to the stock progress-bar frame, which is stationary.
        auto frameBox = m_progressBar->boundingBox();
        float frameWidth = std::abs(frameBox.size.width);
        float frameHeight = std::abs(frameBox.size.height);
        if (frameWidth < 40.0f) frameWidth = std::max(m_progressWidth, 140.0f);
        if (frameHeight < 4.0f) frameHeight = std::max(m_progressHeight, 10.0f);

        CCPoint barCenter {frameBox.getMidX(), frameBox.getMidY()};
        constexpr float GOLD_FILL_Y_OFFSET = -0.45f;
        constexpr float GOLD_FILL_HEIGHT_FACTOR = 0.40f;

        // Fit the gold texture inside the white stock frame on every resolution.
        // These are small border insets relative to the real frame dimensions,
        // not screen coordinates.
        float horizontalInset = std::clamp(frameHeight * 0.25f, 2.0f, 4.5f);
        float verticalInset = std::clamp(frameHeight * 0.16f, 1.2f, 2.8f);
        float innerWidth = std::max(frameWidth - horizontalInset * 2.0f, 40.0f);
        float innerHeight = std::max(frameHeight - verticalInset * 2.0f, 5.0f);

        m_fields->bestBarWidth = innerWidth;
        m_fields->bestBarHeight = innerHeight;
        m_fields->bestBarY = barCenter.y;

        if (m_fields->bestBarRoot) {
            m_fields->bestBarRoot->setPosition({barCenter.x, barCenter.y + GOLD_FILL_Y_OFFSET});
            m_fields->bestBarRoot->setScaleX(
                innerWidth / std::max(m_fields->goldMaskSourceWidth, 1.0f)
            );
            m_fields->bestBarRoot->setScaleY(
                (innerHeight * GOLD_FILL_HEIGHT_FACTOR) /
                std::max(m_fields->goldMaskSourceHeight, 1.0f)
            );
        }

        if (m_fields->currentGoldRoot) m_fields->currentGoldRoot->setVisible(false);
        if (m_fields->currentGoldGlow) m_fields->currentGoldGlow->setVisible(false);

        if (m_fields->currentDiamondRoot) {
            float ratio = std::clamp(m_fields->currentProgressRatio, 0.0f, 1.0f);
            float x = barCenter.x - innerWidth * 0.5f + innerWidth * ratio;
            m_fields->currentDiamondRoot->setPosition({x, barCenter.y + 0.35f});
            m_fields->currentDiamondRoot->setScale(
                std::clamp(frameHeight / 34.0f, 0.38f, 0.56f)
            );
        }

        if (m_fields->sparkleRoot) {
            m_fields->sparkleRoot->setPosition(barCenter);
            const std::array<float, 8> fractions = {
                0.045f, 0.16f, 0.29f, 0.43f, 0.58f, 0.72f, 0.85f, 0.965f
            };
            const std::array<float, 8> yOffsets = {
                -6.0f, 8.0f, -8.0f, 6.0f, -6.0f, 8.0f, -7.0f, 6.0f
            };
            for (size_t i = 0; i < m_fields->ambientSparkles.size(); ++i) {
                if (!m_fields->ambientSparkles[i]) continue;
                m_fields->ambientSparkles[i]->setPosition({
                    -innerWidth * 0.5f + innerWidth * fractions[i],
                    yOffsets[i]
                });
            }
        }

        if (m_fields->burstRoot) m_fields->burstRoot->setPosition(barCenter);

        if (m_fields->bestLabel) {
            m_fields->bestLabel->setPosition({
                barCenter.x,
                barCenter.y - frameHeight * 0.5f - 8.0f
            });
        }
    }

    void updateBestStatus(bool show, bool completed) {
        if (!m_fields->bestStatusRoot || !m_fields->bestLabel) return;

        m_fields->bestStatusRoot->setVisible(show);
        if (!show) return;

        if (auto shadowX = typeinfo_cast<CCSprite*>(m_fields->bestStatusRoot->getChildByID("best-bar-status-x-shadow"))) {
            shadowX->setVisible(!completed);
        }
        if (m_fields->bestStatusX) m_fields->bestStatusX->setVisible(!completed);
        if (auto shadowCheck = typeinfo_cast<CCSprite*>(m_fields->bestStatusRoot->getChildByID("best-bar-status-check-shadow"))) {
            shadowCheck->setVisible(completed);
        }
        if (m_fields->bestStatusCheck) m_fields->bestStatusCheck->setVisible(completed);
        if (m_fields->bestStatusSpark) {
            m_fields->bestStatusSpark->setVisible(
                completed && settingEnabled("animated-sparkles")
            );
        }

        auto labelPos = m_fields->bestLabel->getPosition();
        float labelWidth =
            m_fields->bestLabel->getContentSize().width * std::abs(m_fields->bestLabel->getScaleX());
        auto win = CCDirector::get()->getWinSize();
        float statusX = labelPos.x + labelWidth * 0.5f + 11.8f;
        statusX = std::clamp(statusX, 15.0f, win.width - 15.0f);
        m_fields->bestStatusRoot->setPosition({statusX, labelPos.y + 0.2f});
        m_fields->bestStatusRoot->setScale(0.40f);

        if (completed && !m_fields->bestStatusCompletedShown && m_fields->bestStatusCheck) {
            m_fields->bestStatusCompletedShown = true;
            m_fields->bestStatusCheck->stopAllActions();
            m_fields->bestStatusCheck->setOpacity(0);
            m_fields->bestStatusCheck->setScale(0.42f);
            m_fields->bestStatusCheck->setRotation(-10.0f);
            m_fields->bestStatusCheck->runAction(CCSpawn::create(
                CCFadeIn::create(0.10f),
                CCEaseBackOut::create(CCScaleTo::create(0.28f, 1.0f)),
                CCRotateTo::create(0.20f, 0.0f),
                nullptr
            ));
        }
        else if (!completed) {
            m_fields->bestStatusCompletedShown = false;
            if (m_fields->bestStatusCheck) {
                m_fields->bestStatusCheck->setOpacity(255);
                m_fields->bestStatusCheck->setScale(1.0f);
                m_fields->bestStatusCheck->setRotation(0.0f);
            }
        }
    }

    void createBestBarUI() {
        if (m_fields->uiReady || !m_progressBar || !m_percentageLabel) return;

        m_fields->originalPercentColor = m_percentageLabel->getColor();
        m_fields->originalPercentScale = m_percentageLabel->getScale();
        auto color = bestColor();
        auto fill = m_progressFill ? m_progressFill : m_progressBar;
        auto parent = m_progressBar ? m_progressBar->getParent() : nullptr;
        if (!fill || !parent) return;
        auto baseZ = std::max(fill->getZOrder(), m_progressBar->getZOrder());
        if (!m_fields->progressFillOpacityCaptured) {
            m_fields->originalProgressFillOpacity = fill->getOpacity();
            m_fields->progressFillOpacityCaptured = true;
        }

        // Intentionally do not create the old partial "bar following the diamond".
        // The only active replacement is the real stock progress fill below.
        m_fields->currentGoldRoot = nullptr;
        m_fields->currentGoldGlow = nullptr;
        m_fields->currentGoldClip = nullptr;
        m_fields->currentGoldFill = nullptr;
        m_fields->currentGoldShine = nullptr;

        // Exact conditional gold fill. It is hidden normally and replaces the
        // stock red fill only in Gold Run, after passing attempt-start best,
        // or at 100%.
        m_fields->bestBarRoot = CCNodeRGBA::create();
        m_fields->bestBarRoot->setID("best-bar-root");
        m_fields->bestBarRoot->setCascadeOpacityEnabled(true);
        m_fields->bestBarRoot->setVisible(false);
        parent->addChild(m_fields->bestBarRoot, baseZ + 10);

        m_fields->bestBarGlow = CCSprite::create("bestbar-glow.png"_spr);
        if (m_fields->bestBarGlow) {
            auto glowSize = m_fields->bestBarGlow->getContentSize();
            m_fields->goldGlowSourceWidth = std::max(glowSize.width, 1.0f);
            m_fields->goldGlowSourceHeight = std::max(glowSize.height, 1.0f);
            m_fields->bestBarGlow->setID("best-bar-glow");
            m_fields->bestBarGlow->setColor(color);
            m_fields->bestBarGlow->setAnchorPoint({0.0f, 0.5f});
            m_fields->bestBarGlow->setOpacity(132);
            m_fields->bestBarGlow->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
            m_fields->bestBarGlow->runAction(CCRepeatForever::create(CCSequence::create(
                CCFadeTo::create(0.62f, 175),
                CCFadeTo::create(0.72f, 82),
                nullptr
            )));
            m_fields->bestBarRoot->addChild(m_fields->bestBarGlow, -2);
        }

        m_fields->bestBarFill = CCSprite::create("bestbar-gold-fill.png"_spr);
        if (m_fields->bestBarFill) {
            auto fillSize = m_fields->bestBarFill->getContentSize();
            m_fields->goldFillSourceWidth = std::max(fillSize.width, 1.0f);
            m_fields->goldFillSourceHeight = std::max(fillSize.height, 1.0f);
            m_fields->bestBarFill->setID("best-bar-real-gold-fill");
            m_fields->bestBarFill->setColor(ccWHITE);
            m_fields->bestBarFill->setAnchorPoint({0.0f, 0.5f});
            m_fields->bestBarRoot->addChild(m_fields->bestBarFill, 0);
        }

        m_fields->bestBarMask = CCSprite::create("bestbar-mask.png"_spr);
        if (m_fields->bestBarMask) {
            auto maskSize = m_fields->bestBarMask->getContentSize();
            m_fields->goldMaskSourceWidth = std::max(maskSize.width, 1.0f);
            m_fields->goldMaskSourceHeight = std::max(maskSize.height, 1.0f);
            m_fields->bestBarMask->setAnchorPoint({0.0f, 0.5f});
            m_fields->bestBarMask->setPosition({-m_fields->goldMaskSourceWidth * 0.5f, 0.0f});

            m_fields->goldFillBaseScaleX = m_fields->goldMaskSourceWidth /
                std::max(m_fields->goldFillSourceWidth, 1.0f);
            m_fields->goldFillBaseScaleY = m_fields->goldMaskSourceHeight /
                std::max(m_fields->goldFillSourceHeight, 1.0f);
            m_fields->goldGlowBaseScaleX = m_fields->goldMaskSourceWidth /
                std::max(m_fields->goldGlowSourceWidth, 1.0f);
            m_fields->goldGlowBaseScaleY = m_fields->goldMaskSourceHeight /
                std::max(m_fields->goldGlowSourceHeight, 1.0f);

            if (m_fields->bestBarFill) {
                m_fields->bestBarFill->setPosition({-m_fields->goldMaskSourceWidth * 0.5f, 0.0f});
                m_fields->bestBarFill->setScaleX(m_fields->goldFillBaseScaleX);
                m_fields->bestBarFill->setScaleY(m_fields->goldFillBaseScaleY);
            }
            if (m_fields->bestBarGlow) {
                m_fields->bestBarGlow->setPosition({-m_fields->goldMaskSourceWidth * 0.5f, 0.0f});
                m_fields->bestBarGlow->setScaleX(m_fields->goldGlowBaseScaleX);
                m_fields->bestBarGlow->setScaleY(m_fields->goldGlowBaseScaleY * 1.02f);
            }
        }
        m_fields->bestBarClip = m_fields->bestBarMask
            ? CCClippingNode::create(m_fields->bestBarMask)
            : nullptr;
        if (m_fields->bestBarClip) {
            m_fields->bestBarClip->setID("best-bar-fill-clip");
            m_fields->bestBarClip->setAlphaThreshold(0.05f);
            m_fields->bestBarRoot->addChild(m_fields->bestBarClip, 2);

            if (m_fields->bestBarGlow) {
                m_fields->bestBarGlow->retain();
                m_fields->bestBarGlow->removeFromParentAndCleanup(false);
                m_fields->bestBarClip->addChild(m_fields->bestBarGlow, -2);
                m_fields->bestBarGlow->release();
            }
            if (m_fields->bestBarFill) {
                m_fields->bestBarFill->retain();
                m_fields->bestBarFill->removeFromParentAndCleanup(false);
                m_fields->bestBarClip->addChild(m_fields->bestBarFill, 0);
                m_fields->bestBarFill->release();
            }

            m_fields->bestBarShine = CCSprite::create("bestbar-shine.png"_spr);
            if (m_fields->bestBarShine) {
                m_fields->bestBarShine->setID("best-bar-shine");
                m_fields->bestBarShine->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
                m_fields->bestBarShine->setOpacity(0);
                m_fields->bestBarShine->setVisible(false);
                m_fields->bestBarClip->addChild(m_fields->bestBarShine, 1);
            }
        }

        // Tall GD-style current-position diamond, centered on the actual bar.
        m_fields->currentDiamondRoot = CCNode::create();
        m_fields->currentDiamondRoot->setID("best-bar-current-diamond-root");
        parent->addChild(m_fields->currentDiamondRoot, baseZ + 15);

        m_fields->currentDiamondGlow = CCSprite::create("bestbar-diamond-glow.png"_spr);
        if (m_fields->currentDiamondGlow) {
            m_fields->currentDiamondGlow->setID("best-bar-current-diamond-glow");
            m_fields->currentDiamondGlow->setOpacity(0);
            m_fields->currentDiamondGlow->setVisible(false);
            m_fields->currentDiamondGlow->runAction(CCRepeatForever::create(CCSequence::create(
                CCFadeTo::create(0.50f, 155),
                CCFadeTo::create(0.50f, 72),
                nullptr
            )));
            m_fields->currentDiamondRoot->addChild(m_fields->currentDiamondGlow, -1);
        }

        m_fields->currentDiamondNormal = CCSprite::create("bestbar-current-diamond.png"_spr);
        if (m_fields->currentDiamondNormal) {
            m_fields->currentDiamondNormal->setID("best-bar-current-diamond-normal");
            m_fields->currentDiamondRoot->addChild(m_fields->currentDiamondNormal, 1);
        }

        m_fields->currentDiamondGold = CCSprite::create("bestbar-current-diamond-gold.png"_spr);
        if (m_fields->currentDiamondGold) {
            m_fields->currentDiamondGold->setID("best-bar-current-diamond-gold");
            m_fields->currentDiamondGold->setVisible(false);
            m_fields->currentDiamondRoot->addChild(m_fields->currentDiamondGold, 2);
        }
        m_fields->currentDiamondRoot->setVisible(false);

        m_fields->bestMarkerRoot = CCNode::create();
        m_fields->bestMarkerRoot->setID("best-bar-marker-root");
        m_fields->bestMarkerRoot->setVisible(false);
        parent->addChild(m_fields->bestMarkerRoot, baseZ + 15);

        m_fields->markerSpark = CCSprite::create("bestbar-spark.png"_spr);
        if (m_fields->markerSpark) {
            m_fields->markerSpark->setID("best-bar-marker-spark");
            m_fields->markerSpark->setColor(color);
            m_fields->markerSpark->setOpacity(95);
            m_fields->markerSpark->setScale(0.62f);
            m_fields->markerSpark->setVisible(false);
            m_fields->markerSpark->runAction(CCRepeatForever::create(CCSequence::create(
                CCSpawn::create(CCFadeTo::create(0.42f, 185), CCScaleTo::create(0.42f, 0.80f), nullptr),
                CCSpawn::create(CCFadeTo::create(0.48f, 75), CCScaleTo::create(0.48f, 0.56f), nullptr),
                nullptr
            )));
            m_fields->bestMarkerRoot->addChild(m_fields->markerSpark, 0);
        }

        m_fields->bestMarker = CCSprite::create("bestbar-current-diamond.png"_spr);
        if (m_fields->bestMarker) {
            m_fields->bestMarker->setID("best-bar-marker");
            m_fields->bestMarker->setScale(0.46f);
            m_fields->bestMarkerRoot->addChild(m_fields->bestMarker, 1);
        }

        m_fields->completionCheck = CCSprite::create("bestbar-check.png"_spr);
        if (m_fields->completionCheck) {
            m_fields->completionCheck->setID("best-bar-marker-check");
            m_fields->completionCheck->setScale(0.42f);
            m_fields->completionCheck->setVisible(false);
            m_fields->bestMarkerRoot->addChild(m_fields->completionCheck, 2);
        }

        m_fields->sparkleRoot = CCNode::create();
        m_fields->sparkleRoot->setID("best-bar-sparkles");
        m_fields->sparkleRoot->setVisible(false);
        parent->addChild(m_fields->sparkleRoot, baseZ + 14);

        for (size_t i = 0; i < m_fields->ambientSparkles.size(); ++i) {
            auto sparkle = CCSprite::create("bestbar-spark.png"_spr);
            m_fields->ambientSparkles[i] = sparkle;
            if (!sparkle) continue;
            sparkle->setID(fmt::format("best-bar-spark-{}", i));
            sparkle->setColor(color);
            m_fields->sparkleRoot->addChild(sparkle, 2);
            startAmbientSparkle(
                sparkle,
                static_cast<float>(i) * 0.10f,
                0.12f + 0.018f * static_cast<float>(i % 3)
            );
        }

        m_fields->tipSpark = CCSprite::create("bestbar-spark.png"_spr);
        if (m_fields->tipSpark) {
            m_fields->tipSpark->setID("best-bar-tip-spark");
            m_fields->tipSpark->setColor(color);
            m_fields->tipSpark->setOpacity(0);
            m_fields->tipSpark->setScale(0.74f);
            m_fields->tipSpark->setVisible(true);
            m_fields->currentDiamondRoot->addChild(m_fields->tipSpark, 3);
        }

        m_fields->bestLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_fields->bestLabel->setID("best-bar-label");
        m_fields->bestLabel->setScale(0.47f);
        m_fields->bestLabel->setColor(color);
        parent->addChild(m_fields->bestLabel, baseZ + 16);

        m_fields->bestStatusRoot = CCNode::create();
        m_fields->bestStatusRoot->setID("best-bar-status-root");
        parent->addChild(m_fields->bestStatusRoot, baseZ + 17);

        m_fields->bestStatusSpark = CCSprite::create("bestbar-spark.png"_spr);
        if (m_fields->bestStatusSpark) {
            m_fields->bestStatusSpark->setID("best-bar-status-spark");
            m_fields->bestStatusSpark->setColor(color);
            m_fields->bestStatusSpark->setOpacity(80);
            m_fields->bestStatusSpark->setScale(1.10f);
            m_fields->bestStatusSpark->setVisible(false);
            m_fields->bestStatusSpark->runAction(CCRepeatForever::create(CCSequence::create(
                CCSpawn::create(
                    CCFadeTo::create(0.38f, 190),
                    CCScaleTo::create(0.38f, 1.28f),
                    CCRotateBy::create(0.38f, 50.0f),
                    nullptr
                ),
                CCSpawn::create(
                    CCFadeTo::create(0.42f, 70),
                    CCScaleTo::create(0.42f, 0.92f),
                    CCRotateBy::create(0.42f, 50.0f),
                    nullptr
                ),
                nullptr
            )));
            m_fields->bestStatusRoot->addChild(m_fields->bestStatusSpark, 0);
        }

        auto statusShadowX = CCSprite::create("bestbar-x.png"_spr);
        if (statusShadowX) {
            statusShadowX->setID("best-bar-status-x-shadow");
            statusShadowX->setColor({8, 6, 16});
            statusShadowX->setOpacity(220);
            statusShadowX->setPosition({0.9f, -1.1f});
            statusShadowX->setScale(1.08f);
            m_fields->bestStatusRoot->addChild(statusShadowX, 0);
        }

        m_fields->bestStatusX = CCSprite::create("bestbar-x.png"_spr);
        if (m_fields->bestStatusX) {
            m_fields->bestStatusX->setID("best-bar-status-x");
            m_fields->bestStatusX->setPosition({0.0f, 0.2f});
            m_fields->bestStatusX->setScale(1.0f);
            m_fields->bestStatusRoot->addChild(m_fields->bestStatusX, 1);
        }

        auto statusShadowCheck = CCSprite::create("bestbar-check.png"_spr);
        if (statusShadowCheck) {
            statusShadowCheck->setColor({12, 8, 20});
            statusShadowCheck->setOpacity(205);
            statusShadowCheck->setPosition({0.9f, -1.1f});
            statusShadowCheck->setScale(1.04f);
            statusShadowCheck->setVisible(false);
            statusShadowCheck->setID("best-bar-status-check-shadow");
            m_fields->bestStatusRoot->addChild(statusShadowCheck, 1);
        }

        m_fields->bestStatusCheck = CCSprite::create("bestbar-check.png"_spr);
        if (m_fields->bestStatusCheck) {
            m_fields->bestStatusCheck->setID("best-bar-status-check");
            m_fields->bestStatusCheck->setPosition({0.0f, 0.2f});
            m_fields->bestStatusCheck->setVisible(false);
            m_fields->bestStatusRoot->addChild(m_fields->bestStatusCheck, 2);
        }
        m_fields->bestStatusRoot->setVisible(false);

        auto winSize = CCDirector::get()->getWinSize();
        // Avoid full-screen translucent layers during gameplay. Both the old
        // flash and shine layers could create a one-frame GPU hitch right when
        // the player passed their best. Record effects now stay on the HUD bar.
        m_fields->flashLayer = nullptr;

        // Keep the dramatic screen-crossing shine, but render it as one narrow
        // pre-baked sprite instead of a rotated full-screen translucent layer.
        // This preserves the visible sweep without the large overdraw spike.
        m_fields->shineBand = nullptr;
        m_fields->screenShine = CCSprite::create("bestbar-screen-shine.png"_spr);
        if (m_fields->screenShine) {
            m_fields->screenShine->setID("best-bar-screen-shine");
            m_fields->screenShine->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
            m_fields->screenShine->setOpacity(0);
            m_fields->screenShine->setVisible(false);
            auto shineSize = m_fields->screenShine->getContentSize();
            m_fields->screenShine->setScaleX(1.18f);
            if (shineSize.height > 1.0f) {
                m_fields->screenShine->setScaleY((winSize.height * 1.22f) / shineSize.height);
            }
            m_fields->screenShine->setPosition({-shineSize.width * 1.18f, winSize.height * 0.5f});
            addChild(m_fields->screenShine, 10'001);
        }

        m_fields->burstRoot = CCNode::create();
        m_fields->burstRoot->setID("best-bar-burst");
        parent->addChild(m_fields->burstRoot, baseZ + 18);
        for (size_t i = 0; i < m_fields->burstSparkles.size(); ++i) {
            auto sparkle = CCSprite::create("bestbar-spark.png"_spr);
            m_fields->burstSparkles[i] = sparkle;
            if (!sparkle) continue;
            sparkle->setColor(color);
            sparkle->setOpacity(0);
            sparkle->setScale(0.08f);
            m_fields->burstRoot->addChild(sparkle);
        }

        m_fields->goldRunTimerLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_fields->goldRunTimerLabel->setID("best-bar-gold-run-timer");
        m_fields->goldRunTimerLabel->setScale(0.42f);
        m_fields->goldRunTimerLabel->setAnchorPoint({1.0f, 0.5f});
        addChild(m_fields->goldRunTimerLabel, 250);

        m_fields->goldRunBestLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_fields->goldRunBestLabel->setID("best-bar-gold-run-best");
        m_fields->goldRunBestLabel->setScale(0.30f);
        m_fields->goldRunBestLabel->setAnchorPoint({1.0f, 0.5f});
        addChild(m_fields->goldRunBestLabel, 250);

        m_fields->goldRunStatsLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_fields->goldRunStatsLabel->setID("best-bar-gold-run-stats");
        m_fields->goldRunStatsLabel->setScale(0.21f);
        m_fields->goldRunStatsLabel->setAnchorPoint({1.0f, 0.5f});
        addChild(m_fields->goldRunStatsLabel, 250);

        m_fields->bannerLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_fields->bannerLabel->setID("best-bar-banner");
        m_fields->bannerLabel->setScale(0.54f);
        m_fields->bannerLabel->setOpacity(0);
        m_fields->bannerLabel->setPosition({winSize.width / 2.0f, winSize.height - 58.0f});
        addChild(m_fields->bannerLabel, 300);

        updateBestBarLayout();
        m_fields->uiReady = true;
    }

    void updateColors() {
        if (!m_fields->uiReady) return;
        auto color = bestColor();
        if (m_fields->currentGoldFill) m_fields->currentGoldFill->setColor(color);
        if (m_fields->currentGoldGlow) m_fields->currentGoldGlow->setColor(color);
        if (m_fields->markerSpark) m_fields->markerSpark->setColor(color);
        if (m_fields->bestBarGlow) m_fields->bestBarGlow->setColor(color);
        if (m_fields->bestBarFill) m_fields->bestBarFill->setColor(ccWHITE);
        if (m_fields->bestBarTrail) m_fields->bestBarTrail->setColor(color);
        if (m_fields->bestLabel) m_fields->bestLabel->setColor(color);
        if (m_fields->flashLayer) m_fields->flashLayer->setColor(color);
        if (m_fields->tipSpark) m_fields->tipSpark->setColor(color);
        if (m_fields->bestStatusSpark) m_fields->bestStatusSpark->setColor(color);
        if (m_fields->goldRunBestLabel) m_fields->goldRunBestLabel->setColor(color);
        if (m_fields->bannerLabel) m_fields->bannerLabel->setColor(color);
        for (auto sparkle : m_fields->ambientSparkles) if (sparkle) sparkle->setColor(color);
        for (auto sparkle : m_fields->burstSparkles) if (sparkle) sparkle->setColor(color);
    }

    int loadBestPercent() {
        auto saved = Mod::get()->getSavedValue<int>(percentKey(m_level), 0);
        auto gameBest = m_level ? m_level->getNormalPercent() : 0;
        auto best = std::clamp(std::max(saved, gameBest), 0, 100);
        if (best != saved) Mod::get()->setSavedValue<int>(percentKey(m_level), best);
        return best;
    }

    int loadNormalBestTime() {
        auto saved = Mod::get()->getSavedValue<int>(normalTimeKey(m_level), 0);
        auto gameBest = m_level ? m_level->m_bestTime : 0;
        int best = saved;
        if (best <= 0 || (gameBest > 0 && gameBest < best)) best = gameBest;
        if (best > 0 && best != saved) Mod::get()->setSavedValue<int>(normalTimeKey(m_level), best);
        return std::max(best, 0);
    }

    void loadGoldRunStats() {
        auto mod = Mod::get();
        m_fields->goldRunBestTimeMs = mod->getSavedValue<int>(speedrunBestKey(m_level), 0);
        m_fields->goldRunLastTimeMs = mod->getSavedValue<int>(speedrunLastKey(m_level), 0);
        m_fields->goldRunRuns = mod->getSavedValue<int>(speedrunRunsKey(m_level), 0);
        m_fields->goldRunFailures = mod->getSavedValue<int>(speedrunFailuresKey(m_level), 0);
        m_fields->goldRunFinishes = mod->getSavedValue<int>(speedrunFinishesKey(m_level), 0);
    }

    void syncAttemptState() {
        m_fields->attemptStartBestPercent = loadBestPercent();
        m_fields->shownBestPercent = m_fields->attemptStartBestPercent;
        m_fields->currentPercent = 0;
        m_fields->currentProgressRatio = 0.0f;
        m_fields->lastAnimatedPercent = -1;
        m_fields->lastMarkerPercent = -1;
        m_fields->completionCheckShown = false;
        m_fields->currentGoldRatio = 0.0f;
        m_fields->savedBestTimeMs = loadNormalBestTime();
        loadGoldRunStats();

        m_fields->currentTimeMs = 0;
        m_fields->flashedThisAttempt = false;
        m_fields->settingsKeyDownLast = false;
        m_fields->completionHandled = false;
        m_fields->bestChaseActive = false;
        m_fields->goldVisualActive = false;
        m_fields->pendingGoldShine = false;
        m_fields->diamondWasGold = false;
        m_fields->bestStatusCompletedShown = false;
        m_fields->goldRunCompleted = false;
        m_fields->goldRunStarted = m_fields->goldRunMode && supportsGoldRun();
        m_fields->nextIntegrityScanSeconds = 0.0f;

        m_fields->runSafeMode = settingEnabled("leaderboard-safe-mode", true);
        if (!m_fields->goldRunMode) {
            m_fields->runCheated = false;
            m_fields->runIntegrityReason.clear();
        }
        else {
            auto integrity = inspectLoadedMods();
            m_fields->runCheated = integrity.majorCheatDetected;
            m_fields->runIntegrityReason = m_fields->runCheated
                ? integrity.reason
                : (m_fields->runSafeMode ? "SAFE MODE" : "VALID");
        }
        restorePercentLabel();
    }

    void showBanner(std::string const& text) {
        if (!m_fields->bannerLabel || text.empty()) return;
        m_fields->bannerLabel->stopAllActions();
        m_fields->bannerLabel->setString(text.c_str());
        m_fields->bannerLabel->setOpacity(255);
        m_fields->bannerLabel->setScale(0.54f);
        m_fields->bannerLabel->runAction(CCSequence::create(
            CCScaleTo::create(0.10f, 0.70f),
            CCScaleTo::create(0.16f, 0.54f),
            CCDelayTime::create(0.76f),
            CCFadeOut::create(0.34f),
            nullptr
        ));
    }

    void showPendingBanner() {
        if (m_fields->pendingBanner.empty()) return;
        auto text = m_fields->pendingBanner;
        m_fields->pendingBanner.clear();
        showBanner(text);
    }

    void runBurstAnimation() {
        if (!m_fields->burstRoot) return;
        const std::array<CCPoint, 8> offsets = {
            CCPoint {-72.0f, 16.0f}, CCPoint {-48.0f, -21.0f},
            CCPoint {-18.0f, 29.0f}, CCPoint {16.0f, -28.0f},
            CCPoint {44.0f, 24.0f}, CCPoint {72.0f, -12.0f},
            CCPoint {-88.0f, -6.0f}, CCPoint {88.0f, 10.0f},
        };

        for (size_t i = 0; i < m_fields->burstSparkles.size(); ++i) {
            auto sparkle = m_fields->burstSparkles[i];
            if (!sparkle) continue;
            sparkle->stopAllActions();
            if (i >= 2) {
                sparkle->setOpacity(0);
                continue;
            }
            sparkle->setPosition({0.0f, 0.0f});
            sparkle->setOpacity(180);
            sparkle->setScale(0.07f);
            sparkle->runAction(CCSpawn::create(
                CCMoveTo::create(0.24f, offsets[i * 2]),
                CCSequence::create(
                    CCScaleTo::create(0.07f, 0.17f),
                    CCSpawn::create(CCScaleTo::create(0.17f, 0.05f), CCFadeOut::create(0.17f), nullptr),
                    nullptr
                ),
                nullptr
            ));
        }
    }

    void hideScreenShine() {
        if (!m_fields->screenShine) return;
        m_fields->screenShine->stopAllActions();
        m_fields->screenShine->setOpacity(0);
        m_fields->screenShine->setVisible(false);
    }

    void stopGoldShineAnimation() {
        hideScreenShine();
        m_fields->pendingGoldShine = false;

        if (m_fields->bestBarShine) {
            m_fields->bestBarShine->stopAllActions();
            m_fields->bestBarShine->setOpacity(0);
            m_fields->bestBarShine->setVisible(false);
        }
        if (m_fields->tipSpark) {
            m_fields->tipSpark->stopAllActions();
            m_fields->tipSpark->setOpacity(0);
        }
        for (auto sparkle : m_fields->burstSparkles) {
            if (!sparkle) continue;
            sparkle->stopAllActions();
            sparkle->setOpacity(0);
        }
    }

    void playGoldShineAnimation() {
        if (!m_fields->uiReady) return;
        m_fields->pendingGoldShine = false;

        if (m_fields->screenShine) {
            auto winSize = CCDirector::get()->getWinSize();
            auto shineSize = m_fields->screenShine->getContentSize();
            m_fields->screenShine->setScaleX(1.18f);
            float scaledWidth = shineSize.width * std::abs(m_fields->screenShine->getScaleX());
            m_fields->screenShine->stopAllActions();
            m_fields->screenShine->setVisible(true);
            m_fields->screenShine->setOpacity(0);
            m_fields->screenShine->setPosition({-scaledWidth - 24.0f, winSize.height * 0.5f});
            m_fields->screenShine->runAction(CCSequence::create(
                CCSpawn::create(
                    CCEaseSineInOut::create(CCMoveTo::create(0.29f, {winSize.width + scaledWidth + 24.0f, winSize.height * 0.5f})),
                    CCSequence::create(
                        CCFadeTo::create(0.035f, 255),
                        CCDelayTime::create(0.145f),
                        CCFadeOut::create(0.095f),
                        nullptr
                    ),
                    nullptr
                ),
                CCCallFunc::create(this, callfunc_selector(BestBarPlayLayer::hideScreenShine)),
                nullptr
            ));
        }

        if (m_fields->bestBarGlow) {
            m_fields->bestBarGlow->stopAllActions();
            m_fields->bestBarGlow->setOpacity(120);
            m_fields->bestBarGlow->runAction(CCSequence::create(
                CCFadeTo::create(0.05f, 235),
                CCFadeTo::create(0.17f, 92),
                nullptr
            ));
        }

        if (m_fields->bestBarShine && m_fields->bestBarRoot && m_fields->bestBarRoot->isVisible()) {
            float ratio = std::clamp(m_fields->currentGoldRatio > 0.0f
                ? m_fields->currentGoldRatio
                : m_fields->currentProgressRatio, 0.0f, 1.0f);
            float width = std::max(18.0f, m_fields->goldMaskSourceWidth * ratio);
            float shineWidth = m_fields->bestBarShine->getContentSize().width;
            float left = -m_fields->goldMaskSourceWidth * 0.5f;
            m_fields->bestBarShine->stopAllActions();
            m_fields->bestBarShine->setVisible(true);
            m_fields->bestBarShine->setOpacity(0);
            m_fields->bestBarShine->setScaleX(0.92f);
            m_fields->bestBarShine->setScaleY(m_fields->goldFillBaseScaleY * 1.04f);
            m_fields->bestBarShine->setPosition({left - shineWidth * 0.35f, 0.0f});
            m_fields->bestBarShine->runAction(CCSpawn::create(
                CCMoveTo::create(0.22f, {left + width + shineWidth * 0.35f, 0.0f}),
                CCSequence::create(
                    CCFadeTo::create(0.04f, 240),
                    CCDelayTime::create(0.09f),
                    CCFadeOut::create(0.10f),
                    nullptr
                ),
                nullptr
            ));
        }

        if (m_fields->tipSpark) {
            m_fields->tipSpark->stopAllActions();
            m_fields->tipSpark->setOpacity(0);
            m_fields->tipSpark->runAction(CCSequence::create(
                CCFadeTo::create(0.03f, 255),
                CCFadeTo::create(0.10f, 94),
                CCFadeOut::create(0.10f),
                nullptr
            ));
        }

        runBurstAnimation();
    }

    void triggerBestFlash(std::string const& banner = "") {
        if (!m_fields->uiReady) return;

        playBestSound();

        if (settingEnabled("gold-flash")) {
            // Lightweight flash: pulse the diamond and marker instead of
            // drawing a translucent rectangle over the entire playfield.
            if (m_fields->currentDiamondGlow && m_fields->currentDiamondRoot &&
                m_fields->currentDiamondRoot->isVisible()) {
                m_fields->currentDiamondGlow->stopAllActions();
                m_fields->currentDiamondGlow->setVisible(true);
                m_fields->currentDiamondGlow->setOpacity(40);
                m_fields->currentDiamondGlow->runAction(CCSequence::create(
                    CCFadeTo::create(0.04f, 180),
                    CCFadeTo::create(0.13f, 70),
                    nullptr
                ));
            }
            if (m_fields->markerSpark && m_fields->bestMarkerRoot &&
                m_fields->bestMarkerRoot->isVisible()) {
                m_fields->markerSpark->stopAllActions();
                m_fields->markerSpark->setOpacity(55);
                m_fields->markerSpark->runAction(CCSequence::create(
                    CCFadeTo::create(0.04f, 165),
                    CCFadeTo::create(0.12f, 60),
                    nullptr
                ));
            }
        }

        if (settingEnabled("gold-shine")) {
            m_fields->pendingGoldShine = true;
            playGoldShineAnimation();
        }

        if (settingEnabled("pulse-label") && m_fields->bestLabel) {
            m_fields->bestLabel->stopAllActions();
            m_fields->bestLabel->setScale(0.48f);
            m_fields->bestLabel->runAction(CCSequence::create(
                CCScaleTo::create(0.08f, 0.66f),
                CCScaleTo::create(0.18f, 0.48f),
                nullptr
            ));
        }

        if (!banner.empty()) showBanner(banner);
    }

    void setGoldVisualActive(bool active, float ratio = 1.0f) {
        if (!m_fields->bestBarRoot) return;
        auto fill = m_progressFill ? m_progressFill : m_progressBar;
        ratio = std::clamp(ratio, 0.0f, 1.0f);

        if (!active || ratio <= 0.0f) {
            if (fill && m_fields->progressFillOpacityCaptured) {
                fill->setOpacity(m_fields->originalProgressFillOpacity);
            }
            m_fields->bestBarRoot->stopAllActions();
            m_fields->bestBarRoot->setVisible(false);
            m_fields->bestBarRoot->setOpacity(255);
            if (m_fields->bestBarShine) {
                m_fields->bestBarShine->stopAllActions();
                m_fields->bestBarShine->setVisible(false);
                m_fields->bestBarShine->setOpacity(0);
            }
            if (m_fields->tipSpark) {
                m_fields->tipSpark->stopAllActions();
                m_fields->tipSpark->setOpacity(0);
            }
            m_fields->goldVisualActive = false;
            m_fields->pendingGoldShine = false;
            m_fields->currentGoldRatio = 0.0f;
            return;
        }

        // Use actual runtime content units rather than raw PNG pixels. This
        // prevents the crop from reaching its maximum around 50% on scaled
        // textures and lets the gold continue all the way through 100%.
        ratio = std::clamp(m_fields->currentProgressRatio > 0.0f
            ? m_fields->currentProgressRatio
            : ratio, 0.0f, 1.0f);
        float fillSourceWidth = std::clamp(
            m_fields->goldFillSourceWidth * ratio,
            1.0f,
            m_fields->goldFillSourceWidth
        );
        float glowSourceWidth = std::clamp(
            m_fields->goldGlowSourceWidth * ratio,
            1.0f,
            m_fields->goldGlowSourceWidth
        );
        float maskSourceWidth = std::clamp(
            m_fields->goldMaskSourceWidth * ratio,
            1.0f,
            m_fields->goldMaskSourceWidth
        );

        if (fill) fill->setOpacity(0);
        m_fields->bestBarRoot->setVisible(true);

        if (m_fields->bestBarFill) {
            m_fields->bestBarFill->setScaleX(m_fields->goldFillBaseScaleX);
            m_fields->bestBarFill->setScaleY(m_fields->goldFillBaseScaleY);
            m_fields->bestBarFill->setTextureRect(
                {0.0f, 0.0f, fillSourceWidth, m_fields->goldFillSourceHeight},
                false,
                {fillSourceWidth, m_fields->goldFillSourceHeight}
            );
            m_fields->bestBarFill->setOpacity(255);
        }
        if (m_fields->bestBarGlow) {
            m_fields->bestBarGlow->setScaleX(m_fields->goldGlowBaseScaleX);
            m_fields->bestBarGlow->setScaleY(m_fields->goldGlowBaseScaleY * 1.02f);
            m_fields->bestBarGlow->setTextureRect(
                {0.0f, 0.0f, glowSourceWidth, m_fields->goldGlowSourceHeight},
                false,
                {glowSourceWidth, m_fields->goldGlowSourceHeight}
            );
            m_fields->bestBarGlow->setOpacity(
                static_cast<GLubyte>(std::clamp(115.0f + ratio * 90.0f, 115.0f, 205.0f))
            );
        }
        if (m_fields->bestBarMask) {
            m_fields->bestBarMask->setScaleX(1.0f);
            m_fields->bestBarMask->setScaleY(1.0f);
            m_fields->bestBarMask->setTextureRect(
                {0.0f, 0.0f, maskSourceWidth, m_fields->goldMaskSourceHeight},
                false,
                {maskSourceWidth, m_fields->goldMaskSourceHeight}
            );
        }

        if (!m_fields->goldVisualActive) {
            m_fields->bestBarRoot->stopAllActions();
            m_fields->bestBarRoot->setOpacity(255);
            if (settingEnabled("animated-sparkles")) runBurstAnimation();
        }

        m_fields->goldVisualActive = true;
        m_fields->currentGoldRatio = ratio;

        if (m_fields->pendingGoldShine && settingEnabled("gold-shine")) {
            playGoldShineAnimation();
        }
    }

    // Retained only for source compatibility with older saves/build scripts.
    // The v1.5.0 replacement targets the stock fill and never creates a second bar.
    void setBestBarPercent(int percent, bool) {
        m_fields->lastAnimatedPercent = std::clamp(percent, 0, 100);
    }

    void setCurrentGoldPercent(int percent, bool show) {
        if (!m_fields->currentGoldRoot) return;

        percent = std::clamp(percent, 0, 100);
        float ratio = static_cast<float>(percent) / 100.0f;
        bool visible = show && percent > 0;

        m_fields->currentGoldRoot->setVisible(visible);
        if (m_fields->currentGoldGlow) {
            m_fields->currentGoldGlow->setVisible(visible);
        }

        if (!visible) {
            if (m_fields->currentGoldFill) {
                m_fields->currentGoldFill->stopAllActions();
                m_fields->currentGoldFill->setScaleX(0.0f);
            }
            if (m_fields->currentGoldGlow) m_fields->currentGoldGlow->setOpacity(0);
            m_fields->currentGoldRatio = 0.0f;
            return;
        }

        if (m_fields->currentGoldFill) {
            auto previous = std::clamp(m_fields->currentGoldRatio, 0.0f, 1.0f);
            auto duration = std::clamp(0.08f + std::abs(ratio - previous) * 0.42f, 0.08f, 0.24f);
            m_fields->currentGoldFill->stopAllActions();

            if (ratio >= previous) {
                m_fields->currentGoldFill->runAction(CCScaleTo::create(duration, ratio, 1.0f));
            }
            else {
                m_fields->currentGoldFill->setScaleX(ratio);
            }

            // The gold becomes brighter as the run moves toward 100%.
            m_fields->currentGoldFill->setOpacity(
                static_cast<GLubyte>(std::clamp(145.0f + ratio * 110.0f, 145.0f, 255.0f))
            );
        }

        if (m_fields->currentGoldShine) {
            m_fields->currentGoldShine->setVisible(ratio > 0.02f);
            m_fields->currentGoldShine->setOpacity(
                static_cast<GLubyte>(std::clamp(105.0f + ratio * 150.0f, 105.0f, 255.0f))
            );
        }

        if (m_fields->currentGoldGlow) {
            m_fields->currentGoldGlow->setOpacity(
                static_cast<GLubyte>(std::clamp(24.0f + ratio * 95.0f, 24.0f, 119.0f))
            );
        }

        m_fields->currentGoldRatio = ratio;
    }

    void updateCurrentDiamond(int percent, bool show, bool gold) {
        if (!m_fields->currentDiamondRoot) return;

        m_fields->currentPercent = std::clamp(percent, 0, 100);
        m_fields->currentDiamondRoot->setVisible(show);
        if (!show) {
            m_fields->diamondWasGold = false;
            return;
        }

        if (m_fields->currentDiamondNormal) m_fields->currentDiamondNormal->setVisible(!gold);
        if (m_fields->currentDiamondGold) m_fields->currentDiamondGold->setVisible(gold);
        if (m_fields->currentDiamondGlow) {
            m_fields->currentDiamondGlow->setVisible(gold && settingEnabled("animated-sparkles"));
        }

        if (gold && !m_fields->diamondWasGold && m_fields->currentDiamondGold) {
            m_fields->currentDiamondGold->stopAllActions();
            m_fields->currentDiamondGold->setOpacity(0);
            m_fields->currentDiamondGold->setScale(0.58f);
            m_fields->currentDiamondGold->runAction(CCSpawn::create(
                CCFadeIn::create(0.10f),
                CCEaseBackOut::create(CCScaleTo::create(0.24f, 1.0f)),
                nullptr
            ));
        }
        else if (!gold && m_fields->currentDiamondGold) {
            m_fields->currentDiamondGold->stopAllActions();
            m_fields->currentDiamondGold->setOpacity(255);
            m_fields->currentDiamondGold->setScale(1.0f);
        }

        m_fields->diamondWasGold = gold;
    }

    void updateGoldSparkles(int, bool show) {
        if (!m_fields->sparkleRoot) return;
        bool visible = show && settingEnabled("animated-sparkles");
        m_fields->sparkleRoot->setVisible(visible);
    }

    void updateBestMarker(int percent, bool show) {
        if (!m_fields->bestMarkerRoot || !m_progressBar) return;

        percent = std::clamp(percent, 0, 100);
        bool visible = show && percent > 0;
        m_fields->bestMarkerRoot->setVisible(visible);
        if (!visible) return;

        auto progressBox = m_progressBar->boundingBox();
        float centerX = progressBox.getMidX();
        float x = centerX - m_fields->bestBarWidth * 0.5f +
            m_fields->bestBarWidth * (static_cast<float>(percent) / 100.0f);
        m_fields->bestMarkerRoot->setPosition({x, m_fields->bestBarY});

        bool completed = percent >= 100;
        if (m_fields->bestMarker) m_fields->bestMarker->setVisible(!completed);
        if (m_fields->completionCheck) m_fields->completionCheck->setVisible(completed);
        if (m_fields->markerSpark) m_fields->markerSpark->setVisible(settingEnabled("animated-sparkles"));

        if (completed && !m_fields->completionCheckShown && m_fields->completionCheck) {
            m_fields->completionCheckShown = true;
            m_fields->completionCheck->stopActionByTag(808);
            m_fields->completionCheck->setScale(0.58f);
            auto bounce = CCSequence::create(
                CCScaleTo::create(0.10f, 0.74f),
                CCScaleTo::create(0.16f, 0.58f),
                nullptr
            );
            bounce->setTag(808);
            m_fields->completionCheck->runAction(bounce);
        }
        else if (!completed) {
            m_fields->completionCheckShown = false;
            if (m_fields->completionCheck) m_fields->completionCheck->setScale(0.42f);
        }
    }

    bool isKeyDownWin(int vk) {
#ifdef GEODE_IS_WINDOWS
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
#else
        return false;
#endif
    }

    void handleSettingsUIHotkey() {
        int vk = getConfiguredSettingsUIKey();
        if (!vk) {
            m_fields->settingsKeyDownLast = false;
            return;
        }

        bool down = isKeyDownWin(vk);
        if (down && !m_fields->settingsKeyDownLast) {
            openQuickSettingsUI();
        }
        m_fields->settingsKeyDownLast = down;
    }

    void markGoldRunInvalid(std::string reason) {
        if (!m_fields->goldRunMode || m_fields->runCheated) return;
        m_fields->runCheated = true;
        m_fields->runIntegrityReason = std::move(reason);
        showBanner(fmt::format("INVALID: {}", shorten(m_fields->runIntegrityReason, 34)));
    }

    void scanGoldRunIntegrity() {
        if (!m_fields->goldRunMode || !m_fields->goldRunStarted) return;

        auto scheduler = CCDirector::get()->getScheduler();
        float timeScale = scheduler ? scheduler->getTimeScale() : 1.0f;
        if (std::abs(timeScale - 1.0f) > 0.015f) {
            markGoldRunInvalid(fmt::format("Modified game speed ({:.3f}x)", timeScale));
        }

        float elapsedSeconds = static_cast<float>(m_fields->currentTimeMs) / 1000.0f;
        if (elapsedSeconds < m_fields->nextIntegrityScanSeconds) return;
        m_fields->nextIntegrityScanSeconds = elapsedSeconds + 0.20f;

        auto integrity = inspectLoadedMods();
        if (integrity.majorCheatDetected) markGoldRunInvalid(integrity.reason);
    }

    void submitOnlineRun(int timeMs, bool valid, std::string const& status) {
        auto api = leaderboardApiBase();
        if (api.empty() || !m_level) return;
        if (!valid && !settingEnabled("submit-cheated-runs", true)) return;

        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        auto nonceMaterial = fmt::format(
            "{}|{}|{}|{}", installID(), stableLevelSuffix(m_level), timeMs, nowMs
        );
        auto nonce = fmt::format("{:016x}-{}", std::hash<std::string>{}(nonceMaterial), nowMs);

        auto payload = matjson::makeObject({
            {"levelId", stableLevelSuffix(m_level)},
            {"levelName", std::string(m_level->m_levelName.c_str())},
            {"difficulty", difficultyName(m_level)},
            {"rewards", std::max(0, m_level->m_stars.value())},
            {"platformer", m_level->isPlatformer()},
            {"player", leaderboardPlayerName()},
            {"playerId", installID()},
            {"timeMs", timeMs},
            {"valid", valid},
            {"status", status},
            {"reason", m_fields->runIntegrityReason},
            {"safeMode", m_fields->runSafeMode},
            {"modVersion", "1.7.8"},
            {"runNonce", nonce}
        });

        auto request = web::WebRequest();
        request.header("Content-Type", "application/json");
        request.bodyJSON(payload);
        m_fields->leaderboardSubmitTask.spawn(
            "Best Bar Leaderboard Submit",
            request.post(api + "/api/v1/runs"),
            [](web::WebResponse response) {
                if (!response.ok()) {
                    log::warn("Best Bar leaderboard submission failed with HTTP {}", response.code());
                }
            }
        );
    }

    void refreshGoldRunHUD() {
        auto show = shouldShow() && supportsGoldRun() && m_fields->goldRunMode &&
            settingEnabled("gold-run-mode");

        if (m_fields->goldRunTimerLabel) m_fields->goldRunTimerLabel->setVisible(show);
        if (m_fields->goldRunBestLabel) m_fields->goldRunBestLabel->setVisible(show);
        if (m_fields->goldRunStatsLabel) m_fields->goldRunStatsLabel->setVisible(show);
        if (!show) return;

        auto winSize = CCDirector::get()->getWinSize();
        auto x = winSize.width - 16.0f;
        auto y = winSize.height - 44.0f;
        auto color = bestColor();
        bool ahead = m_fields->goldRunBestTimeMs <= 0 ||
            (m_fields->currentTimeMs > 0 && m_fields->currentTimeMs < m_fields->goldRunBestTimeMs);

        m_fields->goldRunTimerLabel->setPosition({x, y});
        m_fields->goldRunTimerLabel->setString(
            fmt::format("TIME {}", formatTime(m_fields->currentTimeMs)).c_str()
        );
        m_fields->goldRunTimerLabel->setColor(ahead ? color : ccWHITE);

        m_fields->goldRunBestLabel->setPosition({x, y - 17.0f});
        m_fields->goldRunBestLabel->setString(
            fmt::format("GOLD BEST {}", formatTime(m_fields->goldRunBestTimeMs)).c_str()
        );

        m_fields->goldRunStatsLabel->setPosition({x, y - 31.0f});
        if (m_fields->runCheated) {
            auto reason = m_fields->runIntegrityReason.empty()
                ? std::string("Unknown integrity failure")
                : m_fields->runIntegrityReason;
            m_fields->goldRunStatsLabel->setString(
                fmt::format("INVALID: {}", shorten(reason, 62)).c_str()
            );
            m_fields->goldRunStatsLabel->setColor({255, 95, 95});
        }
        else {
            m_fields->goldRunStatsLabel->setString(
                fmt::format("VALID  |  RUNS {}  FINISHES {}  FAILS {}", m_fields->goldRunRuns,
                    m_fields->goldRunFinishes, m_fields->goldRunFailures).c_str()
            );
            m_fields->goldRunStatsLabel->setColor({235, 235, 235});
        }
    }

    void refreshBestBarUI() {
        createBestBarUI();
        if (!m_fields->uiReady) return;
        updateColors();
        updateBestBarLayout();

        bool visible = shouldShow();
        bool platformer = isPlatformerLevel();
        bool progressVisible = m_progressBar && m_progressBar->isVisible();
        bool barSetting = settingEnabled("show-best-bar");

        // These old nodes are force-disabled so no thin second bar can reappear.
        setCurrentGoldPercent(0, false);
        updateBestStatus(false, false);

        if (!visible) {
            setGoldVisualActive(false, 0.0f);
            if (m_fields->sparkleRoot) m_fields->sparkleRoot->setVisible(false);
            if (m_fields->bestLabel) m_fields->bestLabel->setVisible(false);
            updateCurrentDiamond(0, false, false);
            restorePercentLabel();
            updateBestMarker(0, false);
            refreshGoldRunHUD();
            return;
        }

        if (platformer) {
            bool goldRunActive = m_fields->goldRunMode && settingEnabled("gold-run-mode") &&
                barSetting && progressVisible;

            setGoldVisualActive(goldRunActive, m_fields->currentProgressRatio);
            updateCurrentDiamond(
                m_fields->currentPercent,
                progressVisible && barSetting,
                goldRunActive
            );
            updateGoldSparkles(100, goldRunActive);
            colorCurrentLabel(goldRunActive);

            bool showNormalBest = !m_fields->goldRunMode && settingEnabled("platformer-mode") &&
                settingEnabled("show-best-label") && m_percentageLabel && m_percentageLabel->isVisible();
            if (m_fields->bestLabel) {
                m_fields->bestLabel->setVisible(showNormalBest);
                m_fields->bestLabel->setString(
                    fmt::format("BEST {}", formatTime(m_fields->savedBestTimeMs)).c_str()
                );
                auto winSize = CCDirector::get()->getWinSize();
                m_fields->bestLabel->setPosition({winSize.width / 2.0f, winSize.height - 24.0f});
            }
            updateBestMarker(0, false);
            updateBestStatus(false, false);
            refreshGoldRunHUD();
            return;
        }

        refreshGoldRunHUD();

        bool passedOriginalBest = m_fields->currentPercent > m_fields->attemptStartBestPercent;
        bool completedThisAttempt = m_fields->currentPercent >= 100;
        bool levelCompleted = m_fields->attemptStartBestPercent >= 100 ||
            m_fields->shownBestPercent >= 100 ||
            (m_level && m_level->getNormalPercent() >= 100);
        bool goldActive = barSetting && progressVisible &&
            (levelCompleted || passedOriginalBest || completedThisAttempt);
        float goldRatio = m_fields->currentProgressRatio;

        // Previously completed levels start gold immediately, but the fill still
        // tracks the real live progress amount instead of showing a full bar.
        setGoldVisualActive(goldActive, goldRatio);
        updateCurrentDiamond(m_fields->currentPercent, barSetting && progressVisible, goldActive);
        updateGoldSparkles(m_fields->currentPercent, goldActive);
        colorCurrentLabel(goldActive);

        bool showLabel = settingEnabled("show-best-label") && progressVisible;
        if (m_fields->bestLabel) {
            m_fields->bestLabel->setVisible(showLabel);
            m_fields->bestLabel->setString(fmt::format("BEST {}%", m_fields->shownBestPercent).c_str());
            // updateBestBarLayout already places this in the correct parent space.
        }
        updateBestMarker(m_fields->shownBestPercent, showLabel);
        updateBestStatus(showLabel, levelCompleted);
    }

    void recordGoldRun(bool completed) {
        if (!m_level || !m_fields->goldRunMode || !canSaveRecord()) return;
        auto timeMs = std::max(m_fields->currentTimeMs, 0);
        auto mod = Mod::get();
        bool valid = !m_fields->runCheated;
        std::string status = valid ? "VALID" : "CHEATED / NOT VALID";

        ++m_fields->goldRunRuns;
        m_fields->goldRunLastTimeMs = timeMs;
        mod->setSavedValue<int>(speedrunRunsKey(m_level), m_fields->goldRunRuns);
        mod->setSavedValue<int>(speedrunLastKey(m_level), timeMs);

        if (completed) {
            ++m_fields->goldRunFinishes;
            mod->setSavedValue<int>(speedrunFinishesKey(m_level), m_fields->goldRunFinishes);

            bool newBest = valid && timeMs > 0 &&
                (m_fields->goldRunBestTimeMs <= 0 || timeMs < m_fields->goldRunBestTimeMs);
            if (newBest) {
                m_fields->goldRunBestTimeMs = timeMs;
                mod->setSavedValue<int>(speedrunBestKey(m_level), timeMs);
                triggerBestFlash("NEW GOLD BEST!");
            }
            else if (!valid) {
                showBanner("CHEATED / NOT VALID");
            }
            else {
                showBanner("GOLD RUN COMPLETE");
            }

            appendLocalRun(m_level, timeMs, valid, status);
            submitOnlineRun(timeMs, valid, status);
        }
        else {
            ++m_fields->goldRunFailures;
            mod->setSavedValue<int>(speedrunFailuresKey(m_level), m_fields->goldRunFailures);
            m_fields->pendingBanner = valid
                ? fmt::format("RUN SAVED  {}", formatTime(timeMs))
                : "CHEATED RUN SAVED - NOT VALID";
        }

        appendSpeedrunHistory(m_level, completed, timeMs);
        m_fields->goldRunStarted = false;
        m_fields->goldRunCompleted = completed;
    }

    void finalizeGoldRunFailureIfNeeded() {
        if (!m_fields->setupFinished || !m_fields->goldRunMode ||
            !m_fields->goldRunStarted || m_fields->goldRunCompleted) return;

        if (m_fields->currentTimeMs <= 0) {
            m_fields->currentTimeMs = 1;
        }
        recordGoldRun(false);
    }

public:
    void update(float dt) {
        PlayLayer::update(dt);
        createBestBarUI();

        if (!m_level || !shouldShow()) return;
        syncLiveProgress();

        if (m_level->isPlatformer()) {
            bool passedOldBest = m_fields->currentPercent > m_fields->attemptStartBestPercent;
            if (passedOldBest && !m_fields->flashedThisAttempt && m_fields->attemptStartBestPercent < 100) {
                m_fields->flashedThisAttempt = true;
                m_fields->bestChaseActive = true;
                triggerBestFlash("NEW BEST IN PROGRESS");
            }
            if (canSaveRecord() && m_fields->currentPercent > m_fields->shownBestPercent) {
                m_fields->shownBestPercent = m_fields->currentPercent;
                Mod::get()->setSavedValue<int>(percentKey(m_level), m_fields->currentPercent);
            }
        }

        handleSettingsUIHotkey();

        if (m_fields->goldRunMode && !m_level->isPlatformer() && !m_fields->completionHandled) {
            dt = std::max(dt, 0.0f);
            auto elapsedSeconds = static_cast<float>(m_fields->currentTimeMs) / 1000.0f + dt;
            m_fields->currentTimeMs = static_cast<int>(std::round(elapsedSeconds * 1000.0f));
            scanGoldRunIntegrity();
        }

        refreshBestBarUI();
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        stopGoldShineAnimation();
        PlayLayer::destroyPlayer(player, object);
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        createBestBarUI();

        auto mod = Mod::get();
        auto pending = mod ? mod->getSavedValue<std::string>(PENDING_GOLD_RUN_KEY, "") : "";
        bool outsideGoldRun = m_level && m_level->isPlatformer() &&
            !pending.empty() && pending == stableLevelSuffix(m_level);
        if (mod && !pending.empty()) mod->setSavedValue<std::string>(PENDING_GOLD_RUN_KEY, "");

        m_fields->goldRunMode = outsideGoldRun;
        m_fields->setupFinished = true;
        m_fields->debugOffsetX = 0.0f;
        m_fields->debugOffsetY = 0.0f;
        if (mod) {
            mod->setSavedValue<float>("layout-debug-offset-x", 0.0f);
            mod->setSavedValue<float>("layout-debug-offset-y", 0.0f);
        }
        syncAttemptState();
        refreshBestBarUI();
        if (outsideGoldRun) showBanner("GOLD RUN ON");


    }

    void resetLevel() {
        stopGoldShineAnimation();
        finalizeGoldRunFailureIfNeeded();
        PlayLayer::resetLevel();
        createBestBarUI();
        syncAttemptState();
        refreshBestBarUI();
        showPendingBanner();
    }

    void updateProgressbar() {
        PlayLayer::updateProgressbar();
        createBestBarUI();

        if (!m_level || m_level->isPlatformer() || !shouldShow()) {
            refreshBestBarUI();
            return;
        }

        syncLiveProgress();
        auto currentPercent = m_fields->currentPercent;
        bool passedOldBest = currentPercent > m_fields->attemptStartBestPercent;

        if (passedOldBest && !m_fields->flashedThisAttempt && m_fields->attemptStartBestPercent < 100) {
            m_fields->flashedThisAttempt = true;
            m_fields->bestChaseActive = true;
            triggerBestFlash("NEW BEST IN PROGRESS");
        }

        if (canSaveRecord() && currentPercent > m_fields->shownBestPercent) {
            m_fields->shownBestPercent = currentPercent;
            Mod::get()->setSavedValue<int>(percentKey(m_level), currentPercent);
        }

        colorCurrentLabel(m_fields->bestChaseActive);
        refreshBestBarUI();
    }

    void updateTimeLabel(int seconds, int centiseconds, bool showMilliseconds) {
        PlayLayer::updateTimeLabel(seconds, centiseconds, showMilliseconds);
        createBestBarUI();

        if (m_level && m_level->isPlatformer()) {
            m_fields->currentTimeMs = std::max(0, seconds * 1000 + centiseconds * 10);
        }

        if (!m_level || !m_level->isPlatformer() || !shouldShow()) {
            refreshBestBarUI();
            return;
        }

        // Keep the diamond and gold crop attached to the live top-bar percent.
        syncLiveProgress();

        if (m_fields->goldRunMode) {
            scanGoldRunIntegrity();
            refreshBestBarUI();
            return;
        }

        if (settingEnabled("platformer-mode")) {
            bool aheadOfBest = m_fields->savedBestTimeMs <= 0 ||
                (m_fields->currentTimeMs > 0 && m_fields->currentTimeMs < m_fields->savedBestTimeMs);
            colorCurrentLabel(aheadOfBest);
        }
        refreshBestBarUI();
    }

    void levelComplete() {
        if (!m_fields->completionHandled && m_level && shouldShow() && canSaveRecord()) {
            m_fields->completionHandled = true;

            if (m_fields->goldRunMode && settingEnabled("gold-run-mode")) {
                recordGoldRun(true);
            }

            if (m_level->isPlatformer()) {
                if (!m_fields->goldRunMode && settingEnabled("platformer-mode")) {
                    auto completedTime = m_fields->currentTimeMs;
                    if (completedTime > 0 &&
                        (m_fields->savedBestTimeMs <= 0 || completedTime < m_fields->savedBestTimeMs)) {
                        m_fields->savedBestTimeMs = completedTime;
                        Mod::get()->setSavedValue<int>(normalTimeKey(m_level), completedTime);
                        triggerBestFlash("NEW PLATFORMER BEST!");
                    }
                }
            }
            else {
                if (m_fields->shownBestPercent < 100) {
                    m_fields->shownBestPercent = 100;
                    Mod::get()->setSavedValue<int>(percentKey(m_level), 100);
                }
                m_fields->bestChaseActive = true;
                m_fields->currentPercent = 100;
                m_fields->currentProgressRatio = 1.0f;
                triggerBestFlash("LEVEL COMPLETE!");
            }
            refreshBestBarUI();
        }

        PlayLayer::levelComplete();
    }

    void onQuit() {
        stopGoldShineAnimation();
        auto fill = m_progressFill ? m_progressFill : m_progressBar;
        if (fill && m_fields->progressFillOpacityCaptured) {
            fill->setOpacity(m_fields->originalProgressFillOpacity);
        }
        finalizeGoldRunFailureIfNeeded();
        PlayLayer::onQuit();
    }
};


class BestBarLeaderboardLayer : public CCLayer {
protected:
    struct RowUI {
        CCLayerColor* background = nullptr;
        CCLayerColor* topHighlight = nullptr;
        CCLayerColor* statusBadge = nullptr;
        CCLabelBMFont* rank = nullptr;
        CCLabelBMFont* player = nullptr;
        CCLabelBMFont* time = nullptr;
        CCLabelBMFont* level = nullptr;
        CCLabelBMFont* details = nullptr;
        CCLabelBMFont* validity = nullptr;
    };

    async::TaskHolder<web::WebResponse> m_request;
    std::array<RowUI, 6> m_rows {};
    CCLayerColor* m_shade = nullptr;
    CCNodeRGBA* m_panelRoot = nullptr;
    CCLabelBMFont* m_status = nullptr;
    CCLabelBMFont* m_emptyTitle = nullptr;
    CCLabelBMFont* m_emptyHint = nullptr;
    CCSprite* m_emptyIcon = nullptr;
    float m_panelWidth = 0.0f;
    float m_panelHeight = 0.0f;
    bool m_refreshing = false;
    bool m_closing = false;

    CCLayerColor* makeRect(
        ccColor4B color,
        CCSize size,
        CCPoint origin,
        int z
    ) {
        auto rect = CCLayerColor::create(color, size.width, size.height);
        if (!rect) return nullptr;
        rect->setPosition(origin);
        m_panelRoot->addChild(rect, z);
        return rect;
    }

    CCLabelBMFont* makeLabel(
        char const* text,
        CCPoint position,
        CCPoint anchor,
        float scale,
        ccColor3B color = ccWHITE,
        int z = 20
    ) {
        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        if (!label) return nullptr;
        label->setPosition(position);
        label->setAnchorPoint(anchor);
        label->setScale(scale);
        label->setColor(color);
        m_panelRoot->addChild(label, z);
        return label;
    }

    void setStatus(std::string const& text, ccColor3B color = ccWHITE) {
        if (!m_status) return;
        m_status->setString(text.c_str());
        m_status->setColor(color);
    }

    void showEmptyState(bool show) {
        if (m_emptyTitle) m_emptyTitle->setVisible(show);
        if (m_emptyHint) m_emptyHint->setVisible(show);
        if (m_emptyIcon) m_emptyIcon->setVisible(show);
    }

    void clearRows() {
        showEmptyState(false);
        for (auto& row : m_rows) {
            if (row.background) row.background->setVisible(false);
            if (row.topHighlight) row.topHighlight->setVisible(false);
            if (row.statusBadge) row.statusBadge->setVisible(false);
            for (auto label : {
                row.rank, row.player, row.time, row.level, row.details, row.validity
            }) {
                if (!label) continue;
                label->setString("");
                label->setVisible(false);
            }
        }
    }

    void styleRow(RowUI& row, size_t index, bool valid) {
        ccColor3B rowColor {54, 14, 48};
        ccColor3B highlightColor {255, 244, 179};

        if (!valid) {
            rowColor = {124, 20, 45};
            highlightColor = {255, 132, 132};
        }
        else if (index == 0) {
            rowColor = {152, 99, 18};
            highlightColor = {255, 237, 132};
        }
        else if (index == 1) {
            rowColor = {88, 76, 108};
            highlightColor = {225, 214, 252};
        }
        else if (index == 2) {
            rowColor = {117, 52, 28};
            highlightColor = {255, 191, 122};
        }

        if (row.background) {
            row.background->setColor(rowColor);
            row.background->setOpacity(252);
        }
        if (row.topHighlight) {
            row.topHighlight->setColor(highlightColor);
            row.topHighlight->setOpacity(valid ? 205 : 235);
        }
        if (row.statusBadge) {
            row.statusBadge->setColor(valid ? ccColor3B {70, 196, 38} : ccColor3B {220, 40, 42});
            row.statusBadge->setOpacity(255);
        }
        if (row.rank) {
            row.rank->setColor(index < 3 && valid ? ccColor3B {255, 247, 153} : ccWHITE);
        }
        if (row.details) {
            row.details->setColor(valid ? ccColor3B {255, 232, 158} : ccColor3B {255, 190, 190});
        }
    }

    void displayRuns(std::vector<LocalLeaderboardRun> const& runs, bool online) {
        clearRows();
        if (runs.empty()) {
            showEmptyState(true);
            setStatus(
                online ? "ONLINE LEADERBOARD IS EMPTY" : "NO SAVED GOLD RUNS YET",
                ccColor3B {255, 232, 100}
            );
            return;
        }

        size_t count = std::min(runs.size(), m_rows.size());
        for (size_t i = 0; i < count; ++i) {
            auto& row = m_rows[i];
            auto const& run = runs[i];
            styleRow(row, i, run.valid);

            if (row.background) row.background->setVisible(true);
            if (row.topHighlight) row.topHighlight->setVisible(true);
            if (row.statusBadge) row.statusBadge->setVisible(true);

            std::string rankText;
            if (i == 0) rankText = "1";
            else if (i == 1) rankText = "2";
            else if (i == 2) rankText = "3";
            else rankText = fmt::format("{}", i + 1);

            if (row.rank) {
                row.rank->setString(rankText.c_str());
                row.rank->setVisible(true);
            }
            if (row.player) {
                row.player->setString(shorten(run.player, 15).c_str());
                row.player->setVisible(true);
            }
            if (row.time) {
                row.time->setString(formatTime(run.timeMs).c_str());
                row.time->setVisible(true);
            }
            if (row.level) {
                row.level->setString(shorten(run.level, 18).c_str());
                row.level->setVisible(true);
            }
            if (row.details) {
                row.details->setString(shorten(runDetails(run), 37).c_str());
                row.details->setVisible(true);
            }
            if (row.validity) {
                row.validity->setString(run.valid ? "VALID" : "CHEATED");
                row.validity->setColor(ccWHITE);
                row.validity->setVisible(true);
            }
        }

        setStatus(
            online ? "ONLINE  -  AUTO-UPDATES EVERY 15 SECONDS" : "LOCAL SAVED RUNS",
            online ? ccColor3B {137, 255, 151} : ccColor3B {255, 232, 100}
        );
    }

    void displayLocalFallback(std::string const& status = "LOCAL SAVED RUNS") {
        auto runs = readLocalRuns();
        displayRuns(runs, false);
        if (!runs.empty()) setStatus(status, ccColor3B {255, 232, 100});
    }

    void handleOnlineResponse(web::WebResponse response) {
        m_refreshing = false;
        if (!response.ok()) {
            displayLocalFallback(fmt::format("ONLINE ERROR {}  -  SHOWING LOCAL", response.code()));
            return;
        }

        auto parsed = response.json();
        if (parsed.isErr()) {
            displayLocalFallback("INVALID ONLINE RESPONSE  -  SHOWING LOCAL");
            return;
        }

        auto root = parsed.unwrap();
        auto const& jsonRuns = root["runs"];
        if (!jsonRuns.isArray()) {
            displayLocalFallback("ONLINE RESPONSE HAS NO RUNS  -  SHOWING LOCAL");
            return;
        }

        std::vector<LocalLeaderboardRun> runs;
        for (auto const& item : jsonRuns) {
            if (!item.isObject()) continue;
            LocalLeaderboardRun run;
            run.level = item["levelName"].asString().unwrapOr("Unknown Level");
            run.player = item["player"].asString().unwrapOr("PLAYER");
            run.timeMs = static_cast<int>(item["timeMs"].asInt().unwrapOr(0));
            run.valid = item["valid"].asBool().unwrapOr(false);
            run.status = item["status"].asString().unwrapOr(run.valid ? "VALID" : "NOT VALID");
            run.difficulty = item["difficulty"].asString().unwrapOr("UNKNOWN");
            run.levelId = item["levelId"].asString().unwrapOr("UNKNOWN");
            run.rewards = static_cast<int>(item["rewards"].asInt().unwrapOr(0));
            run.platformer = item["platformer"].asBool().unwrapOr(true);
            if (run.timeMs > 0) runs.push_back(std::move(run));
        }
        displayRuns(runs, true);
    }

    void finishClose() {
        removeFromParentAndCleanup(true);
    }

public:
    static BestBarLeaderboardLayer* create() {
        auto ret = new BestBarLeaderboardLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        setID("best-bar-leaderboard-layer");
        setKeypadEnabled(true);
        setTouchMode(kCCTouchesOneByOne);
        setTouchPriority(-1000);
        setTouchEnabled(true);

        auto win = CCDirector::get()->getWinSize();
        m_panelWidth = std::min(560.0f, win.width - 18.0f);
        m_panelHeight = std::min(314.0f, win.height - 14.0f);

        m_shade = CCLayerColor::create(ccColor4B {7, 0, 24, 218}, win.width, win.height);
        m_shade->setOpacity(0);
        addChild(m_shade, -5);
        m_shade->runAction(CCFadeTo::create(0.18f, 218));

        m_panelRoot = CCNodeRGBA::create();
        m_panelRoot->setID("best-bar-leaderboard-panel-root");
        m_panelRoot->setPosition(win / 2.0f);
        m_panelRoot->setCascadeOpacityEnabled(true);
        m_panelRoot->setScale(0.74f);
        m_panelRoot->setOpacity(0);
        addChild(m_panelRoot, 2);
        m_panelRoot->runAction(CCSpawn::create(
            CCEaseBackOut::create(CCScaleTo::create(0.26f, 1.0f)),
            CCFadeIn::create(0.13f),
            nullptr
        ));

        float left = -m_panelWidth * 0.5f;
        float bottom = -m_panelHeight * 0.5f;

        // Independent colored rectangles keep the panel readable even when a
        // texture pack replaces Geometry Dash's default square sprites.
        makeRect({0, 0, 0, 145}, {m_panelWidth + 10.0f, m_panelHeight + 10.0f},
            {left - 5.0f, bottom - 7.0f}, -20);
        makeRect({244, 247, 255, 255}, {m_panelWidth + 6.0f, m_panelHeight + 6.0f},
            {left - 3.0f, bottom - 3.0f}, -19);
        makeRect({25, 7, 52, 255}, {m_panelWidth, m_panelHeight},
            {left, bottom}, -18);

        makeRect({47, 10, 40, 255}, {12.0f, m_panelHeight - 14.0f},
            {left + 5.0f, bottom + 7.0f}, -16);
        makeRect({47, 10, 40, 255}, {12.0f, m_panelHeight - 14.0f},
            {left + m_panelWidth - 17.0f, bottom + 7.0f}, -16);

        makeRect({72, 211, 226, 255}, {20.0f, 54.0f},
            {left + 4.0f, bottom + m_panelHeight - 59.0f}, -14);
        makeRect({72, 211, 226, 255}, {20.0f, 54.0f},
            {left + m_panelWidth - 24.0f, bottom + m_panelHeight - 59.0f}, -14);
        makeRect({97, 0, 39, 255}, {m_panelWidth - 44.0f, 54.0f},
            {left + 24.0f, bottom + m_panelHeight - 59.0f}, -15);
        makeRect({255, 231, 120, 90}, {m_panelWidth - 44.0f, 3.0f},
            {left + 24.0f, bottom + m_panelHeight - 12.0f}, -13);

        makeRect({72, 211, 226, 255}, {20.0f, 18.0f},
            {left + 4.0f, bottom + 4.0f}, -14);
        makeRect({72, 211, 226, 255}, {20.0f, 18.0f},
            {left + m_panelWidth - 24.0f, bottom + 4.0f}, -14);
        makeRect({97, 0, 39, 255}, {m_panelWidth - 44.0f, 18.0f},
            {left + 24.0f, bottom + 4.0f}, -15);

        auto title = CCLabelBMFont::create("SPEEDRUN LEADERBOARD", "bigFont.fnt");
        if (title) {
            title->setScale(0.62f);
            title->setPosition({0.0f, bottom + m_panelHeight - 31.0f});
            title->setColor(ccWHITE);
            m_panelRoot->addChild(title, 25);
        }

        m_status = CCLabelBMFont::create("LOADING...", "bigFont.fnt");
        if (m_status) {
            m_status->setScale(0.205f);
            m_status->setPosition({0.0f, bottom + m_panelHeight - 66.0f});
            m_panelRoot->addChild(m_status, 25);
        }

        float contentLeft = left + 26.0f;
        float contentRight = left + m_panelWidth - 26.0f;
        float rankX = contentLeft + 15.0f;
        float playerX = contentLeft + 59.0f;
        float timeX = contentLeft + 187.0f;
        float levelX = contentLeft + 285.0f;
        float validX = contentRight - 8.0f;
        float headerY = bottom + m_panelHeight - 88.0f;

        makeLabel("RANK", {rankX, headerY}, {0.5f, 0.5f}, 0.205f, {255, 236, 100});
        makeLabel("PLAYER", {playerX, headerY}, {0.0f, 0.5f}, 0.205f, {255, 236, 100});
        makeLabel("TIME", {timeX, headerY}, {0.0f, 0.5f}, 0.205f, {255, 236, 100});
        makeLabel("LEVEL / DIFFICULTY", {levelX, headerY}, {0.0f, 0.5f}, 0.185f, {255, 236, 100});
        makeLabel("STATUS", {validX, headerY}, {1.0f, 0.5f}, 0.205f, {255, 236, 100});

        float rowWidth = m_panelWidth - 52.0f;
        float rowHeight = 29.0f;
        float firstY = headerY - 22.0f;
        float rowGap = 31.0f;
        for (size_t i = 0; i < m_rows.size(); ++i) {
            auto& row = m_rows[i];
            float y = firstY - static_cast<float>(i) * rowGap;

            row.background = CCLayerColor::create(
                ccColor4B {65, 12, 62, 252}, rowWidth, rowHeight
            );
            if (row.background) {
                row.background->setPosition({contentLeft, y - rowHeight * 0.5f});
                row.background->setVisible(false);
                m_panelRoot->addChild(row.background, 2);
            }

            row.topHighlight = CCLayerColor::create(
                ccColor4B {255, 255, 255, 84}, rowWidth, 2.0f
            );
            if (row.topHighlight) {
                row.topHighlight->setPosition({contentLeft, y + rowHeight * 0.5f - 2.0f});
                row.topHighlight->setVisible(false);
                m_panelRoot->addChild(row.topHighlight, 3);
            }

            row.statusBadge = CCLayerColor::create(
                ccColor4B {186, 141, 24, 255}, 72.0f, 20.0f
            );
            if (row.statusBadge) {
                row.statusBadge->setPosition({validX - 72.0f, y - 10.0f});
                row.statusBadge->setVisible(false);
                m_panelRoot->addChild(row.statusBadge, 6);
            }

            row.rank = makeLabel("", {rankX, y}, {0.5f, 0.5f}, 0.255f);
            row.player = makeLabel("", {playerX, y}, {0.0f, 0.5f}, 0.245f);
            row.time = makeLabel("", {timeX, y}, {0.0f, 0.5f}, 0.225f);
            row.level = makeLabel("", {levelX, y + 4.5f}, {0.0f, 0.5f}, 0.205f);
            row.details = makeLabel("", {levelX, y - 7.0f}, {0.0f, 0.5f}, 0.125f, {255, 232, 158});
            row.validity = makeLabel("", {validX - 36.0f, y}, {0.5f, 0.5f}, 0.185f, ccWHITE, 25);
        }

        m_emptyIcon = CCSprite::create("bestbar-leaderboard-icon.png"_spr);
        if (m_emptyIcon) {
            m_emptyIcon->setPosition({0.0f, bottom + 118.0f});
            m_emptyIcon->setScale(0.72f);
            m_emptyIcon->setOpacity(210);
            m_panelRoot->addChild(m_emptyIcon, 18);
        }
        m_emptyTitle = makeLabel(
            "NO SAVED RUNS YET",
            {0.0f, bottom + 84.0f},
            {0.5f, 0.5f},
            0.36f,
            {255, 236, 100}
        );
        m_emptyHint = makeLabel(
            "FINISH A GOLD RUN TO CREATE YOUR FIRST ENTRY",
            {0.0f, bottom + 61.0f},
            {0.5f, 0.5f},
            0.17f,
            {220, 220, 240}
        );

        auto menu = CCMenu::create();
        menu->setPosition({0.0f, 0.0f});
        m_panelRoot->addChild(menu, 40);

        auto closeSprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        if (closeSprite) {
            closeSprite->setScale(0.82f);
            auto close = CCMenuItemSpriteExtra::create(
                closeSprite, this, menu_selector(BestBarLeaderboardLayer::onClose)
            );
            close->setPosition({
                left + m_panelWidth - 20.0f,
                bottom + m_panelHeight - 20.0f
            });
            menu->addChild(close);
        }

        auto refreshSprite = ButtonSprite::create("REFRESH");
        if (refreshSprite) {
            refreshSprite->setScale(0.48f);
            auto refresh = CCMenuItemSpriteExtra::create(
                refreshSprite, this, menu_selector(BestBarLeaderboardLayer::onRefresh)
            );
            refresh->setPosition({
                left + m_panelWidth - 74.0f,
                bottom + 13.0f
            });
            menu->addChild(refresh);
        }

        auto footer = makeLabel(
            "LEVEL NAME, DIFFICULTY, REWARDS, ID, TIME, AND RUN VALIDITY",
            {left + 31.0f, bottom + 13.0f},
            {0.0f, 0.5f},
            0.125f,
            {18, 10, 34}
        );
        if (footer) footer->setOpacity(220);

        displayLocalFallback();
        refreshOnline();
        if (settingEnabled("leaderboard-auto-refresh", true)) {
            schedule(schedule_selector(BestBarLeaderboardLayer::onAutoRefresh), 15.0f);
        }
        return true;
    }

    void refreshOnline() {
        if (m_refreshing) return;
        auto api = leaderboardApiBase();
        if (api.empty()) {
            displayLocalFallback("LOCAL RUNS  -  SET API URL FOR ONLINE");
            return;
        }

        m_refreshing = true;
        setStatus("UPDATING ONLINE LEADERBOARD...", ccColor3B {255, 232, 100});
        auto request = web::WebRequest();
        m_request.spawn(
            "Best Bar Leaderboard Refresh",
            request.get(api + "/api/v1/leaderboard?limit=6&includeInvalid=1"),
            [this](web::WebResponse response) {
                handleOnlineResponse(std::move(response));
            }
        );
    }

    void onRefresh(CCObject*) {
        refreshOnline();
    }

    void onAutoRefresh(float) {
        refreshOnline();
    }

    void onClose(CCObject*) {
        if (m_closing) return;
        m_closing = true;
        unscheduleAllSelectors();
        if (m_shade) m_shade->runAction(CCFadeOut::create(0.14f));
        if (m_panelRoot) {
            m_panelRoot->stopAllActions();
            m_panelRoot->runAction(CCSequence::create(
                CCSpawn::create(
                    CCScaleTo::create(0.14f, 0.82f),
                    CCFadeOut::create(0.12f),
                    nullptr
                ),
                CCCallFunc::create(this, callfunc_selector(BestBarLeaderboardLayer::finishClose)),
                nullptr
            ));
        }
        else {
            finishClose();
        }
    }

    bool ccTouchBegan(CCTouch*, CCEvent*) override {
        return true;
    }

    void keyBackClicked() override {
        onClose(nullptr);
    }
};

class $modify(BestBarMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        if (!settingEnabled("enabled") || !settingEnabled("show-leaderboard-button", true)) return true;

        // Use Geode's actual right-side menu instead of a separate absolute menu.
        // This keeps the button correctly sized, laid out, and included in menu
        // animation mods just like the daily chest and other mod-added items.
        NodeIDs::provideFor(this);
        auto menu = typeinfo_cast<CCMenu*>(getChildByID("right-side-menu"));
        if (!menu) menu = typeinfo_cast<CCMenu*>(getChildByID("top-right-menu"));
        if (!menu) return true;

        auto sprite = CircleButtonSprite::createWithSprite(
            "bestbar-leaderboard-icon.png"_spr,
            0.88f,
            CircleBaseColor::Green,
            CircleBaseSize::Medium
        );
        if (!sprite) return true;

        auto item = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(BestBarMenuLayer::onLeaderboard)
        );
        item->setID("best-bar-leaderboard-button");
        menu->addChild(item);
        menu->updateLayout();
        return true;
    }

    void onLeaderboard(CCObject*) {
        auto layer = BestBarLeaderboardLayer::create();
        if (!layer) return;
        CCDirector::get()->getRunningScene()->addChild(layer, 10'000);
    }
};


class $modify(BestBarLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        if (!settingEnabled("enabled") || !settingEnabled("gold-run-mode") ||
            !settingEnabled("show-gold-run-button") || !level || !level->isPlatformer()) {
            return true;
        }

        NodeIDs::provideFor(this);
        auto win = CCDirector::get()->getWinSize();

        auto noBestLabel = findLabelContaining(this, "NO BEST TIME");
        float labelWorldY = win.height * 0.405f;
        if (noBestLabel) {
            centerNodeOnScreenX(noBestLabel);
            labelWorldY = noBestLabel->getParent()->convertToWorldSpace(noBestLabel->getPosition()).y;
        }

        auto menu = CCMenu::create();
        menu->setID("best-bar-level-info-gold-run-menu");
        menu->setPosition({0.0f, 0.0f});
        addChild(menu, 150);

        auto buttonSprite = CCSprite::create("bestbar-speedrun-button.png"_spr);
        if (!buttonSprite) return true;

        float targetSize = 46.0f;
        auto size = buttonSprite->getContentSize();
        buttonSprite->setScale(targetSize / std::max(size.width, 1.0f));

        auto glow = CCSprite::create("bestbar-speedrun-glow.png"_spr);
        if (glow) {
            glow->setPosition(size / 2.0f);
            auto glowSize = glow->getContentSize();
            glow->setScale((size.width / std::max(glowSize.width, 1.0f)) * 1.04f);
            glow->setColor(bestColor());
            glow->setOpacity(68);
            glow->runAction(CCRepeatForever::create(CCSequence::create(
                CCFadeTo::create(0.55f, 118),
                CCFadeTo::create(0.55f, 55),
                nullptr
            )));
            buttonSprite->addChild(glow, -1);
        }

        auto holder = CCNode::create();
        holder->setContentSize({targetSize, targetSize});
        buttonSprite->setPosition(holder->getContentSize() / 2.0f);
        holder->addChild(buttonSprite);

        auto item = CCMenuItemSpriteExtra::create(
            holder, this, menu_selector(BestBarLevelInfoLayer::onOutsideGoldRun)
        );
        item->setID("best-bar-level-info-gold-run-button");
        float outsideButtonX = win.width * 0.5f + 118.0f;
        item->setPosition({outsideButtonX, labelWorldY - 35.0f});
        menu->addChild(item);

        auto label = CCLabelBMFont::create("GOLD RUN", "goldFont.fnt");
        if (label) {
            label->setID("best-bar-level-info-gold-run-label");
            label->setScale(0.27f);
            label->setColor(bestColor());
            label->setPosition({outsideButtonX, labelWorldY - 62.0f});
            addChild(label, 151);
        }
        return true;
    }

    void onOutsideGoldRun(CCObject*) {
        if (!m_level || !m_level->isPlatformer()) return;
        if (auto mod = Mod::get()) {
            mod->setSavedValue<std::string>(PENDING_GOLD_RUN_KEY, stableLevelSuffix(m_level));
        }
        LevelInfoLayer::onPlay(nullptr);
    }
};

class $modify(BestBarPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        for (auto id : {"best-bar-gold-run-button", "best-bar-gold-run-status", "best-bar-gold-run-menu"}) {
            if (auto node = getChildByIDRecursive(id)) {
                node->removeFromParentAndCleanup(true);
            }
        }

        if (!settingEnabled("enabled") || !settingEnabled("show-gold-run-button")) return;

        auto play = PlayLayer::get();
        if (!play) return;
        auto bestBar = static_cast<BestBarPlayLayer*>(play);
        auto winSize = CCDirector::get()->getWinSize();
        bool platformer = bestBar->isPlatformerLevel();
        bool active = platformer && bestBar->isGoldRunMode();
        auto color = bestColor();

        // Prefer the same menu and Y level as the stock retry/practice row.
        // This prevents overlap at non-16:9 resolutions.
        auto retryButton = getChildByIDRecursive("retry-button");
        auto practiceButton = getChildByIDRecursive("practice-button");
        auto anchorButton = retryButton ? retryButton : practiceButton;
        auto menu = anchorButton ? typeinfo_cast<CCMenu*>(anchorButton->getParent()) : nullptr;
        bool ownsMenu = false;
        if (!menu) {
            menu = CCMenu::create();
            menu->setPosition({winSize.width - 142.0f, winSize.height * 0.40f});
            addChild(menu, 500);
            ownsMenu = true;
        }
        if (ownsMenu) menu->setID("best-bar-gold-run-menu");

        auto buttonSprite = CCSprite::create("bestbar-speedrun-button.png"_spr);
        if (!buttonSprite) return;

        float targetButtonSize = 82.0f;
        if (practiceButton) {
            auto practiceSize = practiceButton->getContentSize();
            float displayed = std::abs(practiceSize.width * practiceButton->getScaleX());
            if (displayed > 36.0f && displayed < 130.0f) targetButtonSize = displayed;
        }
        auto buttonSize = buttonSprite->getContentSize();
        auto buttonScale = targetButtonSize / std::max(buttonSize.width, 1.0f);
        buttonSprite->setColor(platformer ? ccWHITE : ccColor3B {128, 128, 138});
        buttonSprite->setOpacity(platformer ? 255 : 190);

        auto glow = CCSprite::create("bestbar-speedrun-glow.png"_spr);
        if (glow) {
            glow->setPosition(buttonSize / 2.0f);
            auto glowSize = glow->getContentSize();
            glow->setScale((buttonSize.width / std::max(glowSize.width, 1.0f)) * 1.05f);
            glow->setColor(color);
            glow->setOpacity(platformer ? (active ? 150 : 52) : 18);
            glow->runAction(CCRepeatForever::create(CCSequence::create(
                CCSpawn::create(CCFadeTo::create(0.55f, platformer ? (active ? 185 : 75) : 24),
                    CCRotateBy::create(0.55f, 18.0f), nullptr),
                CCSpawn::create(CCFadeTo::create(0.55f, platformer ? (active ? 105 : 34) : 12),
                    CCRotateBy::create(0.55f, 18.0f), nullptr),
                nullptr
            )));
            buttonSprite->addChild(glow, -1);
        }

        if (!platformer) {
            auto lock = CCSprite::createWithSpriteFrameName("GJ_lock_001.png");
            if (lock) {
                lock->setScale(0.72f);
                lock->setPosition(buttonSize / 2.0f);
                buttonSprite->addChild(lock, 10);
            }
        }

        if (active) {
            auto offXShadow = CCSprite::create("bestbar-x.png"_spr);
            if (offXShadow) {
                offXShadow->setColor({8, 6, 16});
                offXShadow->setOpacity(215);
                offXShadow->setScale(0.52f);
                offXShadow->setPosition({buttonSize.width * 0.50f + 1.2f, buttonSize.height * 0.50f - 1.2f});
                buttonSprite->addChild(offXShadow, 8);
            }
            auto offX = CCSprite::create("bestbar-x.png"_spr);
            if (offX) {
                offX->setScale(0.52f);
                offX->setPosition({buttonSize.width * 0.50f, buttonSize.height * 0.50f});
                offX->runAction(CCRepeatForever::create(CCSequence::create(
                    CCScaleTo::create(0.42f, 0.56f),
                    CCScaleTo::create(0.42f, 0.52f),
                    nullptr
                )));
                buttonSprite->addChild(offX, 9);
            }
        }

        auto buttonNode = CCNode::create();
        buttonNode->setContentSize({targetButtonSize, targetButtonSize});
        buttonSprite->setScale(buttonScale);
        buttonSprite->setPosition(buttonNode->getContentSize() / 2.0f);
        buttonNode->addChild(buttonSprite);

        auto item = CCMenuItemSpriteExtra::create(
            buttonNode, this, menu_selector(BestBarPauseLayer::onGoldRun)
        );
        item->setID("best-bar-gold-run-button");
        menu->addChild(item, anchorButton ? anchorButton->getZOrder() + 1 : 1);

        if (anchorButton && !ownsMenu) {
            // Same Y, same scale class, directly to the right of retry.
            // Clamp inside the visible screen in the menu's local space.
            float spacing = targetButtonSize + 10.0f;
            auto pos = anchorButton->getPosition();
            item->setPosition({pos.x + spacing, pos.y});
        }
        else {
            item->setPosition({0.0f, 0.0f});
        }

        item->runAction(CCRepeatForever::create(CCSequence::create(
            CCScaleTo::create(0.48f, active ? 1.04f : 1.02f),
            CCScaleTo::create(0.48f, 1.0f),
            nullptr
        )));

        auto status = CCLabelBMFont::create(
            platformer ? "GOLD RUN" : "PLATFORMER ONLY",
            active ? "goldFont.fnt" : "bigFont.fnt"
        );
        status->setID("best-bar-gold-run-status");
        status->setScale(active ? 0.27f : (platformer ? 0.22f : 0.20f));
        status->setColor(platformer ? (active ? color : ccColor3B {235, 235, 235}) : ccColor3B {215, 215, 215});
        auto itemPos = item->getPosition();
        status->setPosition({itemPos.x, itemPos.y - targetButtonSize * 0.60f});
        menu->addChild(status, item->getZOrder() + 1);
    }

    void onGoldRun(CCObject*) {
        auto play = PlayLayer::get();
        if (!play) return;
        auto bestBar = static_cast<BestBarPlayLayer*>(play);

        if (!bestBar->isPlatformerLevel()) {
            FLAlertLayer::create(
                "Gold Run Locked",
                "Gold Run only works on <cy>platformer levels</c>.",
                "OK"
            )->show();
            return;
        }

        if (!settingEnabled("gold-run-mode")) {
            FLAlertLayer::create(
                "Gold Run Disabled",
                "Enable <cy>Gold Run Mode</c> in Best Bar's settings first.",
                "OK"
            )->show();
            return;
        }

        bestBar->toggleGoldRunMode();
        onResume(nullptr);
    }

};
