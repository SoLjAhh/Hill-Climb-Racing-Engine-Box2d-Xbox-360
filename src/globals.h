#pragma once
#include "stdafx.h"

// ============================================================
// FORWARD DECLARATIONS
// ============================================================
struct AudioClip;
class  AudioManager;
struct CustomCarDef;
struct CustomLevelDef;

// ============================================================
// COMPILE-TIME CONSTANTS
// ============================================================
const float Y1 = 5.0f;  // Used in car wheel position calculations

#define MAX_CUSTOM_CARS   1200
#define MAX_CUSTOM_LEVELS 1200
#define CAR_CUSTOM_BASE   24  // boat=22, airplane=23, custom cars 24+
#define LEVEL_CUSTOM_BASE 7

// ============================================================
// ENUMERATIONS
// ============================================================
enum CarType {
	// Base Car Slots
    CAR_JEEP, CAR_RALLY, CAR_MONSTER, CAR_QUAD_BIKE, CAR_RACE_CAR,
    CAR_FAMILY, CAR_OLD_SCHOOL, CAR_PARAMEDIC, CAR_BUGGY, CAR_SPRINTER,
    CAR_POLICE, CAR_STUNT_RIDER, CAR_SUPER_DIESEL, CAR_SUPER_JEEP,
    CAR_TROPHY_TRUCK, CAR_CHEVELLE, CAR_BLAZER, CAR_CAPRICE,
    CAR_CYBERTRUCK, CAR_KNIGHTRIDER, CAR_SCHOOLBUS, CAR_USEDCAR,
    CAR_BOAT, CAR_AIRPLANE,
	// Custom Car Slots (indices CAR_CUSTOM_BASE .. CAR_CUSTOM_BASE + MAX_CUSTOM_CARS - 1)
	// No individual entries needed — all code uses CAR_CUSTOM_BASE + index.
	// The enum value of CAR_CUSTOM_1 marks the start of the custom range.
	CAR_CUSTOM_1
	// Valid custom car enum values: CAR_CUSTOM_1 .. CAR_CUSTOM_1 + MAX_CUSTOM_CARS - 1
};

enum GameState {
    STATE_TITLE_SCREEN, STATE_MAIN_MENU, STATE_CAR_SELECT,
    STATE_LEVEL_SELECT, STATE_SETTINGS, STATE_ABOUT,
    STATE_PLAYING_LEVEL1, STATE_PLAYING_LEVEL2, STATE_PLAYING_LEVEL3,
    STATE_PLAYING_LEVEL4, STATE_PLAYING_LEVEL5, STATE_PLAYING_LEVEL6,
    STATE_PLAYING_LEVEL7,
    STATE_PAUSE_MENU
};

enum Level {
	// Base Level Slots
    LEVEL_1, LEVEL_2, LEVEL_3, LEVEL_4, LEVEL_5, LEVEL_6, LEVEL_7,
	// Custom Level Slots (indices LEVEL_CUSTOM_BASE .. LEVEL_CUSTOM_BASE + MAX_CUSTOM_LEVELS - 1)
	// No individual entries needed — all code uses LEVEL_CUSTOM_1 + index.
	// The enum value of LEVEL_CUSTOM_1 marks the start of the custom range.
    LEVEL_CUSTOM_1
	// Valid custom level enum values: LEVEL_CUSTOM_1 .. LEVEL_CUSTOM_1 + MAX_CUSTOM_LEVELS - 1
};

// ============================================================
// STRUCTS
// ============================================================
struct TerrainSegment { float x1, y1, x2, y2; };
struct TEXVERTEX      { float x, y, z, u, v;  };

struct CustomCarDef {
    bool  loaded;
    char  name[64];
    char  bodyTexPath[128];
    char  wheelTexPath[128];
    char  iconTexPath[128];
    float carWidth, carHeight, wheelRadius;
    float wheelJointFreq, wheelJointDamp;
    float motorMaxSpeed, motorTorqueRear, motorTorqueFront;
    float rideHeightOffset, wheelRearOffsetX, wheelFrontOffsetX;
    LPDIRECT3DTEXTURE9 pTexBody;
    LPDIRECT3DTEXTURE9 pTexWheel;
    LPDIRECT3DTEXTURE9 pTexIcon;
};

