#pragma once
#include "stdafx.h"
#include "globals.h"

void RenderWheel(float parentX, float parentY, float localX, float localY, float carRot, float spin);
void RenderBody(float x, float y, float w, float h, float rot);
void RenderBackground();
void RenderBridge();
void RenderTitleScreen();
void RenderCarSelectScreen();
void RenderMainMenu();
void RenderSettingsScreen();
void RenderAboutScreen();
void RenderLevelSelectScreen();
void RenderPauseMenu();
void RenderHUDGauges();
void RenderWater();
void RenderParticles();
void RenderSnowfall();
void Render();
