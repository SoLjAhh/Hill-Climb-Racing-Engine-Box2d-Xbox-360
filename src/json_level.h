#pragma once
#include "stdafx.h"
#include "globals.h"

//============================================================================================
// JSON LEVEL LOADER
// Loads custom levels from .json files in game:\Config\Levels\
// Supports two formats:
//   1. HCR native JSON  — "level_format": "hcr_level_v1"
//   2. R.U.B.E. (Really Useful Box2D Editor) JSON — detected by "gravity" object + "body" array
//
// JSON levels are appended to the custom level list AFTER settings.cfg [level] entries.
// They populate the same CustomLevelDef slots and appear in level select identically.
//============================================================================================

//============================================================================================
// EXTENDED LEVEL DATA (per-JSON-level, parallel to g_customLevels[])
// Stores fields that CustomLevelDef doesn't cover: jumps, water, gravity, spawn.
// Indexed by custom level index (same as g_customLevels).
//============================================================================================
struct JsonLevelExtra {
    bool  hasData;            // true if this slot was loaded from JSON

    // ── Gravity ──
    float gravityY;           // custom gravity (default = GRAVITY = -9.81)

    // ── Spawn point ──
    float spawnX, spawnY;     // vehicle start position

    // ── Jump features ──
    int      numJumps;
    JumpDef  jumps[MAX_JUMPS_PER_LEVEL];

    // ── Water regions ──
    int   numWater;
    float waterX    [MAX_WATER_REGIONS];
    float waterWidth[MAX_WATER_REGIONS];
    float waterDepth[MAX_WATER_REGIONS];

    // ── Finish line (optional override) ──
    float finishLineX;        // 0 = use global FINISH_LINE_X
};

#define MAX_JSON_LEVELS MAX_CUSTOM_LEVELS
extern JsonLevelExtra g_jsonLevelExtra[MAX_JSON_LEVELS];

//============================================================================================
// PUBLIC API
//============================================================================================

// Scan game:\Config\Levels\ for .json files and append them to the custom level list.
// Call AFTER LoadConfig() so cfg-defined levels are already in place.
// Returns the number of JSON levels loaded.
int  LoadJsonLevels();

// Apply JSON extra data (jumps, water, gravity, spawn) for a custom level.
// Called from LoadLevel() when the level is a JSON-sourced custom level.
// Returns true if JSON extra data was found and applied.
bool ApplyJsonLevelData(int customIndex);

//============================================================================================
// R.U.B.E. SCENE FILE DISCOVERY
// Raw R.U.B.E. scenes (like truck.json) that have no hcr_ markers are NOT
// loaded as custom levels.  Instead, their paths are recorded here so the
// level select screen can offer them as "Sandbox" entries.
// To play one, call RubeLoadScene(g_rubeScenePaths[index]).
//============================================================================================
#define MAX_RUBE_SCENES 64
extern char g_rubeScenePaths[MAX_RUBE_SCENES][256];
extern char g_rubeSceneNames[MAX_RUBE_SCENES][64]; // display name (filename without .json)
extern LPDIRECT3DTEXTURE9 g_rubeSceneIcons[MAX_RUBE_SCENES]; // icon textures (NULL = use default)
extern int  g_numRubeScenes;

// Load icon textures for R.U.B.E. scenes.
// Looks for <scenename>_icon.png next to the .json/.rube file.
// Must be called AFTER D3D device is created (e.g. from InitScene).
void LoadRubeSceneIcons();

//============================================================================================
// MINIMAL JSON PARSER (C++03 / Xbox 360 safe — no STL, no exceptions, no RTTI)
// Operates on a flat char buffer. Provides cursor-based navigation:
//   - FindKey()    → position cursor at a key's value
//   - ReadFloat()  → parse float at cursor
//   - ReadString() → copy string at cursor into buffer
//   - ReadBool()   → parse true/false at cursor
//   - EnterArray() → begin iterating array elements
//   - NextElement()→ advance to next array element
//   - EnterObject()→ descend into nested object
//============================================================================================

struct JsonReader {
    const char* buf;      // full JSON text
    int         len;      // buffer length
    int         pos;      // current cursor position

    // Initialise with a null-terminated JSON string
    void Init(const char* jsonText, int jsonLen);

    // Navigation
    bool FindKey(const char* key);              // find "key": and position after colon
    bool FindKeyIn(const char* key, int start, int end); // scoped search
    void SkipWhitespace();
    int  SkipValue();                           // skip one JSON value, return end pos
    int  FindMatchingBrace(int from);           // find matching } or ]

    // Reading (at current pos)
    float ReadFloat();
    bool  ReadString(char* dst, int dstSize);   // copies into dst, returns success
    bool  ReadBool();
    int   ReadInt();

    // Array iteration
    bool EnterArray();                          // pos must be at '[', advances past it
    bool NextElement();                         // advance to next element or return false at ']'
    int  ArrayCount();                          // count elements (non-destructive peek)

    // Object
    bool EnterObject();                         // pos must be at '{', advances past it

    // Scope helpers — save/restore cursor
    int  SavePos()           { return pos; }
    void RestorePos(int p)   { pos = p;    }
};
