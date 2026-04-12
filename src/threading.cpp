#include "stdafx.h"
#include "threading.h"
#include "globals.h"
#include "rube_loader.h"
#include "physics.h"

//============================================================================================
// GLOBALS
//============================================================================================
PhysicsWorker g_physicsWorker;
AsyncLoader   g_asyncLoader;

//============================================================================================
// PHYSICS WORKER THREAD
// Runs on Core 1, Thread 2. Waits for kick event, steps Box2D, signals done.
//============================================================================================
static DWORD WINAPI PhysicsThreadProc(LPVOID param) {
    // Pin to Core 1, Hardware Thread 0
    XSetThreadProcessor(GetCurrentThread(), 2);

    OutputDebugStringA("THREAD: Physics worker started on Core 1 (Thread 2)\n");

    HANDLE waitHandles[2];
    waitHandles[0] = g_physicsWorker.hKickEvent;
    waitHandles[1] = g_physicsWorker.hShutdownEvent;

    for (;;) {
        // Wait for either kick or shutdown
        DWORD result = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

        // Shutdown requested
        if (result == WAIT_OBJECT_0 + 1) {
            OutputDebugStringA("THREAD: Physics worker shutting down\n");
            break;
        }

        // Kick received — step physics
        if (result == WAIT_OBJECT_0) {
            LARGE_INTEGER startTime;
            QueryPerformanceCounter(&startTime);

            // Read input (thread-safe — main thread doesn't modify after kick)
            float dt       = g_physicsWorker.dt;
            float throttle = g_physicsWorker.throttleInput;

            // ── Step the appropriate physics world ──
            if (RubeIsActive()) {
                // R.U.B.E. scene physics (most expensive — benefits most from threading)
                RubeUpdate(dt, throttle);
            } else if (g_pBox2DWorld != NULL) {
                // Normal HCR game physics
                g_pBox2DWorld->Step(1.0f / 60.0f, 12, 4);
            }

            LARGE_INTEGER endTime;
            QueryPerformanceCounter(&endTime);

            // Record timing
            g_physicsWorker.stepStartTime = startTime;
            g_physicsWorker.stepEndTime   = endTime;

            // Signal done
            SetEvent(g_physicsWorker.hDoneEvent);
        }
    }

    return 0;
}

//============================================================================================
// ASYNC LOADER THREAD
// Runs on Core 2, Thread 4. Handles texture loading during gameplay.
//============================================================================================
static DWORD WINAPI AsyncLoaderThreadProc(LPVOID param) {
    // Pin to Core 2, Hardware Thread 0
    XSetThreadProcessor(GetCurrentThread(), 4);

    OutputDebugStringA("THREAD: Async loader started on Core 2 (Thread 4)\n");

    HANDLE waitHandles[2];
    waitHandles[0] = g_asyncLoader.hWorkEvent;
    waitHandles[1] = g_asyncLoader.hShutdownEvent;

    for (;;) {
        DWORD result = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

        if (result == WAIT_OBJECT_0 + 1) {
            OutputDebugStringA("THREAD: Async loader shutting down\n");
            break;
        }

        if (result == WAIT_OBJECT_0) {
            EnterCriticalSection(&g_asyncLoader.cs);
            AsyncLoader::WorkType work = g_asyncLoader.pendingWork;
            g_asyncLoader.pendingWork = AsyncLoader::WORK_NONE;
            LeaveCriticalSection(&g_asyncLoader.cs);

            // Process work item
            switch (work) {
            case AsyncLoader::WORK_LOAD_RUBE_TEXTURES:
                // Future: load textures on background thread
                OutputDebugStringA("THREAD: Async texture loading (placeholder)\n");
                break;
            default:
                break;
            }

            EnterCriticalSection(&g_asyncLoader.cs);
            g_asyncLoader.workDone = true;
            LeaveCriticalSection(&g_asyncLoader.cs);

            SetEvent(g_asyncLoader.hDoneEvent);
        }
    }

    return 0;
}

//============================================================================================
// INIT / SHUTDOWN
//============================================================================================

