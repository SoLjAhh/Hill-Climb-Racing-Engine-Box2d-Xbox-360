#pragma once
#include "stdafx.h"
#include "globals.h"

void LoadConfig();
void LoadModTextures();

//============================================================================================
// CAR CFG LOADER
// Reads game:\Config\Cars\<name>.cfg and overwrites the car parameter globals.
// Returns true if the file was found and parsed, false if defaults are used.
//============================================================================================
bool LoadCarCfg(const char* carName);
