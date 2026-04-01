/**
 * Xbox 360 Hill Climb Racing
 * main.cpp - Global definitions, D3D init, game loop
 */

#include "stdafx.h"
#include "globals.h"
#include "audio.h"
#include "cars.h"
#include "terrain.h"
#include "bridge.h"
#include "physics.h"
#include "render.h"
#include "config.h"

//============================================================================================
// NOTE: All compile-time constants are defined in globals.h
// Y1, BACKGROUND_PARALLAX_FACTOR etc are there. Only mutable globals are defined here.
//============================================================================================

//============================================================================================
// MUTABLE SETTINGS - DEFINITIONS (overridden by settings.cfg at startup)
//============================================================================================

float BARRIER_WALL_X       = 10.0f;
float FINISH_LINE_X        = 490.0f;
float BRIDGE_SPAN          = 12.0f;
float BRIDGE_PLANK_HEIGHT  = 0.25f;   // slightly thinner for 20-plank chain
float BRIDGE_PLANK_DENSITY = 3.1f;    // heavier planks = more stable under load
float BRIDGE_PLANK_FRICTION    = 0.8f;   // slightly more grip
float BRIDGE_PLANK_RESTITUTION = 0.01f;  // near-zero bounce = no car bounce on bridge
// ✅ BRIDGE_ANCHOR_HEIGHT = -(plankHeight * 0.5 + 0.1) = -0.225
//    Plank top surface sits ~0.1 below cliff lip for smooth roll-down entry.
float BRIDGE_ANCHOR_HEIGHT     = -0.225f;
float BRIDGE_L2_START_X        = 120.0f;
float BRIDGE_L4_START_X        = 200.0f;
// ✅ BRIDGE_VALLEY_DEPTH: should be 60-70% of terrain amplitude (amplitude=6→depth=4)
float BRIDGE_VALLEY_DEPTH      = 3.5f;  // ~75% of terrain amplitude (4.5)
// BRIDGE_APPROACH_LENGTH: keep tight - only needs to cover the cliff wall width
// A large value flattens the approach hills and reduces the visual cliff height
float BRIDGE_APPROACH_LENGTH   = 4.0f;
// ✅ BRIDGE_VALLEY_SLOPE: near-vertical cliff walls (high value = no flattening)
float BRIDGE_VALLEY_SLOPE      = 8.0f;
// ✅ BRIDGE_ENTRY_NUDGE: tiny overlap onto terrain - set 0 for cliff-lip anchors
float BRIDGE_ENTRY_NUDGE       = 0.0f;
// ✅ BRIDGE_CATENARY_SAG: initial downward sag of planks (world units)
//    Pre-positions planks in a natural hanging curve matching gravity
float BRIDGE_CATENARY_SAG      = 1.2f;
float BRIDGE_VISUAL_WIDTH_SCALE  = 0.5f;
float BRIDGE_VISUAL_HEIGHT_SCALE = 0.25f;  // matches BRIDGE_PLANK_HEIGHT
float BRIDGE_VISUAL_Y_OFFSET     = 0.0f;
// ✅ TERRAIN SETTINGS - Redesigned for smooth HCR-style rolling hills
//    Lower frequencies = broader hills, lower amplitudes = gentler slopes
//    Max slope from wave1 = amplitude * frequency - keep this below SLOPE_LIMIT*10
// ✅ Terrain tuned for no slope limiter (HCR-style direct point connection).
// Max natural slope = amplitude * frequency * ~1.5 (sum of wave multipliers).
// Target max slope ~0.65 (33 degrees) = amplitude * freq * 1.5 / 1.0 → A*f ≈ 0.43
float TERRAIN_AMPLITUDE           = 4.5f;
float TERRAIN_FREQUENCY_BASE      = 0.14f;
float TERRAIN_AMPLITUDE_L2        = 5.0f;
float TERRAIN_FREQUENCY_BASE_L2   = 0.11f;
float TERRAIN_AMPLITUDE_L3        = 4.0f;
float TERRAIN_FREQUENCY_BASE_L3   = 0.16f;
float TERRAIN_AMPLITUDE_L4        = 4.5f;
float TERRAIN_FREQUENCY_BASE_L4   = 0.12f;
float TERRAIN_AMPLITUDE_L5        = 4.0f;
float TERRAIN_FREQUENCY_BASE_L5   = 0.15f;
float TERRAIN_AMPLITUDE_L6        = 5.0f;
float TERRAIN_FREQUENCY_BASE_L6   = 0.13f;

//============================================================================================
// AUDIO PATH CONSTANTS - DEFINITIONS
//============================================================================================

// Audio path constants defined in audio.cpp

// ── Jump feature definitions ──────────────────────────────────────────────
// Placed after the car start (X=12) so the first stretch is always clear.
// Humps (+height) launch the car. Dips (-height) create pits/valleys.
// Width controls sharpness: 3-5 = sharp ramp, 8-12 = broad smooth hill.
JumpDef g_levelJumps[MAX_JUMPS_PER_LEVEL];
int     g_numLevelJumps = 0;

void SetLevelJumps(Level level) {
    g_numLevelJumps = 0;

    if (level == LEVEL_1) {
        // Grass country — mix of launch humps and one pit
        g_levelJumps[0].x = 60.0f; g_levelJumps[0].height = 2.5f; g_levelJumps[0].width = 5.0f;// hump
        g_levelJumps[1].x = 95.0f; g_levelJumps[1].height = -2.0f; g_levelJumps[1].width = 6.0f;// dip
        g_levelJumps[2].x = 150.0f; g_levelJumps[2].height = 3.0f; g_levelJumps[2].width = 4.5f;// big hump
        g_levelJumps[3].x = 225.0f; g_levelJumps[3].height = 2.0f; g_levelJumps[3].width = 6.0f;// hump
        g_levelJumps[4].x = 310.0f; g_levelJumps[4].height = -2.5f; g_levelJumps[4].width = 5.0f;// dip
        g_levelJumps[5].x = 390.0f; g_levelJumps[5].height = 2.5f; g_levelJumps[5].width = 5.0f;// hump near finish
        g_numLevelJumps = 6;
    }
    else if (level == LEVEL_2) {
        // Snow — longer air time, big ramps, fewer dips
        g_levelJumps[0].x = 70.0f; g_levelJumps[0].height = 3.5f; g_levelJumps[0].width = 6.0f;g_levelJumps[1].x = 160.0f; g_levelJumps[1].height = 4.0f; g_levelJumps[1].width = 5.0f;  // big launch before bridge
        g_levelJumps[2].x = 270.0f; g_levelJumps[2].height = 3.0f; g_levelJumps[2].width = 6.5f;g_levelJumps[3].x = 370.0f; g_levelJumps[3].height = 3.5f; g_levelJumps[3].width = 5.5f;
        g_numLevelJumps = 4;
    }
    else if (level == LEVEL_3) {
        // Desert — tight sharp dips and small humps
        g_levelJumps[0].x = 50.0f; g_levelJumps[0].height = 1.5f; g_levelJumps[0].width = 3.5f;g_levelJumps[1].x = 80.0f; g_levelJumps[1].height = -3.0f; g_levelJumps[1].width = 4.0f;  // sharp pit
        g_levelJumps[2].x = 140.0f; g_levelJumps[2].height = 2.0f; g_levelJumps[2].width = 4.0f;g_levelJumps[3].x = 200.0f; g_levelJumps[3].height = -2.5f; g_levelJumps[3].width = 3.5f;  // pit
        g_levelJumps[4].x = 280.0f; g_levelJumps[4].height = 2.5f; g_levelJumps[4].width = 4.5f;g_levelJumps[5].x = 360.0f; g_levelJumps[5].height = -2.0f; g_levelJumps[5].width = 4.0f;
        g_numLevelJumps = 6;
    }
    else if (level == LEVEL_4) {
        // Moon/rocky — huge slow-motion humps
        g_levelJumps[0].x = 55.0f; g_levelJumps[0].height = 4.0f; g_levelJumps[0].width = 9.0f;// wide launch ramp
        g_levelJumps[1].x = 160.0f; g_levelJumps[1].height = 4.5f; g_levelJumps[1].width = 8.0f;g_levelJumps[2].x = 280.0f; g_levelJumps[2].height = 5.0f; g_levelJumps[2].width = 10.0f;  // biggest jump
        g_levelJumps[3].x = 400.0f; g_levelJumps[3].height = 3.5f; g_levelJumps[3].width = 8.0f;g_numLevelJumps = 4;
    }
    else if (level == LEVEL_5) {
        // Cave — tight technical dips and small humps
        g_levelJumps[0].x = 45.0f; g_levelJumps[0].height = -2.0f; g_levelJumps[0].width = 3.5f;g_levelJumps[1].x = 90.0f; g_levelJumps[1].height = 2.0f; g_levelJumps[1].width = 3.0f;
        g_levelJumps[2].x = 145.0f; g_levelJumps[2].height = -3.0f; g_levelJumps[2].width = 4.0f;// deep pit
        g_levelJumps[3].x = 210.0f; g_levelJumps[3].height = 1.5f; g_levelJumps[3].width = 3.5f;g_levelJumps[4].x = 275.0f; g_levelJumps[4].height = -2.5f; g_levelJumps[4].width = 3.5f;
        g_levelJumps[5].x = 340.0f; g_levelJumps[5].height = 2.0f; g_levelJumps[5].width = 3.0f;g_levelJumps[6].x = 410.0f; g_levelJumps[6].height = -2.0f; g_levelJumps[6].width = 4.0f;
        g_numLevelJumps = 7;
    }
    else if (level == LEVEL_6) {
        // Highway — fast, long-distance launches
        g_levelJumps[0].x = 65.0f; g_levelJumps[0].height = 3.0f; g_levelJumps[0].width = 7.0f;g_levelJumps[1].x = 155.0f; g_levelJumps[1].height = 3.5f; g_levelJumps[1].width = 7.5f;
        g_levelJumps[2].x = 255.0f; g_levelJumps[2].height = -2.0f; g_levelJumps[2].width = 5.0f;// dip
        g_levelJumps[3].x = 340.0f; g_levelJumps[3].height = 4.0f; g_levelJumps[3].width = 7.0f;g_levelJumps[4].x = 430.0f; g_levelJumps[4].height = 3.0f; g_levelJumps[4].width = 6.5f;
        g_numLevelJumps = 5;
    }
    // Custom levels: no jumps by default (level designers set them via cfg)
}

//============================================================================================
// BOX2D GLOBALS - DEFINITIONS
//============================================================================================

b2World*      g_pBox2DWorld  = NULL;
b2Body*       g_carChassis   = NULL;
b2Body*       g_wheelRear    = NULL;
b2Body*       g_wheelFront   = NULL;
b2Body*       g_groundBody   = NULL;
b2WheelJoint* g_rearSuspension  = NULL;
b2WheelJoint* g_frontSuspension = NULL;

//============================================================================================
// GAME STATE - DEFINITIONS
//============================================================================================

GameState g_gameState      = STATE_TITLE_SCREEN;
CarType   g_selectedCar    = CAR_JEEP;
int       g_carSelectIndex = 0;
Level     g_currentLevel   = LEVEL_1;
std::vector<TerrainSegment> g_terrainSegments;
float g_terrainStartY   = 0.0f;
float g_terrainEndY     = 0.0f;
float g_fScrollPos      = 0.0f;
float g_motorSpeed      = 0.0f;

//============================================================================================
// DYNAMIC CAMERA STATE
//============================================================================================
float g_camY        = 10.5f;   // CAR_START_Y=8.5 + offset 2.0 = 10.5
float g_camFOV      = 0.7854f; // current field-of-view in radians (PI/4 = 45 degrees)
bool  g_passedFinishLine = false;
int   g_lapCount        = 0;
bool  g_prevDpadLeft    = false;
bool  g_prevDpadRight   = false;
bool  g_prevDpadUp      = false;
bool  g_prevDpadDown    = false;
bool  g_prevAButton     = false;
bool  g_prevStartButton = false;
int   g_pauseMenuIndex  = 0;
int   g_mainMenuIndex   = 0;
int   g_levelSelectIndex = 0;

//============================================================================================
// CAR PHYSICS WORKING VARIABLES - DEFINITIONS
//============================================================================================

float g_visualCarWidth;
float g_visualCarHeight;
float g_visualWheelRadius;
float g_wheelJointFrequency;
float g_wheelJointDamping;
float g_motorMaxSpeed;
float g_motorTorqueRear;
float g_motorTorqueFront;
float g_rideHeightOffset;
float g_wheelRearOffsetX;
float g_wheelRearStartY;
float g_wheelFrontOffsetX;
float g_wheelFrontStartY;

//============================================================================================
// D3D GLOBALS - DEFINITIONS
//============================================================================================