void ThreadingInit() {
    OutputDebugStringA("THREAD: Initializing threading system\n");

    // ── Physics Worker ──
    memset(&g_physicsWorker, 0, sizeof(PhysicsWorker));
    InitializeCriticalSection(&g_physicsWorker.cs);
    g_physicsWorker.hKickEvent     = CreateEvent(NULL, FALSE, FALSE, NULL); // auto-reset
    g_physicsWorker.hDoneEvent     = CreateEvent(NULL, FALSE, FALSE, NULL); // auto-reset
    g_physicsWorker.hShutdownEvent = CreateEvent(NULL, TRUE,  FALSE, NULL); // manual-reset
    g_physicsWorker.enabled        = false;  // disabled by default — enable when fully integrated
    g_physicsWorker.active         = true;

    g_physicsWorker.hThread = CreateThread(
        NULL, 65536, // 64KB stack (sufficient for Box2D)
        PhysicsThreadProc, NULL, 0, &g_physicsWorker.threadId);

    if (!g_physicsWorker.hThread) {
        OutputDebugStringA("THREAD: WARNING — failed to create physics thread, falling back to single-threaded\n");
        g_physicsWorker.enabled = false;
        g_physicsWorker.active  = false;
    }

    // ── Async Loader ──
    memset(&g_asyncLoader, 0, sizeof(AsyncLoader));
    InitializeCriticalSection(&g_asyncLoader.cs);
    g_asyncLoader.hWorkEvent     = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_asyncLoader.hDoneEvent     = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_asyncLoader.hShutdownEvent = CreateEvent(NULL, TRUE,  FALSE, NULL);
    g_asyncLoader.pendingWork    = AsyncLoader::WORK_NONE;
    g_asyncLoader.active         = true;

    g_asyncLoader.hThread = CreateThread(
        NULL, 65536,
        AsyncLoaderThreadProc, NULL, 0, NULL);

    if (!g_asyncLoader.hThread) {
        OutputDebugStringA("THREAD: WARNING — failed to create async loader thread\n");
        g_asyncLoader.active = false;
    }

    OutputDebugStringA("THREAD: Threading system initialized\n");
}

void ThreadingShutdown() {
    OutputDebugStringA("THREAD: Shutting down threading system\n");

    // ── Shutdown physics worker ──
    if (g_physicsWorker.active) {
        SetEvent(g_physicsWorker.hShutdownEvent);
        WaitForSingleObject(g_physicsWorker.hThread, 2000); // 2 second timeout
        CloseHandle(g_physicsWorker.hThread);
        CloseHandle(g_physicsWorker.hKickEvent);
        CloseHandle(g_physicsWorker.hDoneEvent);
        CloseHandle(g_physicsWorker.hShutdownEvent);
        DeleteCriticalSection(&g_physicsWorker.cs);
        g_physicsWorker.active = false;
    }

    // ── Shutdown async loader ──
    if (g_asyncLoader.active) {
        SetEvent(g_asyncLoader.hShutdownEvent);
        WaitForSingleObject(g_asyncLoader.hThread, 2000);
        CloseHandle(g_asyncLoader.hThread);
        CloseHandle(g_asyncLoader.hWorkEvent);
        CloseHandle(g_asyncLoader.hDoneEvent);
        CloseHandle(g_asyncLoader.hShutdownEvent);
        DeleteCriticalSection(&g_asyncLoader.cs);
        g_asyncLoader.active = false;
    }

    OutputDebugStringA("THREAD: Threading system shut down\n");
}

//============================================================================================
// PHYSICS KICK / WAIT
// Called from main thread to overlap physics with rendering.
//
// Frame timeline:
//   Main thread:  [Input] → PhysicsKick() → [Render] → PhysicsWait() → [next frame]
//   Physics thread:                         [Step Box2D]
//
// This overlaps physics stepping with rendering, using both cores simultaneously.
//============================================================================================

void PhysicsKick(float dt, float throttleInput) {
    if (!g_physicsWorker.enabled || !g_physicsWorker.active) return;

    // Set input for physics thread
    g_physicsWorker.dt             = dt;
    g_physicsWorker.throttleInput  = throttleInput;

    // Signal physics thread to start stepping
    SetEvent(g_physicsWorker.hKickEvent);
}

void PhysicsWait() {
    if (!g_physicsWorker.enabled || !g_physicsWorker.active) return;

    // Wait for physics step to complete
    WaitForSingleObject(g_physicsWorker.hDoneEvent, 100); // 100ms timeout

    // Calculate step time
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    float elapsed = (float)(g_physicsWorker.stepEndTime.QuadPart -
                            g_physicsWorker.stepStartTime.QuadPart);
    g_physicsWorker.lastStepMs = (elapsed / (float)freq.QuadPart) * 1000.0f;
}

bool IsPhysicsThreaded() {
    return g_physicsWorker.enabled && g_physicsWorker.active;
}

void SetPhysicsThreaded(bool enabled) {
    g_physicsWorker.enabled = enabled;
}

float GetPhysicsStepTimeMs() {
    return g_physicsWorker.lastStepMs;
}