struct CustomLevelDef {
    bool  loaded;
    char  name[64];
    char  terrainTexPath[128];
    char  bgTexPath[128];
    char  bg2TexPath[128];
    char  bg3TexPath[128];
    char  iconTexPath[128];
    float amplitude, frequencyBase;
    float hasBridge, bridgeStartX;
    LPDIRECT3DTEXTURE9 pTexTerrain;
    LPDIRECT3DTEXTURE9 pTexBg;
    LPDIRECT3DTEXTURE9 pTexBg2;
    LPDIRECT3DTEXTURE9 pTexBg3;
    LPDIRECT3DTEXTURE9 pTexIcon;
};

struct TimeStruct {
    LARGE_INTEGER qwTime;
    float fSecsPerTick;
    float fElapsedTime;
};

// ============================================================
// BRIDGE SETTINGS (mutable at runtime via settings.cfg)
// ============================================================
extern float BARRIER_WALL_X;
extern float FINISH_LINE_X;
extern float BRIDGE_SPAN;
extern float BRIDGE_PLANK_HEIGHT;
extern float BRIDGE_PLANK_DENSITY;
extern float BRIDGE_PLANK_FRICTION;
extern float BRIDGE_PLANK_RESTITUTION;
extern float BRIDGE_ANCHOR_HEIGHT;
extern float BRIDGE_L2_START_X;
extern float BRIDGE_L4_START_X;
extern float BRIDGE_VALLEY_DEPTH;
extern float BRIDGE_APPROACH_LENGTH;
extern float BRIDGE_VALLEY_SLOPE;
extern float BRIDGE_ENTRY_NUDGE;
extern float BRIDGE_CATENARY_SAG;
extern float BRIDGE_VISUAL_WIDTH_SCALE;
extern float BRIDGE_VISUAL_HEIGHT_SCALE;
extern float BRIDGE_VISUAL_Y_OFFSET;

// ── Water valley carve constants ─────────────────────────────────────────
// Water sections use the same smoothstep carve as bridges, but shallower.
// WATER_VALLEY_DEPTH controls how deep the pool is carved into terrain.
// WATER_SPAN is the total width of the carved section.
// Per-level start X values are registered in SetLevelWater via g_waterRegions.
const float WATER_VALLEY_DEPTH = 2.0f;   // how deep the water pool sits
const float WATER_VALLEY_WALL  = 0.28f;  // wider slope = smoother car entry, less flip
const float WATER_SPAN         = 18.0f;  // default pool width (world units)

// Water texture
extern LPDIRECT3DTEXTURE9 g_pTexWater;
extern LPDIRECT3DTEXTURE9 g_pTexWaterDrop;  // water drop particle texture
extern LPDIRECT3DTEXTURE9 g_pTexBoat;
extern LPDIRECT3DTEXTURE9 g_pTexAirplane;       // yacht texture for Level 7
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Boat;     // boat car select icon
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Airplane;  // airplane car select icon

// ============================================================
// SCREEN CONFIGURATION - defined here directly (const = internal linkage in C++)
// ============================================================
const float TITLE_SCREEN_SCALE_X  = 1.0f;
const float TITLE_SCREEN_SCALE_Y  = 1.0f;
const float TITLE_SCREEN_OFFSET_X = 0.0f;
const float TITLE_SCREEN_OFFSET_Y = 0.0f;
const float BACKGROUND_SCALE_X    = 1.0f;
const float BACKGROUND_SCALE_Y    = 1.2f;
const float BACKGROUND_OFFSET_Y   = 3.0f;
const float TERRAIN_VISUAL_SCALE_X = 1.0f;
const float TERRAIN_VISUAL_SCALE_Y = 1.0f;
const float BACKGROUND_PARALLAX_FACTOR  = 0.2f;   // Layer 1 - far sky (slowest)
const float BACKGROUND2_PARALLAX_FACTOR = 0.45f;  // Layer 2 - mid hills
const float BACKGROUND3_PARALLAX_FACTOR = 0.72f;  // Layer 3 - near foreground (fastest)
const float BACKGROUND_TILE_SIZE        = 100.0f;
const float BACKGROUND2_SCALE_X        = 1.0f;
const float BACKGROUND2_SCALE_Y        = 0.8f;
const float BACKGROUND2_OFFSET_Y       = -1.5f;
const float SEGMENT_LENGTH    = 0.1f;
const float SLOPE_LIMIT       = 0.2f;
const float TERRAIN_FLOOR     = -12.0f;
const float TERRAIN_WRAP_LENGTH = 500.0f;
const float GRAVITY           = -9.81f;
const float CAR_START_X       = 12.0f;
const float CAR_START_Y       = 1.5f * 5.0f + 1.0f;  // 1.5*Y1+1.0, Y1=5.0
const int   NUM_DISPLAY_SEGMENTS = 300;
const int   CIRCLE_SEGMENTS     = 32;
const int   BRIDGE_PLANK_COUNT  = 40;  // 40 planks = very smooth catenary curve

