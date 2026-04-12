#pragma once
#include "stdafx.h"

//============================================================================================
// THREADING SYSTEM — Xbox 360 Multi-Core Support
//
// Xbox 360 Core Layout:
//   Core 0, Thread 0 — Main game loop, input, rendering
//   Core 0, Thread 1 — Reserved (future use)
//   Core 1, Thread 2 — Physics worker (Box2D stepping)
//   Core 1, Thread 3 — Reserved (future use)
//   Core 2, Thread 4 — Async loading (textures, levels)
//   Core 2, Thread 5 — XAudio2 engine
//
// Design:
//   - Physics stepping runs on Core 1 in parallel with rendering on Core 0
//   - Main thread kicks physics at start of frame, renders last frame's state,
//     then waits for physics to finish before processing input for next frame
//   - Body positions are double-buffered: renderer reads cache while physics writes world
//   - Terrain generation can run on Core 2 during loading screens
//
// Safety:
//   - Box2D world is ONLY touched by the physics thread during stepping
//   - Renderer reads from the position cache (never from b2World directly during step)
//   - All shared state protected by CRITICAL_SECTIONs
//============================================================================================

//============================================================================================
// POSITION CACHE — double-buffered body transforms for thread-safe rendering
//============================================================================================
#define THREAD_MAX_CACHED_BODIES 512

struct CachedBodyTransform {
    float x, y;        // world position
    float angle;        // rotation
    bool  isStatic;     // body type (for render pass selection)
    bool  isPlayer;     // is this the player body?
    bool  isValid;      // slot in use?
};

//============================================================================================
// PHYSICS WORKER — runs Box2D stepping on Core 1
//============================================================================================
struct PhysicsWorker {
    // Thread handle
    HANDLE          hThread;
    DWORD           threadId;

    // Synchronization events
    HANDLE          hKickEvent;     // main → physics: "start stepping"
    HANDLE          hDoneEvent;     // physics → main: "step finished"
    HANDLE          hShutdownEvent; // main → physics: "exit thread"

    // Shared state (protected by critical section)
    CRITICAL_SECTION cs;

    // Physics input (set by main thread before kick)
    float           throttleInput;  // gamepad input for motors
    float           dt;             // frame delta time

    // State
    bool            active;         // thread is running
    bool            enabled;        // threading enabled (can be toggled)

    // Performance counters
    LARGE_INTEGER   stepStartTime;
    LARGE_INTEGER   stepEndTime;
    float           lastStepMs;     // physics step time in milliseconds
};

extern PhysicsWorker g_physicsWorker;

//============================================================================================
// ASYNC LOADER — runs file I/O on Core 2
//============================================================================================
struct AsyncLoader {
    HANDLE          hThread;
    HANDLE          hWorkEvent;
    HANDLE          hDoneEvent;
    HANDLE          hShutdownEvent;
    CRITICAL_SECTION cs;

    // Work request
    enum WorkType { WORK_NONE, WORK_LOAD_RUBE_TEXTURES };
    WorkType        pendingWork;
    char            workPath[256];
    bool            active;
    bool            workDone;
};

extern AsyncLoader g_asyncLoader;

//============================================================================================
// PUBLIC API
//============================================================================================

// Initialize threading system — call once at startup after XSetThreadProcessor
void ThreadingInit();

// Shutdown threading system — call before exit
void ThreadingShutdown();

// Kick physics step on worker thread (non-blocking)
// Call at START of frame, before rendering
void PhysicsKick(float dt, float throttleInput);

// Wait for physics step to complete
// Call AFTER rendering, before processing next frame's input
void PhysicsWait();

// Check if physics threading is enabled
bool IsPhysicsThreaded();

// Enable/disable physics threading (can be toggled at runtime)
void SetPhysicsThreaded(bool enabled);

// Get last physics step time in milliseconds (for debug display)
float GetPhysicsStepTimeMs();
