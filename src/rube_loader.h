#pragma once
#include "stdafx.h"
#include "globals.h"

//============================================================================================
// R.U.B.E. SCENE LOADER (Really Useful Box2D Editor)
//
// Loads a complete Box2D world from a R.U.B.E. JSON export:
//   - All bodies (static, dynamic, kinematic) with exact position/angle/velocity
//   - All fixtures (polygon + circle) with density/friction/restitution/filters
//   - All joints (wheel + revolute) with anchors/motors/limits/springs
//
// Integration with our game:
//   - The static body with the most fixtures becomes the GROUND (replaces terrain)
//   - The dynamic body named "truckchassis" (or biggest by mass) becomes the PLAYER
//   - Wheel joints attached to the player body are driven by gamepad input
//   - Camera follows the player body
//   - All other bodies (stones, chain links, etc.) simulate freely
//
// Usage:
//   1. Call RubeLoadScene("game:\\Config\\Levels\\truck.json")
//   2. Scene creates its own b2World and populates it
//   3. Call RubeUpdate(dt) each frame for stepping + input
//   4. Call RubeGetCameraTarget() for camera position
//   5. Call RubeUnloadScene() when done
//============================================================================================

//============================================================================================
// LIMITS (fixed arrays — no STL, Xbox 360 safe)
//============================================================================================
#define RUBE_MAX_BODIES    512
#define RUBE_MAX_FIXTURES  1024
#define RUBE_MAX_JOINTS    256
#define RUBE_MAX_VERTS     8    // Box2D polygon vertex limit
#define RUBE_MAX_IMAGES    128  // R.U.B.E. scene images

//============================================================================================
// R.U.B.E. IMAGE — a textured quad attached to a body (or world origin)
//============================================================================================
struct RubeImage {
    int            bodyId;       // R.U.B.E. body ID (-1 = world)
    int            bodyIdx;      // resolved array index into bodies[]
    float          centerX, centerY;  // offset from body origin (local space)
    float          angle;        // rotation in radians relative to body
    float          scale;        // image scale (world units)
    float          opacity;      // alpha 0-1
    float          renderOrder;  // draw order (lower = behind)
    char           file[128];    // relative path to texture
    LPDIRECT3DTEXTURE9 texture;  // loaded D3D texture (NULL if not found)
    int            texW, texH;   // texture dimensions (for aspect ratio)
};

//============================================================================================
// SCENE STATE
//============================================================================================
struct RubeScene {
    bool       active;             // true while a R.U.B.E. scene is loaded
    char       loadedPath[256];    // path of the loaded JSON — used for restart

    // ── World ──
    b2World*   world;              // owned — created/destroyed by loader
    float      gravityX, gravityY;
    float      stepsPerSecond;     // simulation rate from JSON (default 60)
    int        velIterations;      // velocity iterations (default 8)
    int        posIterations;      // position iterations (default 3)

    // ── Bodies (array-index matches JSON body order for joint lookups) ──
    b2Body*    bodies[RUBE_MAX_BODIES];
    char       bodyNames[RUBE_MAX_BODIES][64];
    int        bodyTypes[RUBE_MAX_BODIES];   // 0=static, 1=kinematic, 2=dynamic
    int        bodyIds[RUBE_MAX_BODIES];     // R.U.B.E. body IDs (for native format joint refs)
    int        numBodies;

    // ── Player identification ──
    int        playerBodyIdx;      // index into bodies[] of the player chassis
    b2Body*    playerBody;         // shortcut: bodies[playerBodyIdx]

    // Bodies directly connected to the player via joints (for motor capture)
    bool       isPlayerAdjacent[RUBE_MAX_BODIES];

    // ── Motorised joints (joints driving the player's wheels/legs/arms) ──
    // Supports both wheel joints (car/truck) and revolute joints (walker/crane)
    struct RubeMotorJoint {
        b2Joint*  joint;       // base pointer (cast to wheel/revolute as needed)
        int       jointType;   // 0 = wheel, 1 = revolute
        float     maxTorque;
    };
    RubeMotorJoint motorJoints[16];
    int           numMotorJoints;

    // ── Scene bounds (for camera clamping) ──
    float      minX, maxX, minY, maxY;

    // ── Images (textured quads attached to bodies) ──
    RubeImage  images[RUBE_MAX_IMAGES];
    int        numImages;
};

extern RubeScene g_rubeScene;

//============================================================================================
// PUBLIC API
//============================================================================================

// Load a R.U.B.E. JSON file and create the Box2D world.
// Returns true on success.  Destroys any previously loaded scene.
bool RubeLoadScene(const char* jsonPath);

// Unload the current scene and free all Box2D objects.
void RubeUnloadScene();

// Restart the current scene (unload + reload from the same JSON path).
// Returns true on success.
bool RubeRestartScene();

// Step the physics world and apply gamepad input to motor joints.
// throttleInput: -1..+1 from triggers (wheel joints for driving)
// turretInput:   -1..+1 from right stick Y (revolute joints for turret/arms)
void RubeUpdate(float dt, float throttleInput, float turretInput = 0.0f);

// Get the player body position for camera targeting.
// Returns false if no scene is active.
bool RubeGetCameraTarget(float& outX, float& outY);

// Get the player body's current speed (world units/sec) for HUD.
float RubeGetPlayerSpeed();

// Get the player body's angle (radians) for rendering.
float RubeGetPlayerAngle();

// Check if a R.U.B.E. scene is currently active.
bool RubeIsActive();

//============================================================================================
// RENDER HELPERS
// Draw all R.U.B.E. bodies using our existing primitive renderer.
// Polygon outlines for static terrain, filled quads for dynamic bodies.
//============================================================================================

// Render the entire R.U.B.E. scene.
// Uses the current D3D device and camera transform from g_fScrollPos.
void RubeRenderScene();