// ============================================================
// WATER SYSTEM — spring-column heightfield
// ============================================================
#define MAX_WATER_REGIONS  4
#define WATER_COLS_MAX    128   // max columns per region (width/WATER_COL_WIDTH)
#define WATER_COL_WIDTH   0.35f // world units per column — controls wave detail

// One spring column in the water surface heightfield
struct WaterCol {
    float h;    // current height offset from rest surface (positive = crest)
    float vel;  // vertical velocity of this column
};

struct WaterRegion {
    float    x;               // left edge world X
    float    restY;           // rest (equilibrium) surface Y
    float    width;           // horizontal extent
    float    depth;           // depth below restY
    bool     active;
    b2Body*  sensorBody;      // cached sensor body pointer — set by CreateWaterRegions

    WaterCol cols[WATER_COLS_MAX];
    int      numCols;

    // Derived
    float right()  const { return x + width; }
    float bottom() const { return restY - depth; }
    float surfaceY(int ci) const { return restY + cols[ci].h; }
};

extern WaterRegion g_waterRegions[MAX_WATER_REGIONS];
extern int         g_numWaterRegions;
extern bool        g_isBoatLevel;    // true on Level 7 — enables float physics
extern bool        g_isAirplane;     // true when airplane selected — enables aero physics
extern float       g_propSpin;        // accumulated prop rotation angle (radians)

// ============================================================
// JUMP FEATURES
// ============================================================
struct JumpDef {
    float x;       // world X centre of the jump
    float height;  // +ve = hump (launch), -ve = dip (valley/pit)
    float width;   // half-width in world units (controls how sharp the feature is)
};

// Max jumps per level — fixed array, no STL
#define MAX_JUMPS_PER_LEVEL 8

extern JumpDef g_levelJumps[MAX_JUMPS_PER_LEVEL];
extern int     g_numLevelJumps;

// ============================================================
// TERRAIN SETTINGS (mutable)
// ============================================================
extern float TERRAIN_AMPLITUDE;
extern float TERRAIN_FREQUENCY_BASE;
extern float TERRAIN_AMPLITUDE_L2;
extern float TERRAIN_FREQUENCY_BASE_L2;
extern float TERRAIN_AMPLITUDE_L3;
extern float TERRAIN_FREQUENCY_BASE_L3;
extern float TERRAIN_AMPLITUDE_L4;
extern float TERRAIN_FREQUENCY_BASE_L4;
extern float TERRAIN_AMPLITUDE_L5;
extern float TERRAIN_FREQUENCY_BASE_L5;
extern float TERRAIN_AMPLITUDE_L6;
extern float TERRAIN_FREQUENCY_BASE_L6;

// ============================================================
// BOX2D GLOBALS
// ============================================================
extern b2World*      g_pBox2DWorld;
extern b2Body*       g_carChassis;
extern b2Body*       g_wheelRear;
extern b2Body*       g_wheelFront;
extern b2Body*       g_groundBody;
extern b2WheelJoint* g_rearSuspension;
extern b2WheelJoint* g_frontSuspension;

// ============================================================
// GAME STATE
// ============================================================
extern GameState g_gameState;
extern CarType   g_selectedCar;
extern int       g_carSelectIndex;
extern Level     g_currentLevel;
extern std::vector<TerrainSegment> g_terrainSegments;
extern float g_terrainStartY;
extern float g_terrainEndY;
extern float g_fScrollPos;
extern float g_motorSpeed;
extern float g_camY;
extern float g_camFOV;
extern bool  g_passedFinishLine;
extern int   g_lapCount;
extern bool  g_prevDpadLeft;
extern bool  g_prevDpadRight;
extern bool  g_prevAButton;
extern bool  g_prevStartButton;
extern int   g_pauseMenuIndex;
extern int   g_mainMenuIndex;
extern int   g_levelSelectIndex;

