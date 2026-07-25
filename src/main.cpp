#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include <algorithm>
#include <functional>
#include <string>

using namespace geode::prelude;

namespace {
    constexpr char const* MOD_ID = "catchallcat5382.permanent-best";

    bool settingEnabled(char const* key, bool fallback = true) {
        auto mod = Mod::get();
        if (!mod) return fallback;
        return mod->getSettingValue<bool>(key);
    }

    ccColor3B bestColor() {
        auto mod = Mod::get();
        if (!mod) return {255, 215, 64};
        return mod->getSettingValue<ccColor3B>("best-color");
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

        // Keep the newest 20 entries so this never grows forever.
        int separators = static_cast<int>(std::count(history.begin(), history.end(), ';'));
        while (separators >= 20) {
            auto split = history.find(';');
            if (split == std::string::npos) break;
            history.erase(0, split + 1);
            --separators;
        }

        mod->setSavedValue<std::string>(key, history);
    }
}

class $modify(PermanentBestPlayLayer, PlayLayer) {
public:
    struct Fields {
        bool setupFinished = false;
        bool uiReady = false;
        bool flashedThisAttempt = false;
        bool completionHandled = false;
        bool goldChaseActive = false;

        bool goldSpeedrunMode = false;
        bool speedrunRunStarted = false;
        bool speedrunRunCompleted = false;

        int attemptStartBestPercent = 0;
        int shownBestPercent = 0;
        int savedBestTimeMs = 0;
        int currentTimeMs = 0;

        int speedrunBestTimeMs = 0;
        int speedrunLastTimeMs = 0;
        int speedrunRuns = 0;
        int speedrunFailures = 0;
        int speedrunFinishes = 0;

        ccColor3B originalPercentColor = ccWHITE;
        float originalPercentScale = 0.5f;
        std::string pendingBanner;

        CCSprite* goldProgressFill = nullptr;
        CCLayerColor* bestMarker = nullptr;
        CCLabelBMFont* bestLabel = nullptr;
        CCLayerColor* flashLayer = nullptr;
        CCLayerColor* shineBand = nullptr;

        CCLabelBMFont* speedrunTimerLabel = nullptr;
        CCLabelBMFont* speedrunBestLabel = nullptr;
        CCLabelBMFont* speedrunStatsLabel = nullptr;
        CCLabelBMFont* speedrunBannerLabel = nullptr;
    };

    bool isPlatformerLevel() const {
        return m_level && m_level->isPlatformer();
    }

    bool isGoldSpeedrunMode() {
        return m_fields->goldSpeedrunMode;
    }

