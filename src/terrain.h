#pragma once
#include "stdafx.h"
#include "globals.h"

//============================================================================================
// FUNCTION DECLARATIONS
//============================================================================================

float GetTerrainHeightAtX(float worldX);
float GenerateTerrainHeightProcedural(float worldX);
void  GenerateWrappingTerrain();