// ============================================================
// CAR PHYSICS WORKING VARIABLES
// ============================================================
extern float g_visualCarWidth;
extern float g_visualCarHeight;
extern float g_visualWheelRadius;
extern float g_wheelJointFrequency;
extern float g_wheelJointDamping;
extern float g_motorMaxSpeed;
extern float g_motorTorqueRear;
extern float g_motorTorqueFront;
extern float g_rideHeightOffset;
extern float g_wheelRearOffsetX;
extern float g_wheelRearStartY;
extern float g_wheelFrontOffsetX;
extern float g_wheelFrontStartY;

// ============================================================
// D3D GLOBALS
// ============================================================
extern LPDIRECT3DDEVICE9            g_pd3dDevice;
extern LPDIRECT3DVERTEXBUFFER9      g_pVB;
extern LPDIRECT3DVERTEXBUFFER9      g_pSplashVB_Title;
extern LPDIRECT3DVERTEXBUFFER9      g_pSplashVB_CarSelect;
extern LPDIRECT3DVERTEXDECLARATION9 g_pVertexDecl;
extern LPDIRECT3DVERTEXSHADER9      g_pVertexShader;
extern LPDIRECT3DPIXELSHADER9       g_pPixelShader;
extern LPDIRECT3DPIXELSHADER9       g_pMenuPixelShader;
extern LPDIRECT3DPIXELSHADER9       g_pParticlePixelShader;
extern LPDIRECT3DPIXELSHADER9       g_pSolidColourShader;

// ── Particle textures ─────────────────────────────────────────────────────
extern LPDIRECT3DTEXTURE9  g_pTexParticleDirt;
extern LPDIRECT3DTEXTURE9  g_pTexParticleSmoke;
extern LPDIRECT3DTEXTURE9  g_pTexParticleSnow;
extern LPDIRECT3DTEXTURE9  g_pTexParticleRocks;
extern D3DXMATRIX g_matView;
extern D3DXMATRIX g_matProj;