LPDIRECT3DDEVICE9            g_pd3dDevice       = NULL;
LPDIRECT3DVERTEXBUFFER9      g_pVB              = NULL;
LPDIRECT3DVERTEXBUFFER9      g_pSplashVB_Title  = NULL;
LPDIRECT3DVERTEXBUFFER9      g_pSplashVB_CarSelect = NULL;
LPDIRECT3DVERTEXDECLARATION9 g_pVertexDecl      = NULL;
LPDIRECT3DVERTEXSHADER9      g_pVertexShader    = NULL;
LPDIRECT3DPIXELSHADER9       g_pPixelShader     = NULL;
LPDIRECT3DPIXELSHADER9       g_pMenuPixelShader = NULL;
LPDIRECT3DPIXELSHADER9       g_pParticlePixelShader = NULL;
LPDIRECT3DPIXELSHADER9       g_pSolidColourShader   = NULL;
LPDIRECT3DTEXTURE9           g_pTexWater         = NULL;
LPDIRECT3DTEXTURE9           g_pTexWaterDrop     = NULL;
LPDIRECT3DTEXTURE9           g_pTexBoat          = NULL;
LPDIRECT3DTEXTURE9           g_pTexAirplane      = NULL;
LPDIRECT3DTEXTURE9           g_pTexCarSelectIcon_Boat     = NULL;
LPDIRECT3DTEXTURE9           g_pTexCarSelectIcon_Airplane = NULL;
LPDIRECT3DTEXTURE9           g_pTexParticleDirt  = NULL;
LPDIRECT3DTEXTURE9           g_pTexParticleSmoke = NULL;
LPDIRECT3DTEXTURE9           g_pTexParticleSnow  = NULL;
LPDIRECT3DTEXTURE9           g_pTexParticleRocks = NULL;
D3DXMATRIX g_matView;
D3DXMATRIX g_matProj;

LPDIRECT3DTEXTURE9 g_pTexTerrain = NULL;
LPDIRECT3DTEXTURE9 g_pTexTerrain_L2 = NULL;
LPDIRECT3DTEXTURE9 g_pTexTerrain_L3 = NULL;
LPDIRECT3DTEXTURE9 g_pTexTerrain_L4 = NULL;
LPDIRECT3DTEXTURE9 g_pTexTerrain_L5 = NULL;
LPDIRECT3DTEXTURE9 g_pTexTerrain_L6 = NULL;
LPDIRECT3DTEXTURE9 g_pTexTerrain_L7 = NULL;

LPDIRECT3DTEXTURE9 g_pTexBackground = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground_L2 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground_L3 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground_L4 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground_L5 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground_L6 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground_L7 = NULL;

// ✅ MIDGROUND PARALLAX LAYER
LPDIRECT3DTEXTURE9 g_pTexBackground2    = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground2_L2 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground2_L3 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground2_L4 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground2_L5 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground2_L6 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground2_L7 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground3_L7 = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground3     = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground3_L2  = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground3_L3  = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground3_L4  = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground3_L5  = NULL;
LPDIRECT3DTEXTURE9 g_pTexBackground3_L6  = NULL;

// ✅ BRIDGE SYSTEM
LPDIRECT3DTEXTURE9 g_pTexBridgePlank    = NULL;
LPDIRECT3DTEXTURE9 g_pTexSnowflake      = NULL;
b2Body*            g_bridgePlanks[40];         // BRIDGE_PLANK_COUNT = 40
b2Body*            g_bridgeAnchorL       = NULL;
b2Body*            g_bridgeAnchorR       = NULL;
b2Joint*           g_bridgeJoints[41];         // BRIDGE_PLANK_COUNT + 1
bool               g_bridgeActive        = false;

// ✅ ALL 16 CAR TEXTURES
LPDIRECT3DTEXTURE9 g_pTexCar = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarRally = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarMonster = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarQuadBike = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarRacecar = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarFamily = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarOldSchool = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarParamedic = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarBuggy = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSprinter = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarPolice = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarStuntRider = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSuperDiesel = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSuperJeep = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarTrophy = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarChevelle = NULL;
LPDIRECT3DTEXTURE9 g_pTexCar_Blazer = NULL;
LPDIRECT3DTEXTURE9 g_pTexCar_Caprice = NULL;
LPDIRECT3DTEXTURE9 g_pTexCar_Cybertruck = NULL;
LPDIRECT3DTEXTURE9 g_pTexCar_KnightRider = NULL;
LPDIRECT3DTEXTURE9 g_pTexCar_SchoolBus = NULL;
LPDIRECT3DTEXTURE9 g_pTexCar_UsedCar = NULL;

// ✅ TIRE TEXTURES (8 types)
LPDIRECT3DTEXTURE9 g_pTexWheel = NULL;
LPDIRECT3DTEXTURE9 g_pTexWheelMonster = NULL;
LPDIRECT3DTEXTURE9 g_pTexWheelDiesel = NULL;
LPDIRECT3DTEXTURE9 g_pTexWheelClassic = NULL;
LPDIRECT3DTEXTURE9 g_pTexWheelParamedic = NULL;
LPDIRECT3DTEXTURE9 g_pTexWheelStunt = NULL;
LPDIRECT3DTEXTURE9 g_pTexWheelTrophy = NULL;
LPDIRECT3DTEXTURE9 g_pTexWheelTuner = NULL;
LPDIRECT3DTEXTURE9 g_pTexWheelBlazer = NULL;

// ✅ CAR SELECTION ICONS (ALL 15!)
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_jeep = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Rally = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Monster = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_QuadBike = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_RaceCar = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Family = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_OldSchool = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Paramedic = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Buggy = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Sprinter = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Police = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_StuntRider = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_SuperDiesel = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_SuperJeep = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Trophy = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Chevelle = NULL;

// ✅ NEW CAR TEXTURES (22-CAR COLLECTION)
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Blazer = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Caprice = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Cybertruck = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_KnightRider = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_SchoolBus = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_UsedCar = NULL;

LPDIRECT3DTEXTURE9 g_pTexTitle = NULL;
LPDIRECT3DTEXTURE9 g_pTexCarSelectBG = NULL;
LPDIRECT3DTEXTURE9 g_pTexPauseMenuBG = NULL;
LPDIRECT3DTEXTURE9 g_pTexPauseResumeButton = NULL;
LPDIRECT3DTEXTURE9 g_pTexPauseRestartButton = NULL;
LPDIRECT3DTEXTURE9 g_pTexPauseLevelButton = NULL;

// ✅ MAIN MENU TEXTURES
LPDIRECT3DTEXTURE9 g_pTexMainMenuBG = NULL;
LPDIRECT3DTEXTURE9 g_pTexAboutBG = NULL;
LPDIRECT3DTEXTURE9 g_pTexLevelSelectBG = NULL;
LPDIRECT3DTEXTURE9 g_pTexLevelIcon_1 = NULL;
LPDIRECT3DTEXTURE9 g_pTexLevelIcon_2 = NULL;
LPDIRECT3DTEXTURE9 g_pTexLevelIcon_3 = NULL;
LPDIRECT3DTEXTURE9 g_pTexLevelIcon_4 = NULL;
LPDIRECT3DTEXTURE9 g_pTexLevelIcon_5 = NULL;
LPDIRECT3DTEXTURE9 g_pTexLevelIcon_6 = NULL;
LPDIRECT3DTEXTURE9 g_pTexLevelIcon_7 = NULL;

// ✅ MENU BUTTON ICONS (CARS / SETTINGS / ABOUT)
LPDIRECT3DTEXTURE9 g_pTexMenuButtonCars = NULL;
LPDIRECT3DTEXTURE9 g_pTexMenuButtonSettings = NULL;
LPDIRECT3DTEXTURE9 g_pTexMenuButtonAbout = NULL;

// ✅ SETTINGS SCREEN VARIABLES
int   g_settingsIndex = 0;      // 0 = Music slider, 1 = SFX slider
float g_musicVolume   = 0.7f;   // 0.0 – 1.0
float g_sfxVolume     = 0.7f;   // 0.0 – 1.0
LPDIRECT3DTEXTURE9 g_pTexSettingsBG = NULL;

// ✅ HUD GAUGE TEXTURES
LPDIRECT3DTEXTURE9 g_pTexGaugeRPM = NULL;
LPDIRECT3DTEXTURE9 g_pTexGaugeBoost = NULL;
LPDIRECT3DTEXTURE9 g_pTexGaugeNeedle = NULL;

// ✅ HUD DISPLAY VARIABLES
float g_hudSpeedometer = 0.0f;  // 0-1 range for RPM gauge
float g_hudBoost = 0.0f;        // 0-1 range for boost gauge
bool  g_showHUD  = true;

//============================================================================================
// MOD SYSTEM - DEFINITIONS
//============================================================================================

CustomCarDef   g_customCars[MAX_CUSTOM_CARS];
CustomLevelDef g_customLevels[MAX_CUSTOM_LEVELS];
char           g_customLevelMusic[MAX_CUSTOM_LEVELS][128];  // parallel music paths
CustomCarSfx   g_customCarSfx[MAX_CUSTOM_CARS];             // parallel car SFX paths
int            g_numCustomCars   = 0;
int            g_numCustomLevels = 0;
int            g_totalCarSlots   = 24;  // 24 built-in vehicles; up to 1200 custom slots added at runtime
int            g_totalLevelSlots = 7;  // 7 built-in; up to 1200 custom slots added at runtime

XVIDEO_MODE VideoMode;

// ✅ GLOBAL AUDIO MANAGER
TimeStruct   g_Time;
AudioManager g_audioManager;
IXAudio2SourceVoice* g_pEngineVoice  = NULL;  // persistent looping engine voice
bool                 g_enginePlaying = false;  // true while engine SFX is active
IXAudio2SourceVoice* g_pIdleVoice    = NULL;   // persistent looping idle voice
bool                 g_idlePlaying   = false;   // true while idle SFX is active
IXAudio2SourceVoice* g_pReverseVoice = NULL;   // persistent looping reverse voice
bool                 g_reversePlaying = false;  // true while reverse SFX is active

// Physics runs on main thread (Box2D is not thread-safe across LoadLevel)

// Water region pool and heightfield defined in physics.cpp

// ── Particle pool ─────────────────────────────────────────────────────────
Particle g_particles[MAX_PARTICLES];
int      g_numParticles = 0;

// Spawn a single particle — finds a dead slot or overwrites oldest
void SpawnParticle(float x, float y, float vx, float vy,
                   float size, ParticleType type) {
    // Find a dead slot first
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (g_particles[i].type == PARTICLE_NONE) {
            g_particles[i].x            = x;
            g_particles[i].y            = y;
            g_particles[i].vx           = vx;
            g_particles[i].vy           = vy;
            g_particles[i].life         = 1.0f;
            g_particles[i].size         = size;
            g_particles[i].alpha        = 1.0f;
            g_particles[i].type         = type;
            g_particles[i].landedInWater = false;
            if (g_numParticles < MAX_PARTICLES) g_numParticles++;
            return;
        }
    }
    // Pool full — overwrite slot 0 (oldest by index)
    g_particles[0].x = x; g_particles[0].y = y;
    g_particles[0].vx = vx; g_particles[0].vy = vy;
    g_particles[0].life = 1.0f; g_particles[0].size = size;
    g_particles[0].alpha = 1.0f; g_particles[0].type = type;
    g_particles[0].landedInWater = false;
}

// Update all live particles — call once per frame before Render()
void UpdateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (g_particles[i].type == PARTICLE_NONE) continue;

        Particle& p = g_particles[i];

        // Integrate position
        p.x += p.vx * dt;
        p.y += p.vy * dt;

        // Physics per type
        if (p.type == PARTICLE_SMOKE) {
            p.vy += 1.2f * dt;
            p.vx *= 0.97f;
            p.size += 0.4f * dt;
        } else if (p.type == PARTICLE_WAKE) {
            // Boat wake: low flat spray, fades sideways
            p.vy -= 4.0f * dt;   // gentle fall
            p.vx *= 0.88f;
            p.size += 0.3f * dt; // expands as it dissipates
        } else if (p.type == PARTICLE_WATER) {
            if (!p.landedInWater) {
                // Airborne droplet: gravity pulls down, slight air drag
                p.vy -= 12.0f * dt;   // fast fall — water is heavy
                p.vx *= 0.96f;
                // Check if droplet has hit a water surface
                for (int wi = 0; wi < g_numWaterRegions; wi++) {
                    WaterRegion& wr = g_waterRegions[wi];
                    if (!wr.active) continue;
                    if (p.x < wr.x || p.x > wr.right()) continue;
                    float surf = wr.restY; // approximate surface
                    if (p.y <= surf && p.vy < 0.0f) {
                        // Hit water surface — trigger tiny secondary ripple
                        p.landedInWater = true;
                        p.vy = 0.0f;
                        p.vx *= 0.3f;
                        p.y = surf;
                        // Secondary ripple — small displacement
                        SplashWater(p.x, fabsf(p.vy) * 0.02f + 0.03f);
                        p.life = Xbox360Min(p.life, 0.25f); // fade fast on landing
                        break;
                    }
                }
            } else {
                // Landed on water — drift and fade
                p.vx *= 0.8f;
                p.vy  = 0.0f;
            }
        } else {
            p.vy -= 6.0f * dt;
            p.vx *= 0.92f;
        }

        // Age the particle — different lifetimes per type
        float decayRate = 1.5f;
        if (p.type == PARTICLE_SMOKE) decayRate = 0.8f;
        if (p.type == PARTICLE_DUST)  decayRate = 2.0f;
        if (p.type == PARTICLE_WATER) decayRate = 1.8f;
        if (p.type == PARTICLE_WAKE)  decayRate = 1.2f;
        p.life -= decayRate * dt;

        // Fade out in last 40% of life
        p.alpha = (p.life < 0.4f) ? (p.life / 0.4f) : 1.0f;

        if (p.life <= 0.0f) {
            p.type  = PARTICLE_NONE;
            p.alpha = 0.0f;
        }
    }
}


