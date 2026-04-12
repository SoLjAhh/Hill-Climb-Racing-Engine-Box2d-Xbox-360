#include "stdafx.h"
#include "audio.h"
#include "globals.h"

//============================================================================================
// AUDIO PATH CONSTANTS
//============================================================================================
// static = internal linkage, cannot conflict with any other .cpp
static const char* AUDIO_MENU_MUSIC    = "game:\\Audio\\Music\\menu.wav";
static const char* AUDIO_LEVEL1_MUSIC  = "game:\\Audio\\Music\\level1.wav";
static const char* AUDIO_LEVEL2_MUSIC  = "game:\\Audio\\Music\\level2.wav";
static const char* AUDIO_LEVEL3_MUSIC  = "game:\\Audio\\Music\\level3.wav";
static const char* AUDIO_LEVEL4_MUSIC  = "game:\\Audio\\Music\\level4.wav";
static const char* AUDIO_LEVEL5_MUSIC  = "game:\\Audio\\Music\\level5.wav";
static const char* AUDIO_LEVEL6_MUSIC  = "game:\\Audio\\Music\\level6.wav";
static const char* AUDIO_LEVEL7_MUSIC  = "game:\\Audio\\Music\\level7.wav";
static const char* AUDIO_SFX_ENGINE    = "game:\\Audio\\SFX\\engine.wav";
static const char* AUDIO_SFX_COLLISION = "game:\\Audio\\SFX\\collision.wav";
static const char* AUDIO_SFX_SELECT    = "game:\\Audio\\SFX\\select.wav";
static const char* AUDIO_SFX_CONFIRM   = "game:\\Audio\\SFX\\confirm.wav";
static const char* AUDIO_SFX_LEVELUP   = "game:\\Audio\\SFX\\levelup.wav";
static const char* AUDIO_SFX_BOOST     = "game:\\Audio\\SFX\\boost.wav";
static const char* AUDIO_SFX_IDLE      = "game:\\Audio\\SFX\\idle.wav";
static const char* AUDIO_SFX_SPLASH    = "game:\\Audio\\SFX\\splash.wav";
static const char* AUDIO_SFX_HORN      = "game:\\Audio\\SFX\\horn.wav";
static const char* AUDIO_SFX_REVERSE   = "game:\\Audio\\SFX\\reverse.wav";
static const char* AUDIO_SFX_BIG_IMPACT_1   = "game:\\Audio\\SFX\\big_impact_1.wav";
static const char* AUDIO_SFX_BIG_IMPACT_2   = "game:\\Audio\\SFX\\big_impact_2.wav";
static const char* AUDIO_SFX_BIG_IMPACT_3   = "game:\\Audio\\SFX\\big_impact_3.wav";
static const char* AUDIO_SFX_BIG_IMPACT_4   = "game:\\Audio\\SFX\\big_impact_4.wav";
static const char* AUDIO_SFX_SMALL_IMPACT_1 = "game:\\Audio\\SFX\\small_impact_1.wav";
static const char* AUDIO_SFX_SMALL_IMPACT_2 = "game:\\Audio\\SFX\\small_impact_2.wav";

//============================================================================================
// LoadAudioFiles — registers paths with AudioManager (no loading, ATG::WaveFile loads on demand)
//============================================================================================
void LoadAudioFiles() {
    OutputDebugStringA("AudioLoader: Registering audio paths\n");

    // Music — registered by name + path, loaded on first PlayMusic() call
    g_audioManager.AddMusic("menu",   AUDIO_MENU_MUSIC);
    g_audioManager.AddMusic("level1", AUDIO_LEVEL1_MUSIC);
    g_audioManager.AddMusic("level2", AUDIO_LEVEL2_MUSIC);
    g_audioManager.AddMusic("level3", AUDIO_LEVEL3_MUSIC);
    g_audioManager.AddMusic("level4", AUDIO_LEVEL4_MUSIC);
    g_audioManager.AddMusic("level5", AUDIO_LEVEL5_MUSIC);
    g_audioManager.AddMusic("level6", AUDIO_LEVEL6_MUSIC);
    g_audioManager.AddMusic("level7", AUDIO_LEVEL7_MUSIC);

    // SFX
    g_audioManager.AddSFX("engine",    AUDIO_SFX_ENGINE);
    g_audioManager.AddSFX("collision", AUDIO_SFX_COLLISION);
    g_audioManager.AddSFX("select",    AUDIO_SFX_SELECT);
    g_audioManager.AddSFX("confirm",   AUDIO_SFX_CONFIRM);
    g_audioManager.AddSFX("levelup",   AUDIO_SFX_LEVELUP);
    g_audioManager.AddSFX("boost",     AUDIO_SFX_BOOST);
    g_audioManager.AddSFX("idle",      AUDIO_SFX_IDLE);
    g_audioManager.AddSFX("splash",    AUDIO_SFX_SPLASH);
    g_audioManager.AddSFX("horn",      AUDIO_SFX_HORN);
    g_audioManager.AddSFX("reverse",   AUDIO_SFX_REVERSE);
    g_audioManager.AddSFX("big_impact_1",   AUDIO_SFX_BIG_IMPACT_1);
    g_audioManager.AddSFX("big_impact_2",   AUDIO_SFX_BIG_IMPACT_2);
    g_audioManager.AddSFX("big_impact_3",   AUDIO_SFX_BIG_IMPACT_3);
    g_audioManager.AddSFX("big_impact_4",   AUDIO_SFX_BIG_IMPACT_4);
    g_audioManager.AddSFX("small_impact_1", AUDIO_SFX_SMALL_IMPACT_1);
    g_audioManager.AddSFX("small_impact_2", AUDIO_SFX_SMALL_IMPACT_2);

    OutputDebugStringA("AudioLoader: Paths registered OK\n");
}

