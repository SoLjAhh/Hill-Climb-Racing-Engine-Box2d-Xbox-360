#pragma once
#include "stdafx.h"
#include "globals.h"

void InitBox2DPhysics();
void SetLevelJumps(Level level);  // defined in main.cpp
void SetLevelWater(Level level);
void InitWaterSurface();
void ApplyWaterForces();
void UpdateWaterHeightfield(float dt);
void SplashWater(float worldX, float splashHeight);
void DestroyWaterRegions();
void LoadLevel(Level level);
void InitTime();
void UpdateTime();
HRESULT InitD3D();
HRESULT InitScene();