//============================================================================================
// SHADER PROGRAMS
//============================================================================================


const char* g_strVertexShaderProgram =
    " float4x4 matWVP : register(c0); "
    " struct VS_IN { float4 ObjPos : POSITION; float2 Tex : TEXCOORD0; }; "
    " struct VS_OUT { float4 ProjPos : POSITION; float2 Tex : TEXCOORD0; }; "
    " VS_OUT main(VS_IN In) { "
    " VS_OUT Out; "
    " Out.ProjPos = mul(matWVP, In.ObjPos); "
    " Out.Tex = In.Tex; "
    " return Out; "
    " } ";

const char* g_strPixelShaderProgram =
    " sampler2D texSampler : register(s0); "
    " struct PS_IN { float2 Tex : TEXCOORD0; }; "
    " float4 main(PS_IN In) : COLOR { "
    "     return tex2D(texSampler, In.Tex); "
    " }";

const char* g_strMenuPixelShaderProgram =
    " sampler2D texSampler : register(s0); "
    " struct PS_IN { float2 Tex : TEXCOORD0; }; "
    " float4 main(PS_IN In) : COLOR { "
    "     return tex2D(texSampler, In.Tex); "
    " }";

// ✅ Particle pixel shader: samples texture and multiplies by tint from c0.
const char* g_strParticlePixelShaderProgram =
    " sampler2D texSampler : register(s0); "
    " float4 particleTint  : register(c0); "
    " struct PS_IN { float2 Tex : TEXCOORD0; }; "
    " float4 main(PS_IN In) : COLOR { "
    "     float4 t = tex2D(texSampler, In.Tex); "
    "     return t * particleTint; "
    " }";

// ✅ Solid colour shader: outputs c0 directly — NO texture sampling at all.
// Safe with any texture state. Used for water fills and UI colour quads.
const char* g_strSolidColourShaderProgram =
    " float4 solidCol : register(c0); "
    " struct PS_IN { float2 Tex : TEXCOORD0; }; "
    " float4 main(PS_IN In) : COLOR { "
    "     return solidCol; "
    " }";

//============================================================================================
// XBOX 360 COMPATIBLE MIN/MAX
//============================================================================================

// Xbox360Min/Xbox360Max are defined as inline in globals.h

//============================================================================================
// ✅ WAV FILE LOADER STRUCT
void InitTime() {
    LARGE_INTEGER qwTicksPerSec;
    QueryPerformanceFrequency(&qwTicksPerSec);
    g_Time.fSecsPerTick = 1.0f / (float)qwTicksPerSec.QuadPart;
    QueryPerformanceCounter(&g_Time.qwTime);
}

void UpdateTime() {
    LARGE_INTEGER qwNewTime;
    QueryPerformanceCounter(&qwNewTime);
    g_Time.fElapsedTime = g_Time.fSecsPerTick * (float)(qwNewTime.QuadPart - g_Time.qwTime.QuadPart);
    g_Time.qwTime = qwNewTime;
}

HRESULT InitD3D() {
    Direct3D* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    XGetVideoMode(&VideoMode);
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.BackBufferWidth = min(VideoMode.dwDisplayWidth, 1280);
    d3dpp.BackBufferHeight = min(VideoMode.dwDisplayHeight, 720);
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferCount = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pD3D->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice);

    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

    // Initial projection and view — both rebuilt every frame during gameplay
    // (static camera used by menu screens remains in g_matView/g_matProj)
    g_camFOV = D3DX_PI / 4;   // 45 degrees starting FOV
    D3DXMatrixPerspectiveFovLH(&g_matProj, g_camFOV, 16.0f/9.0f, 1.0f, 500.0f);
    // Initial camera: car starts at Y=8.5, camY=10.5, eye=12.0, lookat=10.0
    // No snap on first frame — matches dynamic camera starting state exactly.
    D3DXVECTOR3 vEyePt(0.0f, 12.0f, -15.0f);
    D3DXVECTOR3 vLookatPt(0.0f, 10.0f, 0.0f);
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXMatrixLookAtLH(&g_matView, &vEyePt, &vLookatPt, &vUp);
    return S_OK;
}

// ✅ Forward declaration - LoadModTextures is defined after InitScene
void LoadModTextures();

HRESULT InitScene() {
    LPD3DXBUFFER pVSCode = NULL;
    D3DXCompileShader(g_strVertexShaderProgram, (UINT)strlen(g_strVertexShaderProgram), NULL, NULL, "main", "vs_2_0", 0, &pVSCode, NULL, NULL);
    g_pd3dDevice->CreateVertexShader((DWORD*)pVSCode->GetBufferPointer(), &g_pVertexShader);
    
    LPD3DXBUFFER pPSCode = NULL;
    D3DXCompileShader(g_strPixelShaderProgram, (UINT)strlen(g_strPixelShaderProgram), NULL, NULL, "main", "ps_2_0", 0, &pPSCode, NULL, NULL);
    g_pd3dDevice->CreatePixelShader((DWORD*)pPSCode->GetBufferPointer(), &g_pPixelShader);
    
    LPD3DXBUFFER pMenuPSCode = NULL;
    D3DXCompileShader(g_strMenuPixelShaderProgram, (UINT)strlen(g_strMenuPixelShaderProgram), NULL, NULL, "main", "ps_2_0", 0, &pMenuPSCode, NULL, NULL);
    g_pd3dDevice->CreatePixelShader((DWORD*)pMenuPSCode->GetBufferPointer(), &g_pMenuPixelShader);

    // Compile particle constant-colour shader
    LPD3DXBUFFER pParticlePSCode = NULL;
    D3DXCompileShader(g_strParticlePixelShaderProgram, (UINT)strlen(g_strParticlePixelShaderProgram), NULL, NULL, "main", "ps_2_0", 0, &pParticlePSCode, NULL, NULL);
    if (pParticlePSCode) {
        g_pd3dDevice->CreatePixelShader((DWORD*)pParticlePSCode->GetBufferPointer(), &g_pParticlePixelShader);
        pParticlePSCode->Release();
    }

    LPD3DXBUFFER pSolidPSCode = NULL;
    D3DXCompileShader(g_strSolidColourShaderProgram, (UINT)strlen(g_strSolidColourShaderProgram), NULL, NULL, "main", "ps_2_0", 0, &pSolidPSCode, NULL, NULL);
    if (pSolidPSCode) {
        g_pd3dDevice->CreatePixelShader((DWORD*)pSolidPSCode->GetBufferPointer(), &g_pSolidColourShader);
        pSolidPSCode->Release();
    }

    D3DVERTEXELEMENT9 elems[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    g_pd3dDevice->CreateVertexDeclaration(elems, &g_pVertexDecl);
    g_pd3dDevice->CreateVertexBuffer((NUM_DISPLAY_SEGMENTS * 2) * sizeof(TEXVERTEX), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &g_pVB, NULL);

    TEXVERTEX splashVerts[] = { 
        {-1, 1, 0, 0, 0}, 
        {1, 1, 0, 1, 0}, 
        {-1, -1, 0, 0, 1}, 
        {1, -1, 0, 1, 1} 
    };
    
    g_pd3dDevice->CreateVertexBuffer(sizeof(splashVerts), 0, D3DFVF_XYZ | D3DFVF_TEX1, D3DPOOL_MANAGED, &g_pSplashVB_Title, NULL);
    void* pTitleVerts;
    g_pSplashVB_Title->Lock(0, 0, &pTitleVerts, 0);
    memcpy(pTitleVerts, splashVerts, sizeof(splashVerts));
    g_pSplashVB_Title->Unlock();
    
    g_pd3dDevice->CreateVertexBuffer(sizeof(splashVerts), 0, D3DFVF_XYZ | D3DFVF_TEX1, D3DPOOL_MANAGED, &g_pSplashVB_CarSelect, NULL);
    void* pCarSelectVerts;
    g_pSplashVB_CarSelect->Lock(0, 0, &pCarSelectVerts, 0);
    memcpy(pCarSelectVerts, splashVerts, sizeof(splashVerts));
    g_pSplashVB_CarSelect->Unlock();

    // LOAD TEXTURES
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Levels\\dirt.png", &g_pTexTerrain);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Levels\\dirt_l2.png", &g_pTexTerrain_L2);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Levels\\dirt_l3.png", &g_pTexTerrain_L3);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Levels\\dirt_l4.png", &g_pTexTerrain_L4);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Levels\\dirt_l5.png", &g_pTexTerrain_L5);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Levels\\dirt_l6.png",  &g_pTexTerrain_L6);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Levels\\water_floor.png", &g_pTexTerrain_L7);
    
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground.png", &g_pTexBackground);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground_l2.png", &g_pTexBackground_L2);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground_l3.png", &g_pTexBackground_L3);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground_l4.png", &g_pTexBackground_L4);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground_l5.png", &g_pTexBackground_L5);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground_l6.png",  &g_pTexBackground_L6);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground_l7.png",  &g_pTexBackground_L7);

    // ✅ MIDGROUND PARALLAX LAYER - add these PNGs with transparent sky so far bg shows through
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground2.png",    &g_pTexBackground2);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground2_l2.png", &g_pTexBackground2_L2);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground2_l3.png", &g_pTexBackground2_L3);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground2_l4.png", &g_pTexBackground2_L4);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground2_l5.png", &g_pTexBackground2_L5);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground2_l6.png", &g_pTexBackground2_L6);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground2_l7.png", &g_pTexBackground2_L7);

    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Water\\water.png",      &g_pTexWater);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Water\\water_drop.png", &g_pTexWaterDrop);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\yacht.png",             &g_pTexBoat);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\boat_icon.png",     &g_pTexCarSelectIcon_Boat);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\plane.png",              &g_pTexAirplane);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\airplane_icon.png", &g_pTexCarSelectIcon_Airplane);

    // Layer 3 — nearest foreground (optional, skipped if file missing)
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground3.png",    &g_pTexBackground3);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground3_l2.png", &g_pTexBackground3_L2);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground3_l3.png", &g_pTexBackground3_L3);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground3_l4.png", &g_pTexBackground3_L4);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground3_l5.png", &g_pTexBackground3_L5);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground3_l6.png", &g_pTexBackground3_L6);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Backgrounds\\gamebackground3_l7.png", &g_pTexBackground3_L7);

    // ✅ BRIDGE PLANK TEXTURE - place your PNG at game:\Bridges\bridge_plank.png
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Bridges\\bridge_plank.png", &g_pTexBridgePlank);

    // ✅ Load all custom car and level textures defined in settings.cfg
    LoadModTextures();
    
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\jeep.png", &g_pTexCar);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\rally.png", &g_pTexCarRally);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\monster.png", &g_pTexCarMonster);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\quadbike.png", &g_pTexCarQuadBike);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\racecar.png", &g_pTexCarRacecar);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\family.png", &g_pTexCarFamily);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\oldschool.png", &g_pTexCarOldSchool);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\paramedic.png", &g_pTexCarParamedic);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\buggy.png", &g_pTexCarBuggy);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\sprinter.png", &g_pTexCarSprinter);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\policecar.png", &g_pTexCarPolice);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\stuntrider.png", &g_pTexCarStuntRider);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\superdiesel.png", &g_pTexCarSuperDiesel);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\superjeep.png", &g_pTexCarSuperJeep);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\trophy.png", &g_pTexCarTrophy);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\chevelle.png", &g_pTexCarChevelle);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\blazer.png", &g_pTexCar_Blazer);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\caprice.png", &g_pTexCar_Caprice);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\cybertruck.png", &g_pTexCar_Cybertruck);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\knightrider.png", &g_pTexCar_KnightRider);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\schoolbus.png", &g_pTexCar_SchoolBus);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Cars\\usedcar.png", &g_pTexCar_UsedCar);
    
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Tires\\tire_standard.png", &g_pTexWheel);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Tires\\tire_monster.png", &g_pTexWheelMonster);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Tires\\tire_diesel.png", &g_pTexWheelDiesel);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Tires\\tire_classic.png", &g_pTexWheelClassic);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Tires\\tire_paramedic.png", &g_pTexWheelParamedic);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Tires\\tire_stunt.png", &g_pTexWheelStunt);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Tires\\tire_trophy.png", &g_pTexWheelTrophy);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Tires\\tire_tuner.png", &g_pTexWheelTuner);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Tires\\tire_blazer.png", &g_pTexWheelBlazer);
    
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\jeep_icon.png", &g_pTexCarSelectIcon_jeep);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\rally_icon.png", &g_pTexCarSelectIcon_Rally);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\monster_icon.png", &g_pTexCarSelectIcon_Monster);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\quadbike_icon.png", &g_pTexCarSelectIcon_QuadBike);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\racecar_icon.png", &g_pTexCarSelectIcon_RaceCar);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\family_icon.png", &g_pTexCarSelectIcon_Family);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\oldschool_icon.png", &g_pTexCarSelectIcon_OldSchool);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\paramedic_icon.png", &g_pTexCarSelectIcon_Paramedic);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\buggy_icon.png", &g_pTexCarSelectIcon_Buggy);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\sprinter_icon.png", &g_pTexCarSelectIcon_Sprinter);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\policecar_icon.png", &g_pTexCarSelectIcon_Police);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\stuntrider_icon.png", &g_pTexCarSelectIcon_StuntRider);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\superdiesel_icon.png", &g_pTexCarSelectIcon_SuperDiesel);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\superjeep_icon.png", &g_pTexCarSelectIcon_SuperJeep);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\trophy_icon.png", &g_pTexCarSelectIcon_Trophy);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\chevelle_icon.png", &g_pTexCarSelectIcon_Chevelle);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\blazer_icon.png", &g_pTexCarSelectIcon_Blazer);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\caprice_icon.png", &g_pTexCarSelectIcon_Caprice);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\cybertruck_icon.png", &g_pTexCarSelectIcon_Cybertruck);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\knightrider_icon.png", &g_pTexCarSelectIcon_KnightRider);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\schoolbus_icon.png", &g_pTexCarSelectIcon_SchoolBus);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Cars\\usedcar_icon.png", &g_pTexCarSelectIcon_UsedCar);
    
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\title_screen.png", &g_pTexTitle);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\car_select_bg.png", &g_pTexCarSelectBG);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\pause_menu_bg.png", &g_pTexPauseMenuBG);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\button_resume.png", &g_pTexPauseResumeButton);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\button_restart.png", &g_pTexPauseRestartButton);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\button_level.png", &g_pTexPauseLevelButton);
    
    // ✅ MAIN MENU TEXTURES
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\main_menu_bg.png", &g_pTexMainMenuBG);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\about_bg.png", &g_pTexAboutBG);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\level_select_bg.png", &g_pTexLevelSelectBG);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Levels\\level1_icon.png", &g_pTexLevelIcon_1);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Levels\\level2_icon.png", &g_pTexLevelIcon_2);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Levels\\level3_icon.png", &g_pTexLevelIcon_3);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Levels\\level4_icon.png", &g_pTexLevelIcon_4);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Levels\\level5_icon.png", &g_pTexLevelIcon_5);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Levels\\level6_icon.png", &g_pTexLevelIcon_6);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Icons\\Levels\\level7_icon.png", &g_pTexLevelIcon_7);
    
    // ✅ MENU BUTTON ICONS
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\menu_button_cars.png", &g_pTexMenuButtonCars);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\menu_button_settings.png", &g_pTexMenuButtonSettings);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\menu_button_about.png", &g_pTexMenuButtonAbout);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Menus\\settings_menu.png", &g_pTexSettingsBG);
    
    // ✅ HUD GAUGE TEXTURES
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\HUD\\meter-rpm.png", &g_pTexGaugeRPM);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\HUD\\meter-boost.png", &g_pTexGaugeBoost);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\HUD\\meter-needle.png", &g_pTexGaugeNeedle);

    // ✅ Particle textures
    // Place these PNGs in game:\Particles\ on your Xbox 360
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Particles\\dirt.png",   &g_pTexParticleDirt);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Particles\\smoke.png",  &g_pTexParticleSmoke);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Particles\\snow.png",   &g_pTexParticleSnow);
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Particles\\rocks.png",  &g_pTexParticleRocks);

    // ✅ SNOWFALL OVERLAY TEXTURE — tiled snowflake pattern for Level 2 ambient snow
    D3DXCreateTextureFromFileA(g_pd3dDevice, "game:\\Particles\\snow_flake.png", &g_pTexSnowflake);

    SetCarParameters(CAR_JEEP);
    
    // ✅ Report available memory before audio loads WAV files into RAM
    //    Xbox 360 has 512MB shared CPU+GPU RAM. Textures + WAVs + physics
    //    should stay well under 400MB to leave headroom for the kernel.
    {
        MEMORYSTATUS ms;
        GlobalMemoryStatus(&ms);
        char memMsg[128];
        sprintf_s(memMsg, sizeof(memMsg),
            "RAM: %lu MB total, %lu MB free before audio init\n",
            (unsigned long)(ms.dwTotalPhys  / (1024*1024)),
            (unsigned long)(ms.dwAvailPhys  / (1024*1024)));
        OutputDebugStringA(memMsg);
    }

    // ✅ INITIALIZE AUDIO
    if (!g_audioManager.Initialize()) {
        OutputDebugStringA("WARNING: Audio initialization failed\n");
    } else {
        OutputDebugStringA("Audio: XAudio2 initialized OK on XboxThread5\n");
    }
    
    // ✅ LOAD AUDIO FILES
    LoadAudioFiles();
    RegisterCustomLevelMusic();  // register custom level music from mods (after config + audio init)
    ApplyCarSFX(CAR_JEEP);       // set default SFX paths for initial car

    // ✅ Apply default settings screen volumes
    g_audioManager.SetMusicVolume(g_musicVolume);
    g_audioManager.SetSFXVolume(g_sfxVolume);
    
    InitBox2DPhysics();
    g_currentLevel = LEVEL_1;
    GenerateWrappingTerrain();
    
    return S_OK;
}