// ============================================================
// TEXTURE GLOBALS
// ============================================================
extern LPDIRECT3DTEXTURE9 g_pTexTerrain;
extern LPDIRECT3DTEXTURE9 g_pTexTerrain_L2;
extern LPDIRECT3DTEXTURE9 g_pTexTerrain_L3;
extern LPDIRECT3DTEXTURE9 g_pTexTerrain_L4;
extern LPDIRECT3DTEXTURE9 g_pTexTerrain_L5;
extern LPDIRECT3DTEXTURE9 g_pTexTerrain_L6;
extern LPDIRECT3DTEXTURE9 g_pTexTerrain_L7;    // water level seabed
extern LPDIRECT3DTEXTURE9 g_pTexBackground;
extern LPDIRECT3DTEXTURE9 g_pTexBackground_L2;
extern LPDIRECT3DTEXTURE9 g_pTexBackground_L3;
extern LPDIRECT3DTEXTURE9 g_pTexBackground_L4;
extern LPDIRECT3DTEXTURE9 g_pTexBackground_L5;
extern LPDIRECT3DTEXTURE9 g_pTexBackground_L6;
extern LPDIRECT3DTEXTURE9 g_pTexBackground_L7;  // water level sky
extern LPDIRECT3DTEXTURE9 g_pTexBackground2;
extern LPDIRECT3DTEXTURE9 g_pTexBackground2_L2;
extern LPDIRECT3DTEXTURE9 g_pTexBackground2_L3;
extern LPDIRECT3DTEXTURE9 g_pTexBackground2_L4;
extern LPDIRECT3DTEXTURE9 g_pTexBackground2_L5;
extern LPDIRECT3DTEXTURE9 g_pTexBackground2_L6;
extern LPDIRECT3DTEXTURE9 g_pTexBackground2_L7; // water level mid
extern LPDIRECT3DTEXTURE9 g_pTexBackground3;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L2;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L3;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L4;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L5;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L6;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L7; // water level fg
extern LPDIRECT3DTEXTURE9 g_pTexBackground3;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L2;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L3;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L4;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L5;
extern LPDIRECT3DTEXTURE9 g_pTexBackground3_L6;
extern LPDIRECT3DTEXTURE9 g_pTexBridgePlank;
extern LPDIRECT3DTEXTURE9 g_pTexSnowflake;
extern LPDIRECT3DTEXTURE9 g_pTexCar;
extern LPDIRECT3DTEXTURE9 g_pTexCarRally;
extern LPDIRECT3DTEXTURE9 g_pTexCarMonster;
extern LPDIRECT3DTEXTURE9 g_pTexCarQuadBike;
extern LPDIRECT3DTEXTURE9 g_pTexCarRacecar;
extern LPDIRECT3DTEXTURE9 g_pTexCarFamily;
extern LPDIRECT3DTEXTURE9 g_pTexCarOldSchool;
extern LPDIRECT3DTEXTURE9 g_pTexCarParamedic;
extern LPDIRECT3DTEXTURE9 g_pTexCarBuggy;
extern LPDIRECT3DTEXTURE9 g_pTexCarSprinter;
extern LPDIRECT3DTEXTURE9 g_pTexCarPolice;
extern LPDIRECT3DTEXTURE9 g_pTexCarStuntRider;
extern LPDIRECT3DTEXTURE9 g_pTexCarSuperDiesel;
extern LPDIRECT3DTEXTURE9 g_pTexCarSuperJeep;
extern LPDIRECT3DTEXTURE9 g_pTexCarTrophy;
extern LPDIRECT3DTEXTURE9 g_pTexCarChevelle;
extern LPDIRECT3DTEXTURE9 g_pTexCar_Blazer;
extern LPDIRECT3DTEXTURE9 g_pTexCar_Caprice;
extern LPDIRECT3DTEXTURE9 g_pTexCar_Cybertruck;
extern LPDIRECT3DTEXTURE9 g_pTexCar_KnightRider;
extern LPDIRECT3DTEXTURE9 g_pTexCar_SchoolBus;
extern LPDIRECT3DTEXTURE9 g_pTexCar_UsedCar;
extern LPDIRECT3DTEXTURE9 g_pTexWheel;
extern LPDIRECT3DTEXTURE9 g_pTexWheelMonster;
extern LPDIRECT3DTEXTURE9 g_pTexWheelDiesel;
extern LPDIRECT3DTEXTURE9 g_pTexWheelClassic;
extern LPDIRECT3DTEXTURE9 g_pTexWheelParamedic;
extern LPDIRECT3DTEXTURE9 g_pTexWheelStunt;
extern LPDIRECT3DTEXTURE9 g_pTexWheelTrophy;
extern LPDIRECT3DTEXTURE9 g_pTexWheelTuner;
extern LPDIRECT3DTEXTURE9 g_pTexWheelBlazer;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_jeep;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Rally;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Monster;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_QuadBike;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_RaceCar;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Family;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_OldSchool;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Paramedic;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Buggy;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Sprinter;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Police;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_StuntRider;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_SuperDiesel;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_SuperJeep;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Trophy;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Chevelle;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Blazer;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Caprice;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_Cybertruck;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_KnightRider;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_SchoolBus;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectIcon_UsedCar;
extern LPDIRECT3DTEXTURE9 g_pTexTitle;
extern LPDIRECT3DTEXTURE9 g_pTexCarSelectBG;
extern LPDIRECT3DTEXTURE9 g_pTexPauseMenuBG;
extern LPDIRECT3DTEXTURE9 g_pTexPauseResumeButton;
extern LPDIRECT3DTEXTURE9 g_pTexPauseRestartButton;
extern LPDIRECT3DTEXTURE9 g_pTexPauseLevelButton;
extern LPDIRECT3DTEXTURE9 g_pTexMainMenuBG;
extern LPDIRECT3DTEXTURE9 g_pTexAboutBG;
extern LPDIRECT3DTEXTURE9 g_pTexLevelSelectBG;
extern LPDIRECT3DTEXTURE9 g_pTexLevelIcon_1;
extern LPDIRECT3DTEXTURE9 g_pTexLevelIcon_2;
extern LPDIRECT3DTEXTURE9 g_pTexLevelIcon_3;
extern LPDIRECT3DTEXTURE9 g_pTexLevelIcon_4;
extern LPDIRECT3DTEXTURE9 g_pTexLevelIcon_5;
extern LPDIRECT3DTEXTURE9 g_pTexLevelIcon_6;
extern LPDIRECT3DTEXTURE9 g_pTexLevelIcon_7;  // water level
extern LPDIRECT3DTEXTURE9 g_pTexMenuButtonCars;
extern LPDIRECT3DTEXTURE9 g_pTexMenuButtonSettings;
extern LPDIRECT3DTEXTURE9 g_pTexMenuButtonAbout;
extern LPDIRECT3DTEXTURE9 g_pTexGaugeRPM;
extern LPDIRECT3DTEXTURE9 g_pTexGaugeBoost;
extern LPDIRECT3DTEXTURE9 g_pTexGaugeNeedle;