    void toggleGoldSpeedrunMode() {
        if (!isPlatformerLevel() || !settingEnabled("gold-speedrun-mode")) return;

        if (m_fields->goldSpeedrunMode) {
            finalizeSpeedrunFailureIfNeeded();
            m_fields->goldSpeedrunMode = false;
            m_fields->pendingBanner = "GOLD SPEEDRUN OFF";
        }
        else {
            m_fields->goldSpeedrunMode = true;
            m_fields->pendingBanner = "GOLD SPEEDRUN ON";
        }

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

    void createPermanentBestUI() {
        if (m_fields->uiReady || !m_progressBar || !m_percentageLabel) return;

        m_fields->originalPercentColor = m_percentageLabel->getColor();
        m_fields->originalPercentScale = m_percentageLabel->getScale();
        auto color = bestColor();

        m_fields->goldProgressFill = CCSprite::create("sliderBar2.png");
        if (m_fields->goldProgressFill) {
            m_fields->goldProgressFill->setID("permanent-best-gold-fill");
            m_fields->goldProgressFill->setAnchorPoint({0.0f, 0.0f});
            m_fields->goldProgressFill->setPosition({2.0f, 4.0f});
            m_fields->goldProgressFill->setColor(color);
            m_fields->goldProgressFill->setOpacity(235);
            ccTexParams params = {GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT};
            m_fields->goldProgressFill->getTexture()->setTexParameters(&params);
            m_progressBar->addChild(m_fields->goldProgressFill, 2);
        }

        m_fields->bestMarker = CCLayerColor::create(
            ccColor4B {color.r, color.g, color.b, 255}, 2.0f, 14.0f
        );
        m_fields->bestMarker->setID("permanent-best-marker");
        m_progressBar->addChild(m_fields->bestMarker, 4);

        m_fields->bestLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_fields->bestLabel->setID("permanent-best-label");
        m_fields->bestLabel->setScale(0.38f);
        m_fields->bestLabel->setColor(color);
        addChild(m_fields->bestLabel, 50);

        auto winSize = CCDirector::get()->getWinSize();
        m_fields->flashLayer = CCLayerColor::create(
            ccColor4B {color.r, color.g, color.b, 255}, winSize.width, winSize.height
        );
        m_fields->flashLayer->setID("permanent-best-flash");
        m_fields->flashLayer->setOpacity(0);
        addChild(m_fields->flashLayer, 10'000);

        m_fields->shineBand = CCLayerColor::create(
            ccColor4B {255, 255, 220, 255}, 42.0f, winSize.height * 2.0f
        );
        m_fields->shineBand->setID("permanent-best-shine-band");
        m_fields->shineBand->setRotation(-16.0f);
        m_fields->shineBand->setOpacity(0);
        addChild(m_fields->shineBand, 10'001);

        m_fields->speedrunTimerLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_fields->speedrunTimerLabel->setID("gold-speedrun-timer");
        m_fields->speedrunTimerLabel->setScale(0.62f);
        m_fields->speedrunTimerLabel->setAnchorPoint({1.0f, 0.5f});
        addChild(m_fields->speedrunTimerLabel, 250);

        m_fields->speedrunBestLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_fields->speedrunBestLabel->setID("gold-speedrun-best");
        m_fields->speedrunBestLabel->setScale(0.34f);
        m_fields->speedrunBestLabel->setAnchorPoint({1.0f, 0.5f});
        addChild(m_fields->speedrunBestLabel, 250);

        m_fields->speedrunStatsLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_fields->speedrunStatsLabel->setID("gold-speedrun-stats");
        m_fields->speedrunStatsLabel->setScale(0.25f);
        m_fields->speedrunStatsLabel->setAnchorPoint({1.0f, 0.5f});
        addChild(m_fields->speedrunStatsLabel, 250);

        m_fields->speedrunBannerLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_fields->speedrunBannerLabel->setID("gold-speedrun-banner");
        m_fields->speedrunBannerLabel->setScale(0.52f);
        m_fields->speedrunBannerLabel->setOpacity(0);
        m_fields->speedrunBannerLabel->setPosition({winSize.width / 2.0f, winSize.height - 52.0f});
        addChild(m_fields->speedrunBannerLabel, 300);

        m_fields->uiReady = true;
    }

    void updateColors() {
        if (!m_fields->uiReady) return;
        auto color = bestColor();
        if (m_fields->goldProgressFill) m_fields->goldProgressFill->setColor(color);
        if (m_fields->bestMarker) m_fields->bestMarker->setColor(color);
        if (m_fields->bestLabel) m_fields->bestLabel->setColor(color);
        if (m_fields->flashLayer) m_fields->flashLayer->setColor(color);
        if (m_fields->speedrunBestLabel) m_fields->speedrunBestLabel->setColor(color);
        if (m_fields->speedrunBannerLabel) m_fields->speedrunBannerLabel->setColor(color);
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

    void loadSpeedrunStats() {
        auto mod = Mod::get();
        m_fields->speedrunBestTimeMs = mod->getSavedValue<int>(speedrunBestKey(m_level), 0);
        m_fields->speedrunLastTimeMs = mod->getSavedValue<int>(speedrunLastKey(m_level), 0);
        m_fields->speedrunRuns = mod->getSavedValue<int>(speedrunRunsKey(m_level), 0);
        m_fields->speedrunFailures = mod->getSavedValue<int>(speedrunFailuresKey(m_level), 0);
        m_fields->speedrunFinishes = mod->getSavedValue<int>(speedrunFinishesKey(m_level), 0);
    }

    void syncAttemptState() {
        m_fields->attemptStartBestPercent = loadBestPercent();
        m_fields->shownBestPercent = m_fields->attemptStartBestPercent;
        m_fields->savedBestTimeMs = loadNormalBestTime();
        loadSpeedrunStats();

        m_fields->currentTimeMs = 0;
        m_fields->flashedThisAttempt = false;
        m_fields->completionHandled = false;
        m_fields->goldChaseActive = false;
        m_fields->speedrunRunCompleted = false;
        m_fields->speedrunRunStarted = m_fields->goldSpeedrunMode && isPlatformerLevel();
        restorePercentLabel();
    }

    void showBanner(std::string const& text) {
        if (!m_fields->speedrunBannerLabel || text.empty()) return;
        m_fields->speedrunBannerLabel->stopAllActions();
        m_fields->speedrunBannerLabel->setString(text.c_str());
        m_fields->speedrunBannerLabel->setOpacity(255);
        m_fields->speedrunBannerLabel->setScale(0.52f);
        m_fields->speedrunBannerLabel->runAction(CCSequence::create(
            CCScaleTo::create(0.10f, 0.66f),
            CCScaleTo::create(0.15f, 0.52f),
            CCDelayTime::create(0.75f),
            CCFadeOut::create(0.35f),
            nullptr
        ));
    }

    void showPendingBanner() {
        if (m_fields->pendingBanner.empty()) return;
        auto text = m_fields->pendingBanner;
        m_fields->pendingBanner.clear();
        showBanner(text);
    }

    void triggerBestFlash(std::string const& banner = "") {
        if (!m_fields->uiReady) return;

        if (settingEnabled("gold-flash") && m_fields->flashLayer) {
            m_fields->flashLayer->stopAllActions();
            m_fields->flashLayer->setOpacity(70);
            m_fields->flashLayer->runAction(CCFadeOut::create(0.42f));
        }

        if (settingEnabled("gold-shine") && m_fields->shineBand) {
            auto winSize = CCDirector::get()->getWinSize();
            m_fields->shineBand->stopAllActions();
            m_fields->shineBand->setPosition({-90.0f, -winSize.height * 0.48f});
            m_fields->shineBand->setOpacity(0);
            m_fields->shineBand->runAction(CCSpawn::create(
                CCMoveTo::create(0.58f, {winSize.width + 80.0f, -winSize.height * 0.48f}),
                CCSequence::create(
                    CCFadeIn::create(0.09f),
                    CCDelayTime::create(0.23f),
                    CCFadeOut::create(0.22f),
                    nullptr
                ),
                nullptr
            ));
        }

        if (settingEnabled("pulse-label") && m_fields->bestLabel) {
            m_fields->bestLabel->stopAllActions();
            m_fields->bestLabel->setScale(0.38f);
            m_fields->bestLabel->runAction(CCSequence::create(
                CCScaleTo::create(0.08f, 0.53f),
                CCScaleTo::create(0.18f, 0.38f),
                nullptr
            ));
        }

        if (!banner.empty()) showBanner(banner);
    }

    void setGoldFill(float percent, bool visible) {
        if (!m_fields->goldProgressFill) return;
        percent = std::clamp(percent, 0.0f, 100.0f);
        m_fields->goldProgressFill->setVisible(visible && percent > 0.0f);
        if (!visible || percent <= 0.0f) return;

        auto width = std::max(0.5f, m_progressWidth * (percent / 100.0f));
        m_fields->goldProgressFill->setTextureRect({0.0f, 0.0f, width, m_progressHeight});
    }

    void refreshSpeedrunHUD() {
        auto show = shouldShow() && isPlatformerLevel() && m_fields->goldSpeedrunMode &&
            settingEnabled("gold-speedrun-mode") && settingEnabled("show-speedrun-timer");

        if (m_fields->speedrunTimerLabel) m_fields->speedrunTimerLabel->setVisible(show);
        if (m_fields->speedrunBestLabel) m_fields->speedrunBestLabel->setVisible(show);
        if (m_fields->speedrunStatsLabel) m_fields->speedrunStatsLabel->setVisible(show);
        if (!show) return;

        auto winSize = CCDirector::get()->getWinSize();
        auto x = winSize.width - 12.0f;
        auto y = winSize.height - 58.0f;
        auto color = bestColor();
        bool ahead = m_fields->speedrunBestTimeMs <= 0 ||
            (m_fields->currentTimeMs > 0 && m_fields->currentTimeMs < m_fields->speedrunBestTimeMs);

        m_fields->speedrunTimerLabel->setPosition({x, y});
        m_fields->speedrunTimerLabel->setString(formatTime(m_fields->currentTimeMs).c_str());
        m_fields->speedrunTimerLabel->setColor(ahead ? color : ccWHITE);

        m_fields->speedrunBestLabel->setPosition({x, y - 17.0f});
        m_fields->speedrunBestLabel->setString(
            fmt::format("GOLDEN BEST {}", formatTime(m_fields->speedrunBestTimeMs)).c_str()
        );

        m_fields->speedrunStatsLabel->setPosition({x, y - 31.0f});
        m_fields->speedrunStatsLabel->setString(
            fmt::format("RUNS {}  FINISHES {}  FAILS {}", m_fields->speedrunRuns,
                m_fields->speedrunFinishes, m_fields->speedrunFailures).c_str()
        );
        m_fields->speedrunStatsLabel->setColor({235, 235, 235});
    }

    void refreshPermanentBestUI() {
        createPermanentBestUI();
        if (!m_fields->uiReady) return;
        updateColors();

        bool visible = shouldShow();
        bool platformer = isPlatformerLevel();
        bool progressVisible = m_progressBar && m_progressBar->isVisible();

        if (!visible) {
            setGoldFill(0.0f, false);
            if (m_fields->bestMarker) m_fields->bestMarker->setVisible(false);
            if (m_fields->bestLabel) m_fields->bestLabel->setVisible(false);
            restorePercentLabel();
            refreshSpeedrunHUD();
            return;
        }

        if (platformer) {
            setGoldFill(0.0f, false);
            if (m_fields->bestMarker) m_fields->bestMarker->setVisible(false);

            bool showNormalBest = !m_fields->goldSpeedrunMode && settingEnabled("platformer-mode") &&
                settingEnabled("show-best-label");
            if (m_fields->bestLabel) {
                m_fields->bestLabel->setVisible(showNormalBest && m_percentageLabel && m_percentageLabel->isVisible());
                m_fields->bestLabel->setString(
                    fmt::format("BEST {}", formatTime(m_fields->savedBestTimeMs)).c_str()
                );
                auto winSize = CCDirector::get()->getWinSize();
                m_fields->bestLabel->setPosition({winSize.width / 2.0f, winSize.height - 22.0f});
            }
            refreshSpeedrunHUD();
            return;
        }

        refreshSpeedrunHUD();

        auto currentPercent = std::clamp(PlayLayer::getCurrentPercentInt(), 0, 100);
        float goldPercent = static_cast<float>(m_fields->shownBestPercent);
        if (m_fields->goldChaseActive) goldPercent = static_cast<float>(currentPercent);
        if (m_fields->shownBestPercent >= 100 && settingEnabled("completed-level-full-gold")) goldPercent = 100.0f;

        bool showGoldFill = settingEnabled("show-gold-fill") && progressVisible;
        setGoldFill(goldPercent, showGoldFill);

        bool showMarker = settingEnabled("show-marker") && progressVisible;
        if (m_fields->bestMarker) {
            m_fields->bestMarker->setVisible(showMarker);
            auto percent = std::clamp(m_fields->shownBestPercent, 0, 100);
            auto x = 2.0f + (m_progressWidth * (static_cast<float>(percent) / 100.0f));
            m_fields->bestMarker->setPosition({x - 1.0f, -3.0f});
        }

        bool showLabel = settingEnabled("show-best-label") && progressVisible;
        if (m_fields->bestLabel) {
            m_fields->bestLabel->setVisible(showLabel);
            m_fields->bestLabel->setString(fmt::format("BEST {}%", m_fields->shownBestPercent).c_str());
            auto winSize = CCDirector::get()->getWinSize();
            m_fields->bestLabel->setPosition({winSize.width / 2.0f, winSize.height - 22.0f});
        }
    }

    void recordSpeedrunRun(bool completed) {
        if (!m_level || !m_fields->goldSpeedrunMode || !canSaveRecord()) return;
        auto timeMs = std::max(m_fields->currentTimeMs, 0);
        auto mod = Mod::get();

        ++m_fields->speedrunRuns;
        m_fields->speedrunLastTimeMs = timeMs;
        mod->setSavedValue<int>(speedrunRunsKey(m_level), m_fields->speedrunRuns);
        mod->setSavedValue<int>(speedrunLastKey(m_level), timeMs);

        if (completed) {
            ++m_fields->speedrunFinishes;
            mod->setSavedValue<int>(speedrunFinishesKey(m_level), m_fields->speedrunFinishes);

            bool newBest = timeMs > 0 &&
                (m_fields->speedrunBestTimeMs <= 0 || timeMs < m_fields->speedrunBestTimeMs);
            if (newBest) {
                m_fields->speedrunBestTimeMs = timeMs;
                mod->setSavedValue<int>(speedrunBestKey(m_level), timeMs);
                triggerBestFlash("NEW GOLDEN BEST!");
            }
            else {
                showBanner("GOLD RUN COMPLETE");
            }
        }
        else {
            ++m_fields->speedrunFailures;
            mod->setSavedValue<int>(speedrunFailuresKey(m_level), m_fields->speedrunFailures);
            m_fields->pendingBanner = fmt::format("RUN SAVED  {}", formatTime(timeMs));
        }

        appendSpeedrunHistory(m_level, completed, timeMs);
        m_fields->speedrunRunStarted = false;
        m_fields->speedrunRunCompleted = completed;
    }

    void finalizeSpeedrunFailureIfNeeded() {
        if (!m_fields->setupFinished || !m_fields->goldSpeedrunMode ||
            !m_fields->speedrunRunStarted || m_fields->speedrunRunCompleted) return;

        // Ignore a reset before the timer has actually started.
        if (m_fields->currentTimeMs <= 0) {
            m_fields->speedrunRunStarted = false;
            return;
        }
        recordSpeedrunRun(false);
    }

public:
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        createPermanentBestUI();
        m_fields->goldSpeedrunMode = false;
        m_fields->setupFinished = true;
        syncAttemptState();
        refreshPermanentBestUI();
    }

    void resetLevel() {
        finalizeSpeedrunFailureIfNeeded();
        PlayLayer::resetLevel();
        createPermanentBestUI();
        syncAttemptState();
        refreshPermanentBestUI();
        showPendingBanner();
    }

    void updateProgressbar() {
        PlayLayer::updateProgressbar();
        createPermanentBestUI();

        if (!m_level || m_level->isPlatformer() || !shouldShow()) {
            refreshPermanentBestUI();
            return;
        }

        auto currentPercent = std::clamp(PlayLayer::getCurrentPercentInt(), 0, 100);
        bool touchedOldBest = m_fields->attemptStartBestPercent > 0
            ? currentPercent >= m_fields->attemptStartBestPercent
            : currentPercent > 0;

        if (touchedOldBest && !m_fields->flashedThisAttempt && m_fields->attemptStartBestPercent < 100) {
            m_fields->flashedThisAttempt = true;
            m_fields->goldChaseActive = true;
            triggerBestFlash("NEW BEST IN PROGRESS");
        }

        if (canSaveRecord() && currentPercent > m_fields->shownBestPercent) {
            m_fields->shownBestPercent = currentPercent;
            Mod::get()->setSavedValue<int>(percentKey(m_level), currentPercent);
        }

        colorCurrentLabel(m_fields->goldChaseActive || m_fields->shownBestPercent >= 100);
        refreshPermanentBestUI();
    }

    void updateTimeLabel(int seconds, int centiseconds, bool showMilliseconds) {
        PlayLayer::updateTimeLabel(seconds, centiseconds, showMilliseconds);
        createPermanentBestUI();

        m_fields->currentTimeMs = std::max(0, seconds * 1000 + centiseconds * 10);

        if (!m_level || !m_level->isPlatformer() || !shouldShow()) {
            refreshPermanentBestUI();
            return;
        }

        if (m_fields->goldSpeedrunMode) {
            // The separate top-right speedrun clock is the gold indicator in this mode.
            restorePercentLabel();
            refreshPermanentBestUI();
            return;
        }

        if (settingEnabled("platformer-mode")) {
            bool aheadOfBest = m_fields->savedBestTimeMs <= 0 ||
                (m_fields->currentTimeMs > 0 && m_fields->currentTimeMs < m_fields->savedBestTimeMs);
            colorCurrentLabel(aheadOfBest);
        }
        refreshPermanentBestUI();
    }

    void levelComplete() {
        if (!m_fields->completionHandled && m_level && shouldShow() && canSaveRecord()) {
            m_fields->completionHandled = true;

            if (m_level->isPlatformer()) {
                if (m_fields->goldSpeedrunMode && settingEnabled("gold-speedrun-mode")) {
                    recordSpeedrunRun(true);
                }
                else if (settingEnabled("platformer-mode")) {
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
                m_fields->goldChaseActive = true;
                triggerBestFlash("LEVEL COMPLETE - GOLD BAR UNLOCKED!");
            }
            refreshPermanentBestUI();
        }

        PlayLayer::levelComplete();
    }

    void onQuit() {
        finalizeSpeedrunFailureIfNeeded();
        PlayLayer::onQuit();
    }
};

class $modify(PermanentBestPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        if (!settingEnabled("enabled") || !settingEnabled("show-speedrun-button")) return;

        auto play = PlayLayer::get();
        if (!play) return;
        auto permanent = static_cast<PermanentBestPlayLayer*>(play);
        auto winSize = CCDirector::get()->getWinSize();
        bool platformer = permanent->isPlatformerLevel();
        bool active = platformer && permanent->isGoldSpeedrunMode();
        auto color = bestColor();

        auto menu = CCMenu::create();
        menu->setID("gold-speedrun-menu");
        menu->setPosition({winSize.width - 104.0f, 82.0f});
        addChild(menu, 500);

        auto sprite = CCSprite::createWithSpriteFrameName("GJ_practiceBtn_001.png");
        if (!sprite) return;
        sprite->setScale(0.88f);
        sprite->setColor(platformer ? color : ccColor3B {120, 120, 120});
        sprite->setOpacity(platformer ? 255 : 165);

        auto item = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(PermanentBestPauseLayer::onGoldSpeedrun)
        );
        item->setID("gold-speedrun-button");
        menu->addChild(item);

        auto badge = CCLabelBMFont::create(active ? "ON" : "GOLD", "goldFont.fnt");
        badge->setScale(active ? 0.42f : 0.34f);
        badge->setColor(platformer ? color : ccColor3B {180, 180, 180});
        badge->setPosition({sprite->getContentSize().width / 2.0f, 7.0f});
        sprite->addChild(badge, 5);

        if (!platformer) {
            auto lock = CCSprite::createWithSpriteFrameName("GJ_lock_001.png");
            if (lock) {
                lock->setScale(0.55f);
                lock->setPosition(sprite->getContentSize() / 2.0f);
                sprite->addChild(lock, 10);
            }
        }

        auto status = CCLabelBMFont::create(
            platformer ? (active ? "GOLD RUN ON" : "GOLD SPEEDRUN") : "PLATFORMER ONLY",
            platformer ? "goldFont.fnt" : "bigFont.fnt"
        );
        status->setID("gold-speedrun-status");
        status->setScale(platformer ? 0.27f : 0.23f);
        status->setColor(platformer ? color : ccColor3B {205, 205, 205});
        status->setPosition({0.0f, -39.0f});
        menu->addChild(status);
    }

    void onGoldSpeedrun(CCObject*) {
        auto play = PlayLayer::get();
        if (!play) return;
        auto permanent = static_cast<PermanentBestPlayLayer*>(play);

        if (!permanent->isPlatformerLevel()) {
            FLAlertLayer::create(
                "Gold Speedrun Locked",
                "Gold Speedrun only works on <cy>platformer levels</c>.",
                "OK"
            )->show();
            return;
        }

        if (!settingEnabled("gold-speedrun-mode")) {
            FLAlertLayer::create(
                "Gold Speedrun Disabled",
                "Enable <cy>Gold Speedrun Mode</c> in this mod's Geode settings first.",
                "OK"
            )->show();
            return;
        }

        permanent->toggleGoldSpeedrunMode();
        onResume(nullptr);
    }
};
