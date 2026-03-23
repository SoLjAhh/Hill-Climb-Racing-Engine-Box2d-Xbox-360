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
static const char* AUDIO_SFX_ENGINE    = "game:\\Audio\\SFX\\engine.wav";
static const char* AUDIO_SFX_COLLISION = "game:\\Audio\\SFX\\collision.wav";
static const char* AUDIO_SFX_SELECT    = "game:\\Audio\\SFX\\select.wav";
static const char* AUDIO_SFX_CONFIRM   = "game:\\Audio\\SFX\\confirm.wav";
static const char* AUDIO_SFX_LEVELUP   = "game:\\Audio\\SFX\\levelup.wav";
static const char* AUDIO_SFX_BOOST     = "game:\\Audio\\SFX\\boost.wav";

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

    // SFX
    g_audioManager.AddSFX("engine",    AUDIO_SFX_ENGINE);
    g_audioManager.AddSFX("collision", AUDIO_SFX_COLLISION);
    g_audioManager.AddSFX("select",    AUDIO_SFX_SELECT);
    g_audioManager.AddSFX("confirm",   AUDIO_SFX_CONFIRM);
    g_audioManager.AddSFX("levelup",   AUDIO_SFX_LEVELUP);
    g_audioManager.AddSFX("boost",     AUDIO_SFX_BOOST);

    OutputDebugStringA("AudioLoader: Paths registered OK\n");
}