//============================================================================================
// RegisterCustomLevelMusic — scans g_customLevelMusic[] and registers any with a path set.
// Must be called AFTER LoadConfig() and LoadAudioFiles().
// Registered names: "custom_level_0", "custom_level_1", etc.
//============================================================================================
void RegisterCustomLevelMusic() {
    int count = 0;
    for (int i = 0; i < g_numCustomLevels; i++) {
        if (g_customLevels[i].loaded && g_customLevelMusic[i][0] != '\0') {
            char musicName[AUDIO_NAME_LEN];
            sprintf_s(musicName, sizeof(musicName), "custom_level_%d", i);
            g_audioManager.AddMusic(musicName, g_customLevelMusic[i]);
            count++;
        }
    }
    char dbg[128];
    sprintf_s(dbg, sizeof(dbg), "AudioLoader: Registered %d custom level music tracks\n", count);
    OutputDebugStringA(dbg);
}

//============================================================================================
// PlayMusicForLevel — central helper that plays the correct music for any level index.
// levelSelectIndex: 0-6 = built-in levels, 7+ = custom levels.
// Custom levels with a musicPath set play their own track; otherwise fall back to level1.
//============================================================================================
void PlayMusicForLevel(int levelSelectIndex) {
    // Built-in levels 1-7
    if (levelSelectIndex >= 0 && levelSelectIndex <= 6) {
        const char* builtIn[] = {"level1","level2","level3","level4","level5","level6","level7"};
        g_audioManager.PlayMusic(builtIn[levelSelectIndex]);
        return;
    }

    // Custom levels (index 7+)
    int ci = levelSelectIndex - 7;
    if (ci >= 0 && ci < g_numCustomLevels
        && g_customLevels[ci].loaded
        && g_customLevelMusic[ci][0] != '\0') {
        // Play custom music registered as "custom_level_N"
        char musicName[AUDIO_NAME_LEN];
        sprintf_s(musicName, sizeof(musicName), "custom_level_%d", ci);
        g_audioManager.PlayMusic(musicName);
    } else {
        // No custom music defined — fall back to level 1
        g_audioManager.PlayMusic("level1");
    }
}

//============================================================================================
// ApplyCarSFX — hot-swaps engine/idle/reverse/horn SFX paths for the selected car.
// Custom cars use paths from g_customCarSfx[]; built-in cars restore defaults.
// Call this after every SetCarParameters() so the right sounds play.
//============================================================================================
void ApplyCarSFX(int car) {
    int ci = car - CAR_CUSTOM_BASE;

    // Check if this is a valid custom car with at least one custom SFX
    bool isCustom = (ci >= 0 && ci < g_numCustomCars && g_customCars[ci].loaded);

    // Engine SFX
    if (isCustom && g_customCarSfx[ci].engine[0] != '\0')
        g_audioManager.SetSFXPath("engine", g_customCarSfx[ci].engine);
    else
        g_audioManager.SetSFXPath("engine", AUDIO_SFX_ENGINE);

    // Idle SFX
    if (isCustom && g_customCarSfx[ci].idle[0] != '\0')
        g_audioManager.SetSFXPath("idle", g_customCarSfx[ci].idle);
    else
        g_audioManager.SetSFXPath("idle", AUDIO_SFX_IDLE);

    // Reverse SFX
    if (isCustom && g_customCarSfx[ci].reverse[0] != '\0')
        g_audioManager.SetSFXPath("reverse", g_customCarSfx[ci].reverse);
    else
        g_audioManager.SetSFXPath("reverse", AUDIO_SFX_REVERSE);

    // Horn SFX
    if (isCustom && g_customCarSfx[ci].horn[0] != '\0')
        g_audioManager.SetSFXPath("horn", g_customCarSfx[ci].horn);
    else
        g_audioManager.SetSFXPath("horn", AUDIO_SFX_HORN);
}
