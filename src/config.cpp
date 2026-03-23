#include "stdafx.h"
#include "config.h"
#include "globals.h"

//============================================================================================

// ─── helper: copy string safely into fixed-size buffer ─────────────────────
static void CfgStrCopy(char* dst, int dstSize, const char* src) {
    int i = 0;
    while (i < dstSize - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

struct ConfigEntry {
    const char* key;
    float*      target;
};

// Enum for which section we are currently inside while parsing
enum CfgSection { CFG_GLOBAL = 0, CFG_CAR, CFG_LEVEL };

void LoadConfig() {
    // ── zero-initialise all mod slots ────────────────────────────────────────
    for (int i = 0; i < MAX_CUSTOM_CARS;   i++) {
        memset(&g_customCars[i],   0, sizeof(CustomCarDef));
        // sensible physics defaults so partial definitions still work
        g_customCars[i].carWidth          = 3.1f;
        g_customCars[i].carHeight         = 1.25f;
        g_customCars[i].wheelRadius       = 0.35f;
        g_customCars[i].wheelJointFreq    = 4.0f;
        g_customCars[i].wheelJointDamp    = 0.8f;
        g_customCars[i].motorMaxSpeed     = 60.0f;
        g_customCars[i].motorTorqueRear   = 7.0f;
        g_customCars[i].motorTorqueFront  = 5.0f;
        g_customCars[i].rideHeightOffset  = 0.0f;
        g_customCars[i].wheelRearOffsetX  = -0.96f;
        g_customCars[i].wheelFrontOffsetX = 1.0f;
    }
    for (int i = 0; i < MAX_CUSTOM_LEVELS; i++) {
        memset(&g_customLevels[i], 0, sizeof(CustomLevelDef));
        g_customLevels[i].amplitude    = 15.0f;
        g_customLevels[i].frequencyBase = 0.4f;
        g_customLevels[i].bridgeStartX  = 150.0f;
    }

    // ── global float tunables ────────────────────────────────────────────────
    ConfigEntry entries[] = {
        { "BARRIER_WALL_X",             &BARRIER_WALL_X            },
        { "FINISH_LINE_X",              &FINISH_LINE_X             },
        { "BRIDGE_L2_START_X",          &BRIDGE_L2_START_X         },
        { "BRIDGE_L4_START_X",          &BRIDGE_L4_START_X         },
        { "BRIDGE_SPAN",                &BRIDGE_SPAN               },
        { "BRIDGE_PLANK_HEIGHT",        &BRIDGE_PLANK_HEIGHT       },
        { "BRIDGE_PLANK_DENSITY",       &BRIDGE_PLANK_DENSITY      },
        { "BRIDGE_PLANK_FRICTION",      &BRIDGE_PLANK_FRICTION     },
        { "BRIDGE_PLANK_RESTITUTION",   &BRIDGE_PLANK_RESTITUTION  },
        { "BRIDGE_ANCHOR_HEIGHT",       &BRIDGE_ANCHOR_HEIGHT      },
        { "BRIDGE_ENTRY_NUDGE",         &BRIDGE_ENTRY_NUDGE        },
        { "BRIDGE_VALLEY_DEPTH",        &BRIDGE_VALLEY_DEPTH       },
        { "BRIDGE_APPROACH_LENGTH",     &BRIDGE_APPROACH_LENGTH    },
        { "BRIDGE_VALLEY_SLOPE",        &BRIDGE_VALLEY_SLOPE       },
        { "BRIDGE_VISUAL_WIDTH_SCALE",  &BRIDGE_VISUAL_WIDTH_SCALE },
        { "BRIDGE_VISUAL_HEIGHT_SCALE", &BRIDGE_VISUAL_HEIGHT_SCALE},
        { "BRIDGE_VISUAL_Y_OFFSET",     &BRIDGE_VISUAL_Y_OFFSET    },
    };
    const int entryCount = (int)(sizeof(entries) / sizeof(entries[0]));

    HANDLE hFile = CreateFileA(
        "game:\\Config\\settings.cfg",
        GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        OutputDebugStringA("LoadConfig: settings.cfg not found - using defaults\n");
        return;
    }

    char    lineBuf[256];
    DWORD   bytesRead = 0;
    int     linePos   = 0;
    CfgSection section = CFG_GLOBAL;
    int     carIdx    = -1;   // current [car] slot being filled
    int     lvlIdx    = -1;   // current [level] slot being filled

    // ── line processor lambda (called for every complete line) ───────────────
    // Written as a nested goto-less block for C++98 compatibility
    #define PROCESS_LINE() \
    do { \
        char* line = lineBuf; \
        while (*line == ' ' || *line == '\t') line++; \
        if (*line == '\0' || *line == '#') break; \
        /* ---- section header [car] or [level] ---- */ \
        if (*line == '[') { \
            char* end = line + 1; \
            while (*end && *end != ']') end++; \
            if (*end == ']') { \
                *end = '\0'; \
                char* hdr = line + 1; \
                if (strcmp(hdr, "car") == 0) { \
                    section = CFG_CAR; \
                    carIdx = g_numCustomCars; \
                    if (carIdx < MAX_CUSTOM_CARS) { \
                        g_customCars[carIdx].loaded = true; \
                        g_numCustomCars++; \
                        g_totalCarSlots = CAR_CUSTOM_BASE + g_numCustomCars; \
                    } else { carIdx = -1; } \
                } else if (strcmp(hdr, "level") == 0) { \
                    section = CFG_LEVEL; \
                    lvlIdx = g_numCustomLevels; \
                    if (lvlIdx < MAX_CUSTOM_LEVELS) { \
                        g_customLevels[lvlIdx].loaded = true; \
                        g_numCustomLevels++; \
                        g_totalLevelSlots = 7 + g_numCustomLevels; \
                    } else { lvlIdx = -1; } \
                } \
            } \
            break; \
        } \
        /* ---- key = value pair ---- */ \
        char* eq = line; \
        while (*eq && *eq != '=') eq++; \
        if (*eq != '=') break; \
        *eq = '\0'; \
        char* keyEnd = eq - 1; \
        while (keyEnd >= line && (*keyEnd == ' ' || *keyEnd == '\t')) *keyEnd-- = '\0'; \
        char* valStr = eq + 1; \
        while (*valStr == ' ' || *valStr == '\t') valStr++; \
        /* strip trailing whitespace from value */ \
        char* valEnd = valStr + strlen(valStr) - 1; \
        while (valEnd > valStr && (*valEnd == ' ' || *valEnd == '\t' || *valEnd == '\r')) \
            *valEnd-- = '\0'; \
        if (section == CFG_GLOBAL) { \
            float fval = (float)atof(valStr); \
            for (int _i = 0; _i < entryCount; _i++) { \
                if (strcmp(line, entries[_i].key) == 0) { \
                    *entries[_i].target = fval; break; \
                } \
            } \
        } else if (section == CFG_CAR && carIdx >= 0) { \
            CustomCarDef& cc = g_customCars[carIdx]; \
            float fval = (float)atof(valStr); \
            if      (strcmp(line,"name")               == 0) CfgStrCopy(cc.name,           64,  valStr); \
            else if (strcmp(line,"body_tex")           == 0) CfgStrCopy(cc.bodyTexPath,     128, valStr); \
            else if (strcmp(line,"wheel_tex")          == 0) CfgStrCopy(cc.wheelTexPath,    128, valStr); \
            else if (strcmp(line,"icon_tex")           == 0) CfgStrCopy(cc.iconTexPath,     128, valStr); \
            else if (strcmp(line,"car_width")          == 0) cc.carWidth          = fval; \
            else if (strcmp(line,"car_height")         == 0) cc.carHeight         = fval; \
            else if (strcmp(line,"wheel_radius")       == 0) cc.wheelRadius       = fval; \
            else if (strcmp(line,"wheel_joint_freq")   == 0) cc.wheelJointFreq    = fval; \
            else if (strcmp(line,"wheel_joint_damp")   == 0) cc.wheelJointDamp    = fval; \
            else if (strcmp(line,"motor_max_speed")    == 0) cc.motorMaxSpeed     = fval; \
            else if (strcmp(line,"motor_torque_rear")  == 0) cc.motorTorqueRear   = fval; \
            else if (strcmp(line,"motor_torque_front") == 0) cc.motorTorqueFront  = fval; \
            else if (strcmp(line,"ride_height_offset") == 0) cc.rideHeightOffset  = fval; \
            else if (strcmp(line,"wheel_rear_offset_x")  == 0) cc.wheelRearOffsetX  = fval; \
            else if (strcmp(line,"wheel_front_offset_x") == 0) cc.wheelFrontOffsetX = fval; \
        } else if (section == CFG_LEVEL && lvlIdx >= 0) { \
            CustomLevelDef& cl = g_customLevels[lvlIdx]; \
            float fval = (float)atof(valStr); \
            if      (strcmp(line,"name")           == 0) CfgStrCopy(cl.name,           64,  valStr); \
            else if (strcmp(line,"terrain_tex")    == 0) CfgStrCopy(cl.terrainTexPath, 128, valStr); \
            else if (strcmp(line,"bg_tex")         == 0) CfgStrCopy(cl.bgTexPath,      128, valStr); \
            else if (strcmp(line,"bg2_tex")        == 0) CfgStrCopy(cl.bg2TexPath,     128, valStr); \
            else if (strcmp(line,"bg3_tex")        == 0) CfgStrCopy(cl.bg3TexPath,     128, valStr); \
            else if (strcmp(line,"amplitude")      == 0) cl.amplitude     = fval; \
            else if (strcmp(line,"frequency")      == 0) cl.frequencyBase = fval; \
            else if (strcmp(line,"has_bridge")     == 0) cl.hasBridge     = fval; \
            else if (strcmp(line,"bridge_start_x") == 0) cl.bridgeStartX  = fval; \
            else if (strcmp(line,"icon_tex")        == 0) CfgStrCopy(cl.iconTexPath, 128, valStr); \
        } \
    } while(0)

    char ch;
    while (ReadFile(hFile, &ch, 1, &bytesRead, NULL) && bytesRead == 1) {
        if (ch == '\r') continue;
        if (ch == '\n' || linePos >= 254) {
            lineBuf[linePos] = '\0';
            linePos = 0;
            PROCESS_LINE();
        } else {
            lineBuf[linePos++] = ch;
        }
    }
    if (linePos > 0) { lineBuf[linePos] = '\0'; PROCESS_LINE(); }
    #undef PROCESS_LINE

    CloseHandle(hFile);
    OutputDebugStringA("LoadConfig: settings.cfg loaded OK\n");

    char dbg[128];
    sprintf_s(dbg, sizeof(dbg), "LoadConfig: %d custom cars, %d custom levels\n",
              g_numCustomCars, g_numCustomLevels);
    OutputDebugStringA(dbg);
}

// ─── LoadModTextures ─────────────────────────────────────────────────────────
// Must be called AFTER the D3D device is created (called from InitScene).
// Loads body/wheel/icon textures for all custom cars, and terrain/bg textures
// for all custom levels, using paths stored by LoadConfig().
void LoadModTextures() {
    for (int i = 0; i < g_numCustomCars; i++) {
        CustomCarDef& cc = g_customCars[i];
        if (!cc.loaded) continue;
        if (cc.bodyTexPath[0])
            D3DXCreateTextureFromFileA(g_pd3dDevice, cc.bodyTexPath,  &cc.pTexBody);
        if (cc.wheelTexPath[0])
            D3DXCreateTextureFromFileA(g_pd3dDevice, cc.wheelTexPath, &cc.pTexWheel);
        if (cc.iconTexPath[0])
            D3DXCreateTextureFromFileA(g_pd3dDevice, cc.iconTexPath,  &cc.pTexIcon);
    }
    for (int i = 0; i < g_numCustomLevels; i++) {
        CustomLevelDef& cl = g_customLevels[i];
        if (!cl.loaded) continue;
        if (cl.terrainTexPath[0])
            D3DXCreateTextureFromFileA(g_pd3dDevice, cl.terrainTexPath, &cl.pTexTerrain);
        if (cl.bgTexPath[0])
            D3DXCreateTextureFromFileA(g_pd3dDevice, cl.bgTexPath,      &cl.pTexBg);
        if (cl.bg2TexPath[0])
            D3DXCreateTextureFromFileA(g_pd3dDevice, cl.bg2TexPath,     &cl.pTexBg2);
        if (cl.bg3TexPath[0])
            D3DXCreateTextureFromFileA(g_pd3dDevice, cl.bg3TexPath,     &cl.pTexBg3);
        if (cl.iconTexPath[0])
            D3DXCreateTextureFromFileA(g_pd3dDevice, cl.iconTexPath,    &cl.pTexIcon);
    }
    OutputDebugStringA("LoadModTextures: done\n");
}

//============================================================================================

//============================================================================================
// LoadCarCfg
// Reads game:\Config\Cars\<carName>.cfg and applies values to car parameter globals.
// Format (one key = value per line, # comments ignored):
//   car_width           = 3.1
//   car_height          = 1.25
//   wheel_radius        = 0.35
//   wheel_joint_freq    = 4.0
//   wheel_joint_damping = 0.8
//   motor_max_speed     = 60.0
//   motor_torque_rear   = 7.0
//   motor_torque_front  = 5.0
//   ride_height_offset  = 0.0
//   wheel_rear_offset_x = -0.96
//   wheel_front_offset_x = 1.0
// Any missing key keeps the hardcoded default — safe to have partial cfg files.
//============================================================================================
bool LoadCarCfg(const char* carName) {
    char path[128];
    sprintf_s(path, sizeof(path), "game:\\Config\\Cars\\%s.cfg", carName);

    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize > 8192) { CloseHandle(hFile); return false; }

    char* buf = new char[fileSize + 1];
    DWORD bytesRead = 0;
    ReadFile(hFile, buf, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);
    buf[bytesRead] = '\0';

    // Parse line by line
    char* ctx = NULL;
    char* line = strtok_s(buf, "\r\n", &ctx);
    while (line) {
        // Skip blank lines and comments
        while (*line == ' ' || *line == '\t') line++;
        if (*line == '#' || *line == '\0') { line = strtok_s(NULL, "\r\n", &ctx); continue; }

        // Split at '='
        char* eq = strchr(line, '=');
        if (!eq) { line = strtok_s(NULL, "\r\n", &ctx); continue; }
        *eq = '\0';
        char* key = line;
        char* val = eq + 1;

        // Trim whitespace from key and value
        while (*key == ' ' || *key == '\t') key++;
        char* ke = key + strlen(key) - 1;
        while (ke > key && (*ke == ' ' || *ke == '\t')) *ke-- = '\0';
        while (*val == ' ' || *val == '\t') val++;

        float fval = (float)atof(val);

        if      (strcmp(key, "car_width")           == 0) g_visualCarWidth        = fval;
        else if (strcmp(key, "car_height")          == 0) g_visualCarHeight       = fval;
        else if (strcmp(key, "wheel_radius")        == 0) g_visualWheelRadius     = fval;
        else if (strcmp(key, "wheel_joint_freq")    == 0) g_wheelJointFrequency   = fval;
        else if (strcmp(key, "wheel_joint_damping") == 0) g_wheelJointDamping     = fval;
        else if (strcmp(key, "motor_max_speed")     == 0) g_motorMaxSpeed         = fval;
        else if (strcmp(key, "motor_torque_rear")   == 0) g_motorTorqueRear       = fval;
        else if (strcmp(key, "motor_torque_front")  == 0) g_motorTorqueFront      = fval;
        else if (strcmp(key, "ride_height_offset")  == 0) g_rideHeightOffset      = fval;
        else if (strcmp(key, "wheel_rear_offset_x") == 0) g_wheelRearOffsetX      = fval;
        else if (strcmp(key, "wheel_front_offset_x")== 0) g_wheelFrontOffsetX     = fval;

        line = strtok_s(NULL, "\r\n", &ctx);
    }

    delete[] buf;

    char dbg[128];
    sprintf_s(dbg, sizeof(dbg), "CarCfg: loaded %s.cfg\n", carName);
    OutputDebugStringA(dbg);
    return true;
}