// ============================================================
// BRIDGE PHYSICS GLOBALS
// ============================================================
extern b2Body*          g_bridgeAnchorL;
extern b2Body*          g_bridgeAnchorR;
extern b2Body*          g_bridgePlanks[40];
extern b2Joint*         g_bridgeJoints[41];  // BRIDGE_PLANK_COUNT + 1
extern bool             g_bridgeActive;

// ============================================================
// HUD
// ============================================================
extern float g_hudSpeedometer;
extern float g_hudBoost;
extern bool  g_showHUD;

// ============================================================
// SETTINGS SCREEN
// ============================================================
extern int   g_settingsIndex;    // 0 = Music slider, 1 = SFX slider
extern float g_musicVolume;      // 0.0 – 1.0
extern float g_sfxVolume;        // 0.0 – 1.0
extern bool  g_prevDpadUp;
extern bool  g_prevDpadDown;
extern LPDIRECT3DTEXTURE9 g_pTexSettingsBG;

// ============================================================
// TIMING
// ============================================================
extern TimeStruct g_Time;

// ============================================================
// AUDIO
// ============================================================
extern AudioManager          g_audioManager;
extern IXAudio2SourceVoice*  g_pEngineVoice;
extern bool                  g_enginePlaying;
extern IXAudio2SourceVoice*  g_pIdleVoice;
extern bool                  g_idlePlaying;
extern IXAudio2SourceVoice*  g_pReverseVoice;
extern bool                  g_reversePlaying;

// ============================================================
// MOD SYSTEM
// ============================================================
extern CustomCarDef   g_customCars[MAX_CUSTOM_CARS];
extern CustomLevelDef g_customLevels[MAX_CUSTOM_LEVELS];
extern char           g_customLevelMusic[MAX_CUSTOM_LEVELS][128];  // parallel music paths (separate to preserve struct size)

// ── Custom car SFX paths (parallel to g_customCars, preserves struct size) ──
struct CustomCarSfx {
    char engine[128];
    char idle[128];
    char reverse[128];
    char horn[128];
};
extern CustomCarSfx   g_customCarSfx[MAX_CUSTOM_CARS];
extern int g_numCustomCars;
extern int g_numCustomLevels;
extern int g_totalCarSlots;
extern int g_totalLevelSlots;

// ============================================================
// PARTICLE SYSTEM  (fixed pool — no heap, no STL, C++98 safe)
// ============================================================
#define MAX_PARTICLES 384   // increased for water splash droplets

enum ParticleType {
    PARTICLE_NONE   = 0,
    PARTICLE_DIRT   = 1,   // wheel dust / terrain contact
    PARTICLE_SMOKE  = 2,   // exhaust smoke
    PARTICLE_DUST   = 3,   // landing impact cloud
    PARTICLE_WATER  = 4,   // water splash droplet
    PARTICLE_WAKE   = 5    // boat wake trail
};

struct Particle {
    float       x, y;          // world position
    float       vx, vy;        // velocity (world units/sec)
    float       life;          // remaining life 0-1 (1=just spawned, 0=dead)
    float       size;          // visual size in world units
    float       alpha;         // opacity 0-1
    ParticleType type;
    bool        landedInWater; // water droplet: true after it hits water surface
};

extern Particle  g_particles[MAX_PARTICLES];
extern int       g_numParticles;

// ============================================================
// UTILITY
// ============================================================
inline float Xbox360Min(float a, float b) { return (a < b) ? a : b; }
inline float Xbox360Max(float a, float b) { return (a > b) ? a : b; }

extern XVIDEO_MODE VideoMode;

// Shader source strings (defined in main.cpp)
extern const char* g_strVertexShaderProgram;
extern const char* g_strPixelShaderProgram;
extern const char* g_strMenuPixelShaderProgram;