//============================================================================================
// LEVEL MANAGEMENT

void Update() {
    XINPUT_STATE state;
    
    if(XInputGetState(0, &state) == ERROR_SUCCESS) {

        // ── Stop all engine-related looping voices when outside gameplay ─────
        // Must be FIRST — before any state handler can early-return.
        // Catches every exit path (pause→title, pause→level select, etc.)
        {
            bool playing = (g_gameState >= STATE_PLAYING_LEVEL1 && g_gameState <= STATE_PLAYING_LEVEL7);
            if (!playing) {
                if (g_enginePlaying) {
                    g_audioManager.StopAndDestroyVoice(g_pEngineVoice);
                    g_pEngineVoice  = NULL;
                    g_enginePlaying = false;
                }
                if (g_idlePlaying) {
                    g_audioManager.StopAndDestroyVoice(g_pIdleVoice);
                    g_pIdleVoice  = NULL;
                    g_idlePlaying = false;
                }
                if (g_reversePlaying) {
                    g_audioManager.StopAndDestroyVoice(g_pReverseVoice);
                    g_pReverseVoice  = NULL;
                    g_reversePlaying = false;
                }
            }
        }

        // TITLE SCREEN
        if (g_gameState == STATE_TITLE_SCREEN) {
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) {
                g_gameState = STATE_MAIN_MENU;
                g_mainMenuIndex = 0;
                g_audioManager.PlayMusic("menu");
            }
            return;
        }
        
        // ✅ MAIN MENU
        if (g_gameState == STATE_MAIN_MENU) {
            bool currDpadLeft = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
            bool currDpadRight = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
            bool currAButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
            
            if (currDpadLeft && !g_prevDpadLeft) {
                g_mainMenuIndex--;
                if (g_mainMenuIndex < 0) g_mainMenuIndex = 2;
            }
            if (currDpadRight && !g_prevDpadRight) {
                g_mainMenuIndex++;
                if (g_mainMenuIndex > 2) g_mainMenuIndex = 0;
            }
            
            if (currAButton && !g_prevAButton) {
                if (g_mainMenuIndex == 0) {
                    g_gameState = STATE_CAR_SELECT;
                    g_carSelectIndex = 0;
                } else if (g_mainMenuIndex == 1) {
                    g_gameState = STATE_SETTINGS;
                } else if (g_mainMenuIndex == 2) {
                    g_gameState = STATE_ABOUT;
                }
            }
            
            g_prevDpadLeft = currDpadLeft;
            g_prevDpadRight = currDpadRight;
            g_prevAButton = currAButton;
            return;
        }
        
        // ✅ SETTINGS
        if (g_gameState == STATE_SETTINGS) {
            bool currDpadUp    = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)    != 0;
            bool currDpadDown  = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)  != 0;
            bool currDpadLeft  = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)  != 0;
            bool currDpadRight = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;

            // D-pad Up/Down: switch between Music (0) and SFX (1)
            if (currDpadUp && !g_prevDpadUp) {
                g_settingsIndex--;
                if (g_settingsIndex < 0) g_settingsIndex = 1;
            }
            if (currDpadDown && !g_prevDpadDown) {
                g_settingsIndex++;
                if (g_settingsIndex > 1) g_settingsIndex = 0;
            }

            // D-pad Left/Right: adjust the selected volume slider
            float step = 0.05f;  // 5% per press
            if (g_settingsIndex == 0) {
                // Music volume
                if (currDpadLeft && !g_prevDpadLeft) {
                    g_musicVolume -= step;
                    if (g_musicVolume < 0.0f) g_musicVolume = 0.0f;
                    g_audioManager.SetMusicVolume(g_musicVolume);
                }
                if (currDpadRight && !g_prevDpadRight) {
                    g_musicVolume += step;
                    if (g_musicVolume > 1.0f) g_musicVolume = 1.0f;
                    g_audioManager.SetMusicVolume(g_musicVolume);
                }
            } else {
                // SFX volume
                if (currDpadLeft && !g_prevDpadLeft) {
                    g_sfxVolume -= step;
                    if (g_sfxVolume < 0.0f) g_sfxVolume = 0.0f;
                    g_audioManager.SetSFXVolume(g_sfxVolume);
                }
                if (currDpadRight && !g_prevDpadRight) {
                    g_sfxVolume += step;
                    if (g_sfxVolume > 1.0f) g_sfxVolume = 1.0f;
                    g_audioManager.SetSFXVolume(g_sfxVolume);
                }
            }

            // B button: back to main menu
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) {
                g_gameState = STATE_MAIN_MENU;
                g_mainMenuIndex = 1;
            }

            g_prevDpadUp    = currDpadUp;
            g_prevDpadDown  = currDpadDown;
            g_prevDpadLeft  = currDpadLeft;
            g_prevDpadRight = currDpadRight;
            return;
        }
        
        // ✅ ABOUT
        if (g_gameState == STATE_ABOUT) {
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) {
                g_gameState = STATE_MAIN_MENU;
                g_mainMenuIndex = 2;
            }
            return;
        }
        
        // CAR SELECTION
        if (g_gameState == STATE_CAR_SELECT) {
            bool currDpadLeft = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
            bool currDpadRight = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
            bool currAButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
            
            // LEFT DPAD 
            if (currDpadLeft && !g_prevDpadLeft) {
                if (g_carSelectIndex > 0) {
                    g_carSelectIndex--;
                } else {
                    g_carSelectIndex = g_totalCarSlots - 1;
                }
            }

            // RIGHT DPAD
            if (currDpadRight && !g_prevDpadRight) {
                if (g_carSelectIndex < g_totalCarSlots - 1) {
                    g_carSelectIndex++;
                } else {
                    g_carSelectIndex = 0;
                }
            }
            
            if (currAButton && !g_prevAButton) {
                if (g_carSelectIndex == 0) g_selectedCar = CAR_JEEP;
                else if (g_carSelectIndex == 1) g_selectedCar = CAR_RALLY;
                else if (g_carSelectIndex == 2) g_selectedCar = CAR_MONSTER;
                else if (g_carSelectIndex == 3) g_selectedCar = CAR_QUAD_BIKE;
                else if (g_carSelectIndex == 4) g_selectedCar = CAR_RACE_CAR;
                else if (g_carSelectIndex == 5) g_selectedCar = CAR_FAMILY;
                else if (g_carSelectIndex == 6) g_selectedCar = CAR_OLD_SCHOOL;
                else if (g_carSelectIndex == 7) g_selectedCar = CAR_PARAMEDIC;
                else if (g_carSelectIndex == 8) g_selectedCar = CAR_BUGGY;
                else if (g_carSelectIndex == 9) g_selectedCar = CAR_SPRINTER;
                else if (g_carSelectIndex == 10) g_selectedCar = CAR_POLICE;
                else if (g_carSelectIndex == 11) g_selectedCar = CAR_STUNT_RIDER;
                else if (g_carSelectIndex == 12) g_selectedCar = CAR_SUPER_DIESEL;
                else if (g_carSelectIndex == 13) g_selectedCar = CAR_SUPER_JEEP;
                else if (g_carSelectIndex == 14) g_selectedCar = CAR_TROPHY_TRUCK;
                else if (g_carSelectIndex == 15) g_selectedCar = CAR_CHEVELLE;
                // ✅ NEW CARS
                else if (g_carSelectIndex == 16) g_selectedCar = CAR_BLAZER;
                else if (g_carSelectIndex == 17) g_selectedCar = CAR_CAPRICE;
                else if (g_carSelectIndex == 18) g_selectedCar = CAR_CYBERTRUCK;
                else if (g_carSelectIndex == 19) g_selectedCar = CAR_KNIGHTRIDER;
                else if (g_carSelectIndex == 20) g_selectedCar = CAR_SCHOOLBUS;
                else if (g_carSelectIndex == 21) g_selectedCar = CAR_USEDCAR;
                else if (g_carSelectIndex == 22) g_selectedCar = CAR_BOAT;
                else if (g_carSelectIndex == 23) g_selectedCar = CAR_AIRPLANE;
                else {
                    // ✅ Custom car slot
                    int ci = g_carSelectIndex - CAR_CUSTOM_BASE;
                    if (ci >= 0 && ci < g_numCustomCars && g_customCars[ci].loaded)
                        g_selectedCar = (CarType)(CAR_CUSTOM_BASE + ci);
                }

                if (g_pBox2DWorld != NULL) {
                    delete g_pBox2DWorld;
                    g_pBox2DWorld = NULL;
                    // ✅ Bridge bodies lived in the old world - clear pointers so
                    //    DestroyBridge() doesn't call into the freshly-deleted world
                    g_bridgeActive = false;
                    for (int bi = 0; bi < BRIDGE_PLANK_COUNT; bi++) g_bridgePlanks[bi] = NULL;
                    g_bridgeAnchorL = NULL;
                    g_bridgeAnchorR = NULL;
                    for (int bi = 0; bi <= BRIDGE_PLANK_COUNT; bi++) g_bridgeJoints[bi] = NULL;
                }
                SetCarParameters(g_selectedCar);
                ApplyCarSFX(g_selectedCar);
                g_isAirplane    = (g_selectedCar == CAR_AIRPLANE);
                g_isBoatLevel   = false;
                InitBox2DPhysics();
            
                // Vehicle↔level enforcement:
                //   Boat     → always water level (LEVEL_7)
                //   Airplane → land levels (LEVEL_1), never water level
                //   Land car → always land (LEVEL_1), never water level
                if (g_selectedCar == CAR_AIRPLANE) {
                    g_gameState = STATE_PLAYING_LEVEL1;
                    LoadLevel(LEVEL_1);
                    g_isAirplane = true;
                    g_audioManager.PlayMusic("level1");
                } else if (g_selectedCar == CAR_BOAT) {
                    // Boat always goes straight to water level — no land levels
                    g_gameState = STATE_PLAYING_LEVEL7;
                    LoadLevel(LEVEL_7);
                    g_audioManager.PlayMusic("level7");
                } else {
                    // Land cars always start at Level 1 — cannot enter water level
                    // g_selectedCar is already a land car — no change needed
                    g_gameState = STATE_PLAYING_LEVEL1;
                    LoadLevel(LEVEL_1);
                    g_audioManager.PlayMusic("level1");
                }
            }
            
            // ✅ B button to return to main menu
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) {
                g_gameState = STATE_MAIN_MENU;
                g_mainMenuIndex = 0;
                g_audioManager.PlayMusic("menu");
            }
            
            g_prevDpadLeft = currDpadLeft;
            g_prevDpadRight = currDpadRight;
            g_prevAButton = currAButton;
            
            return;
        }
        
        // ✅ LEVEL SELECT
        if (g_gameState == STATE_LEVEL_SELECT) {
            bool currDpadLeft = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
            bool currDpadRight = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
            bool currAButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
            bool currBButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
            
            if (currDpadLeft && !g_prevDpadLeft) {
                g_levelSelectIndex--;
                if (g_levelSelectIndex < 0) g_levelSelectIndex = g_totalLevelSlots - 1;
            }
            if (currDpadRight && !g_prevDpadRight) {
                g_levelSelectIndex++;
                if (g_levelSelectIndex >= g_totalLevelSlots) g_levelSelectIndex = 0;
            }
            
            if (currAButton && !g_prevAButton) {
                // Built-in levels 0-5, custom levels 6+
                if (g_levelSelectIndex < 7) {
                    Level levelMap[] = {LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4, LEVEL_5, LEVEL_6, LEVEL_7};
                    g_currentLevel = levelMap[g_levelSelectIndex];
                } else {
                    int ci = g_levelSelectIndex - 7;
                    if (ci >= 0 && ci < MAX_CUSTOM_LEVELS)
                        g_currentLevel = (Level)((int)LEVEL_CUSTOM_1 + ci);
                }

                if (g_pBox2DWorld != NULL) {
                    delete g_pBox2DWorld;
                    g_pBox2DWorld = NULL;
                    g_bridgeActive = false;
                    for (int bi = 0; bi < BRIDGE_PLANK_COUNT; bi++) g_bridgePlanks[bi] = NULL;
                    g_bridgeAnchorL = NULL;
                    g_bridgeAnchorR = NULL;
                    for (int bi = 0; bi <= BRIDGE_PLANK_COUNT; bi++) g_bridgeJoints[bi] = NULL;
                }
                // Vehicle↔level enforcement from level select:
                //   Selecting water level with a land car → swap to boat
                //   Selecting a land level with boat     → swap to jeep
                CarType carToUse = g_selectedCar;
                if (g_currentLevel == LEVEL_7 && g_selectedCar == CAR_AIRPLANE)
                    carToUse = CAR_JEEP;  // airplane can't fly on water level
                else if (g_currentLevel == LEVEL_7 && g_selectedCar != CAR_BOAT)
                    carToUse = CAR_BOAT;
                else if (g_currentLevel != LEVEL_7 && g_selectedCar == CAR_BOAT)
                    carToUse = CAR_JEEP;   // default land car fallback
                g_selectedCar = carToUse; // keep selection consistent
                SetCarParameters(carToUse);
                ApplyCarSFX(carToUse);
                g_isAirplane    = (carToUse == CAR_AIRPLANE);
                InitBox2DPhysics();
                            LoadLevel(g_currentLevel);

                // ✅ Play the matching level music track
                PlayMusicForLevel(g_levelSelectIndex);

                // Built-in game states for levels 0-5; custom levels reuse STATE_PLAYING_LEVEL1
                // as a generic "playing" state - g_currentLevel tracks which level is active
                if (g_levelSelectIndex < 7) {
                    GameState stateMap[] = {STATE_PLAYING_LEVEL1, STATE_PLAYING_LEVEL2, STATE_PLAYING_LEVEL3,
                                            STATE_PLAYING_LEVEL4, STATE_PLAYING_LEVEL5, STATE_PLAYING_LEVEL6,
                                            STATE_PLAYING_LEVEL7};
                    g_gameState = stateMap[g_levelSelectIndex];
                } else {
                    g_gameState = STATE_PLAYING_LEVEL1; // custom levels use generic playing state
                }
            }
            
            if (currBButton) {
                g_gameState = STATE_PAUSE_MENU;
            }
            
            g_prevDpadLeft = currDpadLeft;
            g_prevDpadRight = currDpadRight;
            g_prevAButton = currAButton;
            return;
        }
        
        // PAUSE MENU
        if (g_gameState == STATE_PAUSE_MENU) {
            bool currStartButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
            bool currDpadUp = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
            bool currDpadDown = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
            bool currAButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
            
            if (currStartButton && !g_prevStartButton) {
                GameState prevState = STATE_PLAYING_LEVEL1;
                if (g_currentLevel == LEVEL_1) prevState = STATE_PLAYING_LEVEL1;
                else if (g_currentLevel == LEVEL_2) prevState = STATE_PLAYING_LEVEL2;
                else if (g_currentLevel == LEVEL_3) prevState = STATE_PLAYING_LEVEL3;
                else if (g_currentLevel == LEVEL_4) prevState = STATE_PLAYING_LEVEL4;
                else if (g_currentLevel == LEVEL_5) prevState = STATE_PLAYING_LEVEL5;
                else if (g_currentLevel == LEVEL_6) prevState = STATE_PLAYING_LEVEL6;
                else if (g_currentLevel == LEVEL_7) prevState = STATE_PLAYING_LEVEL7;
                
                g_gameState = prevState;
                g_pauseMenuIndex = 0;
            }
            
            if (currDpadUp && !g_prevDpadLeft) {
                g_pauseMenuIndex--;
                if (g_pauseMenuIndex < 0) g_pauseMenuIndex = 2;
            }
            if (currDpadDown && !g_prevDpadRight) {
                g_pauseMenuIndex++;
                if (g_pauseMenuIndex > 2) g_pauseMenuIndex = 0;
            }
            
            if (currAButton && !g_prevAButton) {
                if (g_pauseMenuIndex == 0) {
                    GameState prevState = STATE_PLAYING_LEVEL1;
                    if (g_currentLevel == LEVEL_1) prevState = STATE_PLAYING_LEVEL1;
                    else if (g_currentLevel == LEVEL_2) prevState = STATE_PLAYING_LEVEL2;
                    else if (g_currentLevel == LEVEL_3) prevState = STATE_PLAYING_LEVEL3;
                    else if (g_currentLevel == LEVEL_4) prevState = STATE_PLAYING_LEVEL4;
                    else if (g_currentLevel == LEVEL_5) prevState = STATE_PLAYING_LEVEL5;
                    else if (g_currentLevel == LEVEL_6) prevState = STATE_PLAYING_LEVEL6;
                else if (g_currentLevel == LEVEL_7) prevState = STATE_PLAYING_LEVEL7;
                    
                    g_gameState = prevState;
                    g_pauseMenuIndex = 0;
                }
                else if (g_pauseMenuIndex == 1) {
                    g_gameState = STATE_TITLE_SCREEN;
                    g_pauseMenuIndex = 0;
                    g_audioManager.PlayMusic("menu");
                }
                else if (g_pauseMenuIndex == 2) {
                    g_gameState = STATE_LEVEL_SELECT;
                    g_levelSelectIndex = (int)g_currentLevel;
                }
            }
            
            g_prevDpadLeft = currDpadUp;
            g_prevDpadRight = currDpadDown;
            g_prevAButton = currAButton;
            g_prevStartButton = currStartButton;
            
            return;
        }
        
        // GAMEPLAY
        bool isPlaying = (g_gameState >= STATE_PLAYING_LEVEL1 && g_gameState <= STATE_PLAYING_LEVEL7);  // L1-L7
        
        if (isPlaying) {
            bool currStartButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
            if (currStartButton && !g_prevStartButton) {
                g_gameState = STATE_PAUSE_MENU;
                g_pauseMenuIndex = 0;
                g_prevStartButton = currStartButton;
                return;
            }
            g_prevStartButton = currStartButton;
            
            float fAccelerate = (float)state.Gamepad.bLeftTrigger / 255.0f;
            float fBrakeReverse = (float)state.Gamepad.bRightTrigger / 255.0f;
            
            if (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) {
                LoadLevel(g_currentLevel);
                return;
            }
            
            float dt = g_Time.fElapsedTime;
            if (dt == 0.0f || dt > 0.05f) return;

            // ── Airplane aerodynamics ────────────────────────────────────────
            // Physics model:
            //   Thrust  — along nose direction (right trigger)
            //   Lift    — WORLD-UP force proportional to forward speed squared.
            //             Using world-up (not wing-perpendicular) prevents the
            //             flip spiral: lift always fights gravity regardless of tilt.
            //   Drag    — opposes velocity, limits top speed
            //   Auto-level — spring torque pulling nose back toward horizontal.
            //             This is the key stability system: like a real plane's
            //             tail, it resists pitch deviations without the pilot.
            //   Pitch   — left stick Y overrides auto-level gently
            if (g_isAirplane && g_carChassis) {
                float throttle   = (float)state.Gamepad.bRightTrigger / 255.0f;
                float brake      = (float)state.Gamepad.bLeftTrigger  / 255.0f;
                float pitchInput = -(float)state.Gamepad.sThumbLY / 32767.0f;
                // Dead zone
                if (fabsf(pitchInput) < 0.12f) pitchInput = 0.0f;

                b2Vec2 vel   = g_carChassis->GetLinearVelocity();
                float  angle = g_carChassis->GetAngle();
                float  angVel= g_carChassis->GetAngularVelocity();
                float  speed = vel.Length();

                // Forward speed along nose axis
                float forwardSpeed = vel.x * cosf(angle) + vel.y * sinf(angle);

                // ── Thrust ────────────────────────────────────────────────────
                if (throttle > 0.01f) {
                    b2Vec2 thrust(AIRPLANE_THRUST * throttle * cosf(angle),
                                  AIRPLANE_THRUST * throttle * sinf(angle));
                    g_carChassis->ApplyForce(thrust, g_carChassis->GetPosition(), true);
                }
                if (brake > 0.01f) {
                    b2Vec2 brakeF(-AIRPLANE_THRUST * 0.35f * brake * cosf(angle),
                                  -AIRPLANE_THRUST * 0.35f * brake * sinf(angle));
                    g_carChassis->ApplyForce(brakeF, g_carChassis->GetPosition(), true);
                }

                // ── Lift — WORLD UP, not wing-perpendicular ───────────────────
                // Proportional to forward speed squared. Below ~5 m/s no lift.
                if (forwardSpeed > 2.0f) {
                    float liftMag = AIRPLANE_LIFT_FACTOR * forwardSpeed * forwardSpeed;
                    // Cap at 2× gravity force so it can't rocket upward
                    float mass = g_carChassis->GetMass();
                    float maxLift = mass * (-GRAVITY) * 2.0f;
                    if (liftMag > maxLift) liftMag = maxLift;
                    g_carChassis->ApplyForce(b2Vec2(0.0f, liftMag),
                                             g_carChassis->GetPosition(), true);
                }

                // ── Drag — opposes velocity ───────────────────────────────────
                if (speed > 0.01f) {
                    b2Vec2 drag(-vel.x * AIRPLANE_DRAG_FACTOR * speed,
                                -vel.y * AIRPLANE_DRAG_FACTOR * speed);
                    g_carChassis->ApplyForce(drag, g_carChassis->GetPosition(), true);
                }

                // ── Auto-level spring ─────────────────────────────────────────
                // Pulls nose toward horizontal (angle=0). This is the tail
                // stabiliser effect — without it the plane has neutral stability
                // and any small tilt grows into a flip.
                // Pitch input shifts the target angle so the player can climb/dive.
                float targetAngle = pitchInput * AIRPLANE_MAX_BANK_ANGLE;
                float angleErr    = targetAngle - angle;
                float levelTorque = AIRPLANE_LEVEL_TORQUE * angleErr;
                g_carChassis->ApplyTorque(levelTorque, true);

                // ── Angular damping — kills oscillation ───────────────────────
                g_carChassis->ApplyTorque(-angVel * AIRPLANE_ANGULAR_DAMP, true);

                // ── Prop spin — accumulates based on throttle ────────────────
                float dt = g_Time.fElapsedTime;
                float propRPM = 8.0f + throttle * 40.0f;  // slow idle to fast cruise
                g_propSpin += propRPM * dt;
                if (g_propSpin > 2.0f * 3.14159265f)
                    g_propSpin -= 2.0f * 3.14159265f;

                // ── Speed clamp ───────────────────────────────────────────────
                if (speed > AIRPLANE_MOTOR_MAX_SPEED) {
                    b2Vec2 clamped = vel;
                    clamped.Normalize();
                    clamped *= AIRPLANE_MOTOR_MAX_SPEED;
                    g_carChassis->SetLinearVelocity(clamped);
                }
            }

            if (g_isBoatLevel && g_carChassis) {
                // ── Boat propulsion: direct force on chassis ─────────────
                // RIGHT trigger = forward throttle (matches land car gas)
                // LEFT trigger  = reverse
                const float BOAT_THRUST     = 22.0f;
                const float BOAT_COAST_DAMP = 0.88f;

                // Re-read triggers in correct mapping for boat
                float boatForward = (float)state.Gamepad.bRightTrigger / 255.0f;
                float boatReverse = (float)state.Gamepad.bLeftTrigger  / 255.0f;

                b2Vec2 vel = g_carChassis->GetLinearVelocity();

                if (boatForward > 0.0f) {
                    b2Vec2 thrustForce(BOAT_THRUST * boatForward, 0.0f);
                    g_carChassis->ApplyForce(thrustForce, g_carChassis->GetPosition(), true);
                } else if (boatReverse > 0.0f) {
                    b2Vec2 thrustForce(-BOAT_THRUST * boatReverse, 0.0f);
                    g_carChassis->ApplyForce(thrustForce, g_carChassis->GetPosition(), true);
                } else {
                    g_carChassis->SetLinearVelocity(b2Vec2(vel.x * BOAT_COAST_DAMP, vel.y));
                }

                // Clamp to motor max speed
                vel = g_carChassis->GetLinearVelocity();
                if (fabsf(vel.x) > g_motorMaxSpeed)
                    g_carChassis->SetLinearVelocity(
                        b2Vec2(vel.x > 0 ? g_motorMaxSpeed : -g_motorMaxSpeed, vel.y));

                // ── Nose tilt based on velocity ───────────────────────────
                // Forward speed tilts nose up (positive torque = counter-clockwise on Xbox = nose up)
                // Reverse tilts nose down. Angular drag is applied in ApplyWaterForces.
                vel = g_carChassis->GetLinearVelocity();
                // ── Speed-based nose tilt using angle spring ────────────────
                // Target angle: nose up when going forward, down when reversing.
                // Max tilt ~12 degrees (0.21 rad) at full speed.
                // Spring pulls toward target, strong damping prevents oscillation.
                // Throttle-driven hull trim:
                // Box2D: positive angle = CCW = for a rightward-facing boat = bow rises.
                // Forward throttle → positive angle (bow up = planing stance)
                // Reverse throttle → slight negative angle (nose dips into water)
                // No input         → return to level
                // Bow lift: strong spring toward target angle driven by throttle.
                // Full throttle = ~30 degrees bow rise (0.52 rad), clearly visible.
                // Damping prevents oscillation while allowing quick response.
                float targetAngle;
                if (boatForward > 0.01f)
                    targetAngle =  0.52f * boatForward;  // strong bow rise at full throttle
                else if (boatReverse > 0.01f)
                    targetAngle = -0.12f * boatReverse;  // nose dip in reverse
                else
                    targetAngle = 0.0f;                  // level when coasting

                float currentAngle  = g_carChassis->GetAngle();
                float angleErr      = targetAngle - currentAngle;
                float springTorque  = angleErr * 45.0f;   // strong spring — quick response
                float dampingTorque = -g_carChassis->GetAngularVelocity() * 9.0f;
                g_carChassis->ApplyTorque(springTorque + dampingTorque, true);

                // Freeze wheel bodies to chassis so they don't float independently
                // This prevents the separation artifact from wheel buoyancy
                if (g_wheelRear && g_carChassis) {
                    b2Vec2 rWheelTarget = g_carChassis->GetPosition();
                    rWheelTarget.x += g_wheelRearOffsetX;
                    rWheelTarget.y -= 0.4f;  // below hull keel
                    g_wheelRear->SetTransform(rWheelTarget, 0.0f);
                    g_wheelRear->SetLinearVelocity(g_carChassis->GetLinearVelocity());
                    g_wheelRear->SetAngularVelocity(0.0f);
                }
                if (g_wheelFront && g_carChassis) {
                    b2Vec2 fWheelTarget = g_carChassis->GetPosition();
                    fWheelTarget.x += g_wheelFrontOffsetX;
                    fWheelTarget.y -= 0.4f;
                    g_wheelFront->SetTransform(fWheelTarget, 0.0f);
                    g_wheelFront->SetLinearVelocity(g_carChassis->GetLinearVelocity());
                    g_wheelFront->SetAngularVelocity(0.0f);
                }

                // Wheel animation
                g_motorSpeed = boatForward > 0.0f ?  g_motorMaxSpeed
                             : boatReverse > 0.0f  ? -g_motorMaxSpeed
                             : g_motorSpeed * 0.95f;
                if (g_rearSuspension) g_rearSuspension->SetMotorSpeed(g_motorSpeed);

            } else {
                // ── Normal land vehicle motor ────────────────────────────
                if (fAccelerate > 0.0f) {
                    float newSpeed = g_motorSpeed + 0.3f * fAccelerate;
                    g_motorSpeed = Xbox360Min(g_motorMaxSpeed, newSpeed);
                }
                else if (fBrakeReverse > 0.0f) {
                    float newSpeed = g_motorSpeed - 0.3f * fBrakeReverse;
                    g_motorSpeed = Xbox360Max(-g_motorMaxSpeed, newSpeed);
                }
                else {
                    g_motorSpeed *= 0.95f;
                }

                if (g_rearSuspension) {
                    g_rearSuspension->SetMotorSpeed(g_motorSpeed);
                }
            }

            if (g_pBox2DWorld != NULL) {
                g_pBox2DWorld->Step(1.0f / 60.0f, 12, 4);
                ApplyWaterForces();
                UpdateWaterHeightfield(g_Time.fElapsedTime);
            }

            // ── Spawn particles ───────────────────────────────────────────
            if (g_wheelRear && g_wheelFront && g_carChassis) {
                b2Vec2 rearPos   = g_wheelRear->GetPosition();
                b2Vec2 frontPos  = g_wheelFront->GetPosition();
                b2Vec2 carVel    = g_carChassis->GetLinearVelocity();
                float  speed     = carVel.Length();
                float  gasInput  = state.Gamepad.bRightTrigger / 255.0f;

                // Ground contact check per wheel
                float  rearTerrainY  = GetTerrainHeightAtX(rearPos.x);
                float  frontTerrainY = GetTerrainHeightAtX(frontPos.x);
                bool   rearOnGround  = (rearPos.y  - g_visualWheelRadius) < (rearTerrainY  + 0.25f);
                bool   frontOnGround = (frontPos.y - g_visualWheelRadius) < (frontTerrainY + 0.25f);

                // ── Rear wheel dirt — skip on boat level ─────────────────
                if (!g_isBoatLevel && rearOnGround && speed > 2.0f) {
                    int bursts = (speed > 15.0f) ? 3 : (speed > 8.0f) ? 2 : 1;
                    for (int b = 0; b < bursts; b++) {
                        float spread = ((float)(b * 37 % 7) - 3.0f) * 0.15f;
                        SpawnParticle(
                            rearPos.x, rearTerrainY + 0.1f,
                            -carVel.x * 0.3f + spread,
                             1.5f + (float)(b % 3) * 0.5f,
                            0.18f + speed * 0.008f,
                            PARTICLE_DIRT);
                    }
                }

                // ── Front wheel dirt — skip on boat level ────────────────
                if (!g_isBoatLevel && frontOnGround && speed > 2.0f) {
                    int bursts = (speed > 15.0f) ? 2 : 1;
                    for (int b = 0; b < bursts; b++) {
                        float spread = ((float)(b * 53 % 5) - 2.0f) * 0.12f;
                        SpawnParticle(
                            frontPos.x, frontTerrainY + 0.1f,
                            -carVel.x * 0.2f + spread,
                             1.2f + (float)(b % 2) * 0.4f,
                            0.14f + speed * 0.006f,
                            PARTICLE_DIRT);
                    }
                }

                // ── Exhaust smoke — land vehicles only, not boat ────────────
                if (gasInput > 0.1f && !g_isBoatLevel && !g_isAirplane) {
                    b2Vec2 cPos  = g_carChassis->GetPosition();
                    float  angle = g_carChassis->GetAngle();
                    float  ex    = cPos.x - 1.0f * cosf(angle);
                    float  ey    = cPos.y - 1.0f * sinf(angle) - 0.55f;
                    float  drift = -0.5f - gasInput * 0.3f;
                    SpawnParticle(ex, ey,
                        drift + carVel.x * 0.04f,
                        0.1f + gasInput * 0.2f,
                        0.15f + gasInput * 0.1f,
                        PARTICLE_SMOKE);
                }

                // ── Boat wake — when boat is on water surface ────────────
                if (g_isBoatLevel && speed > 0.5f) {
                    for (int wi = 0; wi < g_numWaterRegions; wi++) {
                        WaterRegion& wr = g_waterRegions[wi];
                        if (!wr.active) continue;
                        b2Vec2 cPos = g_carChassis->GetPosition();
                        bool onWater = cPos.x >= wr.x && cPos.x <= wr.right()
                                    && fabsf(cPos.y - wr.restY) < 1.5f;
                        if (!onWater) continue;

                        // Left wake — sprays backward-left
                        SpawnParticle(cPos.x - 0.8f, wr.restY,
                            -carVel.x * 0.4f - 1.5f,
                             0.8f + speed * 0.04f,
                             0.06f + speed * 0.003f, PARTICLE_WAKE);
                        // Right wake — sprays backward-right
                        SpawnParticle(cPos.x - 0.8f, wr.restY,
                            -carVel.x * 0.4f + 1.5f,
                             0.8f + speed * 0.04f,
                             0.06f + speed * 0.003f, PARTICLE_WAKE);

                        // Bow spray at speed
                        if (speed > 8.0f) {
                            SpawnParticle(cPos.x + 1.5f, wr.restY,
                                carVel.x * 0.15f,
                                1.5f + speed * 0.06f,
                                0.05f, PARTICLE_WATER);
                        }

                        // ── Bow wave + hull displacement ─────────────────────────
                        // Push columns down under the hull (proportional to speed)
                        // and push columns UP just ahead of the bow (pressure wave).
                        // This creates a visible trough under hull + crest at bow.
                        float colWw  = wr.width / (float)wr.numCols;
                        int   ciw    = (int)((cPos.x - wr.x) / colWw);
                        int   hullHW = (int)(g_visualCarWidth * 0.5f / colWw) + 1; // half-hull in cols

                        // ── Hull bow wave — vel-only injection ────────────────────
                        // All energy goes into velocity; the simulation carries it outward.
                        // Never set .h directly — that kills the wave before it starts.

                        // Hull trough: push columns DOWN under hull (hull displaces water)
                        float troughMag = Xbox360Min(speed * 0.008f, 0.25f);
                        for (int dc = -(int)hullHW; dc <= (int)hullHW; dc++) {
                            int cc = ciw + dc;
                            if (cc < 0 || cc >= wr.numCols) continue;
                            float w = 1.0f - fabsf((float)dc) / (hullHW + 1.0f);
                            wr.cols[cc].vel -= troughMag * w * 5.0f;
                        }

                        // Bow crest: push columns UP ahead of bow — wave travels forward
                        float bowMag = Xbox360Min(speed * 0.012f, 0.30f);
                        int bowFront = (int)hullHW + 1;
                        for (int dc = bowFront; dc <= bowFront + 4; dc++) {
                            int cc = ciw + dc;
                            if (cc < 0 || cc >= wr.numCols) continue;
                            float w = 1.0f - (float)(dc - bowFront) * 0.22f;
                            if (w < 0.0f) w = 0.0f;
                            wr.cols[cc].vel += bowMag * w * 4.0f;
                        }

                        // Stern swell: water surges back in behind hull
                        float sternMag = Xbox360Min(speed * 0.007f, 0.20f);
                        int sternBack = (int)hullHW + 1;
                        for (int dc = -sternBack - 4; dc <= -sternBack; dc++) {
                            int cc = ciw + dc;
                            if (cc < 0 || cc >= wr.numCols) continue;
                            float w = 1.0f - fabsf((float)(dc + sternBack + 2)) * 0.22f;
                            if (w < 0.0f) w = 0.0f;
                            wr.cols[cc].vel += sternMag * w * 3.0f;
                        }
                    }
                }

                // ── Airplane prop smoke ──────────────────────────────────────
                if (g_isAirplane && g_carChassis) {
                    float throttle2 = (float)state.Gamepad.bRightTrigger / 255.0f;
                    if (throttle2 > 0.05f) {
                        b2Vec2 cp = g_carChassis->GetPosition();
                        float  ang = g_carChassis->GetAngle();
                        // Prop at nose (front of plane)
                        float px = cp.x + g_visualCarWidth * 0.45f * cosf(ang);
                        float py = cp.y + g_visualCarWidth * 0.45f * sinf(ang);
                        SpawnParticle(px, py,
                            cosf(ang) * 1.5f + 0.0f,
                            sinf(ang) * 1.5f + 0.15f,
                            0.06f + throttle2 * 0.04f,
                            PARTICLE_SMOKE);
                    }
                }

                // ── Water splash — car driving through water ─────────────
                for (int wi = 0; wi < g_numWaterRegions; wi++) {
                    WaterRegion& wr = g_waterRegions[wi];
                    if (!wr.active) continue;

                    // Check if rear or front wheel is inside this water region
                    bool rearInWater  = rearPos.x  >= wr.x && rearPos.x  <= wr.right()
                                     && rearPos.y  <= wr.restY + 0.5f;
                    bool frontInWater = frontPos.x >= wr.x && frontPos.x <= wr.right()
                                     && frontPos.y <= wr.restY + 0.5f;

                    if ((rearInWater || frontInWater) && speed > 1.0f) {
                        float spawnX  = rearInWater ? rearPos.x : frontPos.x;
                        float spawnY  = wr.restY;
                        int   droplets = (speed > 12.0f) ? 4 : (speed > 5.0f) ? 2 : 1;

                        for (int d = 0; d < droplets; d++) {
                            // Pseudo-random spread using index arithmetic
                            float spreadX = ((float)((d * 31 + (int)(speed * 7)) % 11) - 5.0f) * 0.3f;
                            float spreadY = 2.5f + (float)(d % 3) * 1.2f;
                            float dropVX  = -carVel.x * 0.35f + spreadX;
                            float dropVY  = spreadY + speed * 0.15f;
                            float dropSz  = 0.08f + speed * 0.005f;
                            SpawnParticle(spawnX, spawnY, dropVX, dropVY, dropSz, PARTICLE_WATER);
                        }

                        // Wheel splash: small capped displacement
                        float colWs = wr.width / (float)wr.numCols;
                        int   cis   = (int)((spawnX - wr.x) / colWs);
                        float dipMag = Xbox360Min(speed * 0.005f, 0.08f);
                        for (int dc = -1; dc <= 1; dc++) {
                            int cc = cis + dc;
                            if (cc >= 0 && cc < wr.numCols)
                                wr.cols[cc].vel -= dipMag * 5.0f;  // vel only
                        }
                    }
                }

                // ── Boat landing splash ───────────────────────────────────
                // Track when boat hull goes airborne (chassis rises above restY+margin)
                // and triggers a big ripple + particle burst on re-entry.
                if (g_isBoatLevel) {
                    static bool s_boatAirborne = false;
                    bool boatInWater = false;
                    for (int wi = 0; wi < g_numWaterRegions; wi++) {
                        WaterRegion& wr2 = g_waterRegions[wi];
                        if (!wr2.active) continue;
                        b2Vec2 cp2 = g_carChassis->GetPosition();
                        if (cp2.x >= wr2.x && cp2.x <= wr2.right()
                            && cp2.y <= wr2.restY + 1.2f) {
                            boatInWater = true;
                            break;
                        }
                    }
                    if (!boatInWater) {
                        s_boatAirborne = true;
                    } else if (s_boatAirborne && boatInWater) {
                        s_boatAirborne = false;
                        float impactSpd = fabsf(carVel.y);
                        if (impactSpd > 1.5f) {
                            // Big ripple — scale splash with impact velocity
                            float splashMag = Xbox360Min(impactSpd * 0.25f, 1.8f);
                            SplashWater(g_carChassis->GetPosition().x, splashMag);
                            // Water splash SFX — louder with harder impact
                            g_audioManager.PlaySFX("splash", Xbox360Min(0.5f + impactSpd * 0.08f, 1.0f));

                            // Spray droplets outward from impact point
                            int drops = (int)(impactSpd * 2.0f);
                            if (drops > 16) drops = 16;
                            for (int d = 0; d < drops; d++) {
                                float side  = (d % 2 == 0) ? 1.0f : -1.0f;
                                float vx    = side * (1.5f + (float)(d % 4) * 0.8f);
                                float vy    = 2.0f + (float)(d % 3) * 1.5f + impactSpd * 0.3f;
                                float sz    = 0.06f + impactSpd * 0.008f;
                                for (int wi = 0; wi < g_numWaterRegions; wi++) {
                                    if (g_waterRegions[wi].active) {
                                        SpawnParticle(g_carChassis->GetPosition().x,
                                                      g_waterRegions[wi].restY,
                                                      vx, vy, sz, PARTICLE_WATER);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                // ── Car puddle entry ripple ────────────────────────────────
                // When a wheel first touches a puddle it triggers a ripple proportional
                // to horizontal speed (car diving in rather than dropping from above).
                {
                    static bool s_rearWasInWater   = false;
                    static bool s_frontWasInWater  = false;
                    for (int wi = 0; wi < g_numWaterRegions; wi++) {
                        WaterRegion& wr3 = g_waterRegions[wi];
                        if (!wr3.active) continue;
                        bool rearNow  = rearPos.x  >= wr3.x && rearPos.x  <= wr3.right()
                                     && rearPos.y  <= wr3.restY + 0.5f;
                        bool frontNow = frontPos.x >= wr3.x && frontPos.x <= wr3.right()
                                     && frontPos.y <= wr3.restY + 0.5f;
                        // Rear wheel enters puddle
                        if (rearNow && !s_rearWasInWater && speed > 2.0f) {
                            float mag = Xbox360Min(speed * 0.08f, 0.6f);
                            SplashWater(rearPos.x, mag);
                            // Water splash SFX — volume scales with speed
                            g_audioManager.PlaySFX("splash", Xbox360Min(0.4f + speed * 0.03f, 1.0f));
                        }
                        // Front wheel enters puddle
                        if (frontNow && !s_frontWasInWater && speed > 2.0f) {
                            float mag = Xbox360Min(speed * 0.08f, 0.6f);
                            SplashWater(frontPos.x, mag);
                            // Water splash SFX — volume scales with speed
                            g_audioManager.PlaySFX("splash", Xbox360Min(0.4f + speed * 0.03f, 1.0f));
                        }
                        s_rearWasInWater  = rearNow;
                        s_frontWasInWater = frontNow;
                    }
                }

                // ── Landing dust — rear wheel impact ─────────────────────
                static bool s_wasInAir = false;
                if (!rearOnGround) {
                    s_wasInAir = true;
                } else if (s_wasInAir && rearOnGround && !g_isAirplane) {
                    s_wasInAir = false;
                    float impactSpeed = fabsf(carVel.y);
                    if (impactSpeed > 3.0f) {
                        int count = (int)(impactSpeed * 1.5f);
                        if (count > 12) count = 12;
                        for (int d = 0; d < count; d++) {
                            float ang = 0.3f + (float)(d * 41 % 10) * 0.25f;
                            float spd = 1.5f + impactSpeed * 0.3f;
                            SpawnParticle(
                                rearPos.x, rearTerrainY,
                                (d % 2 == 0 ? -1.0f : 1.0f) * spd * cosf(ang),
                                spd * sinf(ang) * 0.5f,
                                0.2f + impactSpeed * 0.015f,
                                PARTICLE_DUST);
                        }

                        // ── Terrain impact SFX — rear wheel ──────────────────
                        // Big impacts (4 variants) for hard landings, small (2 variants)
                        // for softer ones. Volume scales with vertical impact speed.
                        {
                            static int s_bigIdx   = 0;
                            static int s_smallIdx = 0;
                            bool landedInWater = false;
                            for (int wi = 0; wi < g_numWaterRegions; wi++) {
                                WaterRegion& wrChk = g_waterRegions[wi];
                                if (!wrChk.active) continue;
                                if (rearPos.x >= wrChk.x && rearPos.x <= wrChk.right()
                                    && rearPos.y <= wrChk.restY + 1.0f) {
                                    landedInWater = true;
                                    break;
                                }
                            }
                            if (!landedInWater && !g_isBoatLevel) {
                                if (impactSpeed >= 8.0f) {
                                    // Big impact — cycle through 4 variants
                                    const char* bigNames[] = {
                                        "big_impact_1","big_impact_2",
                                        "big_impact_3","big_impact_4"
                                    };
                                    float vol = Xbox360Min(0.5f + impactSpeed * 0.04f, 1.0f);
                                    g_audioManager.PlaySFX(bigNames[s_bigIdx], vol);
                                    s_bigIdx = (s_bigIdx + 1) % 4;
                                } else {
                                    // Small impact — cycle through 2 variants
                                    const char* smallNames[] = {
                                        "small_impact_1","small_impact_2"
                                    };
                                    float vol = Xbox360Min(0.3f + impactSpeed * 0.05f, 0.8f);
                                    g_audioManager.PlaySFX(smallNames[s_smallIdx], vol);
                                    s_smallIdx = (s_smallIdx + 1) % 2;
                                }
                            }
                        }

                        // ── Water landing SFX — car fell from air into a water region ──
                        for (int wi = 0; wi < g_numWaterRegions; wi++) {
                            WaterRegion& wrLand = g_waterRegions[wi];
                            if (!wrLand.active) continue;
                            if (rearPos.x >= wrLand.x && rearPos.x <= wrLand.right()
                                && rearPos.y <= wrLand.restY + 1.0f) {
                                float splashVol = Xbox360Min(0.6f + impactSpeed * 0.06f, 1.0f);
                                g_audioManager.PlaySFX("splash", splashVol);
                                SplashWater(rearPos.x, Xbox360Min(impactSpeed * 0.15f, 1.2f));
                                break;
                            }
                        }
                    }
                }

                // ── Landing impact — front wheel ─────────────────────────
                // Lighter impact SFX for the front wheel touching down after
                // the rear has already landed, preventing double-loud hits.
                static bool s_frontWasInAir = false;
                if (!frontOnGround) {
                    s_frontWasInAir = true;
                } else if (s_frontWasInAir && frontOnGround && !g_isAirplane) {
                    s_frontWasInAir = false;
                    float frontImpact = fabsf(carVel.y);
                    if (frontImpact > 3.0f && !g_isBoatLevel) {
                        // Check front wheel is not landing in water
                        bool frontInWaterRegion = false;
                        for (int wi = 0; wi < g_numWaterRegions; wi++) {
                            WaterRegion& wrF = g_waterRegions[wi];
                            if (!wrF.active) continue;
                            if (frontPos.x >= wrF.x && frontPos.x <= wrF.right()
                                && frontPos.y <= wrF.restY + 1.0f) {
                                frontInWaterRegion = true;
                                break;
                            }
                        }
                        if (!frontInWaterRegion) {
                            static int s_frontSmallIdx = 0;
                            static int s_frontBigIdx   = 0;
                            // Front wheel plays at reduced volume (rear is primary)
                            if (frontImpact >= 8.0f) {
                                const char* bigNames[] = {
                                    "big_impact_1","big_impact_2",
                                    "big_impact_3","big_impact_4"
                                };
                                float vol = Xbox360Min(0.35f + frontImpact * 0.03f, 0.75f);
                                g_audioManager.PlaySFX(bigNames[s_frontBigIdx], vol);
                                s_frontBigIdx = (s_frontBigIdx + 1) % 4;
                            } else {
                                const char* smallNames[] = {
                                    "small_impact_1","small_impact_2"
                                };
                                float vol = Xbox360Min(0.2f + frontImpact * 0.04f, 0.6f);
                                g_audioManager.PlaySFX(smallNames[s_frontSmallIdx], vol);
                                s_frontSmallIdx = (s_frontSmallIdx + 1) % 2;
                            }
                        }
                    }
                }
            }
            UpdateParticles(g_Time.fElapsedTime);

            // ✅ Engine SFX: start persistent looping voice when gas pressed,
            //    stop it when gas released. Only creates the voice once per press.
            // ✅ Engine Idle SFX: plays when car is not accelerating or reversing.
            // ✅ Engine Reverse SFX: plays when brake/reverse trigger is held.
            // ✅ Horn SFX: fires on X button press.
            {
                float fGasCheck     = state.Gamepad.bLeftTrigger  / 255.0f;
                float fReverseCheck = state.Gamepad.bRightTrigger / 255.0f;
                bool  gasHeld       = (fGasCheck     > 0.1f);
                bool  reverseHeld   = (fReverseCheck > 0.1f);

                // ── Engine drive SFX (gas held) ──────────────────────────────
                if (gasHeld && !g_enginePlaying) {
                    // Stop idle/reverse if they were playing
                    if (g_idlePlaying) {
                        g_audioManager.StopAndDestroyVoice(g_pIdleVoice);
                        g_pIdleVoice  = NULL;
                        g_idlePlaying = false;
                    }
                    if (g_reversePlaying) {
                        g_audioManager.StopAndDestroyVoice(g_pReverseVoice);
                        g_pReverseVoice  = NULL;
                        g_reversePlaying = false;
                    }
                    g_pEngineVoice = g_audioManager.PlaySFXLooped("engine", 0.6f + fGasCheck * 0.4f);
                    g_enginePlaying = (g_pEngineVoice != NULL);
                } else if (!gasHeld && g_enginePlaying) {
                    g_audioManager.StopAndDestroyVoice(g_pEngineVoice);
                    g_pEngineVoice  = NULL;
                    g_enginePlaying = false;
                }

                // ── Engine reverse SFX (brake/reverse held, no gas) ──────────
                if (reverseHeld && !gasHeld && !g_reversePlaying) {
                    // Stop idle if it was playing
                    if (g_idlePlaying) {
                        g_audioManager.StopAndDestroyVoice(g_pIdleVoice);
                        g_pIdleVoice  = NULL;
                        g_idlePlaying = false;
                    }
                    g_pReverseVoice = g_audioManager.PlaySFXLooped("reverse", 0.5f + fReverseCheck * 0.3f);
                    g_reversePlaying = (g_pReverseVoice != NULL);
                } else if ((!reverseHeld || gasHeld) && g_reversePlaying) {
                    g_audioManager.StopAndDestroyVoice(g_pReverseVoice);
                    g_pReverseVoice  = NULL;
                    g_reversePlaying = false;
                }

                // ── Engine idle SFX (no gas, no reverse) ─────────────────────
                if (!gasHeld && !reverseHeld && !g_idlePlaying) {
                    g_pIdleVoice = g_audioManager.PlaySFXLooped("idle", 0.35f);
                    g_idlePlaying = (g_pIdleVoice != NULL);
                } else if ((gasHeld || reverseHeld) && g_idlePlaying) {
                    g_audioManager.StopAndDestroyVoice(g_pIdleVoice);
                    g_pIdleVoice  = NULL;
                    g_idlePlaying = false;
                }

                // ── Horn SFX (X button — fire-and-forget) ────────────────────
                {
                    static bool s_prevXButton = false;
                    bool currXButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
                    if (currXButton && !s_prevXButton) {
                        g_audioManager.PlaySFX("horn", 0.85f);
                    }
                    s_prevXButton = currXButton;
                }
            }
            
            // ✅ UPDATE HUD VALUES (based on car velocity)
            if (g_carChassis) {
                b2Vec2 velocity = g_carChassis->GetLinearVelocity();
                float speed = velocity.Length();
                
                // Normalize speed to 0-1 range (0 to 60 km/h max for RPM meter)
                g_hudSpeedometer = (speed > 60.0f) ? 1.0f : (speed / 60.0f);
                if (g_hudSpeedometer < 0.0f) g_hudSpeedometer = 0.0f;
                
                // Boost meter increases with acceleration
                static float prevSpeed = velocity.Length();
                float acceleration = prevSpeed;
                g_hudBoost = (acceleration > 20.0f) ? 1.0f : (acceleration / 20.0f); //acceleration / 20.0f;
                if (g_hudBoost > 1.0f) g_hudBoost = 1.0f;
                if (g_hudBoost < 0.0f) g_hudBoost = 0.0f;
                prevSpeed = speed;
            }

            if (g_carChassis && g_wheelRear && g_wheelFront) {
                b2Vec2 chassisPos = g_carChassis->GetPosition();
                g_fScrollPos = chassisPos.x;
                
                if (g_fScrollPos >= FINISH_LINE_X && !g_passedFinishLine) {
                    g_passedFinishLine = true;
                    g_audioManager.PlaySFX("levelup");

                    if (g_gameState == STATE_PLAYING_LEVEL1) {
                        g_gameState = STATE_PLAYING_LEVEL2;
                        LoadLevel(LEVEL_2);
                        g_audioManager.PlayMusic("level2");
                    }
                    else if (g_gameState == STATE_PLAYING_LEVEL2) {
                        g_gameState = STATE_PLAYING_LEVEL3;
                        LoadLevel(LEVEL_3);
                        g_audioManager.PlayMusic("level3");
                    }
                    else if (g_gameState == STATE_PLAYING_LEVEL3) {
                        g_gameState = STATE_PLAYING_LEVEL4;
                        LoadLevel(LEVEL_4);
                        g_audioManager.PlayMusic("level4");
                    }
                    else if (g_gameState == STATE_PLAYING_LEVEL4) {
                        g_gameState = STATE_PLAYING_LEVEL5;
                        LoadLevel(LEVEL_5);
                        g_audioManager.PlayMusic("level5");
                    }
                    else if (g_gameState == STATE_PLAYING_LEVEL5) {
                        g_gameState = STATE_PLAYING_LEVEL6;
                        LoadLevel(LEVEL_6);
                        g_audioManager.PlayMusic("level6");
                    }
                    else if (g_gameState == STATE_PLAYING_LEVEL6) {
                        g_gameState    = STATE_PLAYING_LEVEL7;
                        g_selectedCar  = CAR_BOAT;
                        SetCarParameters(CAR_BOAT);
                        ApplyCarSFX(CAR_BOAT);
                        LoadLevel(LEVEL_7);
                        g_audioManager.PlayMusic("level7");
                    }
                    else if (g_gameState == STATE_PLAYING_LEVEL7) {
                        g_lapCount++;
                        g_selectedCar  = CAR_BOAT;
                        SetCarParameters(CAR_BOAT);
                        ApplyCarSFX(CAR_BOAT);
                        LoadLevel(LEVEL_7);
                        g_audioManager.PlayMusic("level7");
                    }
                }
                
                if (g_fScrollPos < (FINISH_LINE_X - 10.0f)) {
                    g_passedFinishLine = false;
                }
            }
        }
    }

    // ✅ DYNAMIC CAMERA — tracks car Y height, zooms with speed
    //    Only active during gameplay (isPlaying guard keeps menus using static cam)
    if (g_carChassis) {
        b2Vec2 vel = g_carChassis->GetLinearVelocity();
        float  speed = vel.Length();
        b2Vec2 pos   = g_carChassis->GetPosition();

        // --- Y tracking: camera centre sits ABOVE the car ---
        // +2.0 offset: car appears at ~30% from screen bottom, sky fills top 70%.
        // Matches HCR layout: terrain/car at bottom, open sky above.
        // Negative offset (below car) was wrong - it pushed terrain to screen top.
        float targetCamY   = pos.y + 2.0f;
        float camLerpRate  = 4.5f * g_Time.fElapsedTime;
        if (camLerpRate > 1.0f) camLerpRate = 1.0f;
        g_camY += (targetCamY - g_camY) * camLerpRate;

        // --- FOV zoom: wider when fast, tighter when slow/idle ---
        // Speed normalised 0-1 over a 25-unit range. FOV range: 42 deg (slow) to 56 deg (fast)
        float speedNorm   = speed / 25.0f;
        if (speedNorm > 1.0f) speedNorm = 1.0f;
        float targetFOV   = (D3DX_PI / 4) * (1.0f + speedNorm * 0.28f); // 45->57.6 deg
        float fovLerpRate = 3.0f * g_Time.fElapsedTime;
        if (fovLerpRate > 1.0f) fovLerpRate = 1.0f;
        g_camFOV += (targetFOV - g_camFOV) * fovLerpRate;

        // Rebuild view matrix: eye and lookat shift vertically with g_camY,
        // no rotation (X locked to 0, Z fixed at -15)
        float eyeY    = g_camY + 1.5f;
        float lookatY = g_camY - 0.5f;
        D3DXVECTOR3 vEye(0.0f, eyeY, -15.0f);
        D3DXVECTOR3 vAt (0.0f, lookatY, 0.0f);
        D3DXVECTOR3 vUp (0.0f, 1.0f, 0.0f);
        D3DXMatrixLookAtLH(&g_matView, &vEye, &vAt, &vUp);
        D3DXMatrixPerspectiveFovLH(&g_matProj, g_camFOV, 16.0f/9.0f, 1.0f, 500.0f);
    }

    TEXVERTEX* pV;
    if(SUCCEEDED(g_pVB->Lock(0,0,(void**)&pV,0))) {
        for(int i=0; i<NUM_DISPLAY_SEGMENTS; i++) {
            float displayX = (i - NUM_DISPLAY_SEGMENTS/2) * SEGMENT_LENGTH;
            float worldX = g_fScrollPos + displayX;
            
            float wrappedX = worldX;
            while (wrappedX < 0.0f) wrappedX += TERRAIN_WRAP_LENGTH;
            while (wrappedX >= TERRAIN_WRAP_LENGTH) wrappedX -= TERRAIN_WRAP_LENGTH;
            
            int segIndex = (int)(wrappedX / SEGMENT_LENGTH);
            
            float terrainY = 0.0f;
            if (segIndex >= 0 && segIndex < (int)g_terrainSegments.size()) {
                const TerrainSegment& seg = g_terrainSegments[segIndex];
                
                float segLocalX = wrappedX - seg.x1;
                float t = segLocalX / (seg.x2 - seg.x1);
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                
                terrainY = seg.y1 + t * (seg.y2 - seg.y1);
            }
            
            float scaledTerrainY = terrainY * TERRAIN_VISUAL_SCALE_Y;
            float scaledDisplayX = displayX * TERRAIN_VISUAL_SCALE_X;
            
            pV[i*2].x = scaledDisplayX;
            pV[i*2].y = scaledTerrainY;
            pV[i*2].z = 0;
            pV[i*2].u = wrappedX * 0.2f;
            pV[i*2].v = 0;
            
            pV[i*2+1].x = scaledDisplayX;
            pV[i*2+1].y = (float)TERRAIN_FLOOR;
            pV[i*2+1].z = 0;
            pV[i*2+1].u = wrappedX * 0.2f;
            pV[i*2+1].v = 1.0f;
        }
        g_pVB->Unlock();
    }
}

//============================================================================================
// MAIN
//============================================================================================

void __cdecl main() {
    // ✅ Pin main thread to Core 0, Hardware Thread 0.
    //    Without explicit affinity on Xbox 360 all threads stay on the same
    //    hardware thread as the parent - competing with each other for CPU time.
    //    Core layout:
    //      XboxThread0 (Core 0, HT0) - main game loop / render      <-- us
    //      XboxThread1 (Core 0, HT1) - available for future use
    //      XboxThread2 (Core 1, HT0) - Box2D / workers
    //      XboxThread3 (Core 1, HT1) - available
    //      XboxThread4 (Core 2, HT0) - available
    //      XboxThread5 (Core 2, HT1) - XAudio2 engine (set in audio.h)
    XSetThreadProcessor(GetCurrentThread(), 0);

    // ✅ Load config first so all tunable values are set before D3D/physics init
    LoadConfig();

    if (FAILED(InitD3D())) {
        return;
    }
    
    if (FAILED(InitScene())) {
        return;
    }
    
    InitTime();
    int frameCount = 0;
    for (;;) {
        UpdateTime();
        Update();
        Render();
        frameCount++;
    }
    
    // Clean up engine voice before audio shutdown
    if (g_pEngineVoice) {
        g_audioManager.StopAndDestroyVoice(g_pEngineVoice);
        g_pEngineVoice  = NULL;
        g_enginePlaying = false;
    }
    if (g_pIdleVoice) {
        g_audioManager.StopAndDestroyVoice(g_pIdleVoice);
        g_pIdleVoice  = NULL;
        g_idlePlaying = false;
    }
    if (g_pReverseVoice) {
        g_audioManager.StopAndDestroyVoice(g_pReverseVoice);
        g_pReverseVoice  = NULL;
        g_reversePlaying = false;
    }
    g_audioManager.Shutdown();
}

//============================================================================================
// END OF XBOX 360 HILL CLIMB RACING - 16 CARS WITH AUDIO
//============================================================================================
