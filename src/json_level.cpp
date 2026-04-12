#include "stdafx.h"
#include "json_level.h"
#include "config.h"

//============================================================================================
// JSON LEVEL EXTRA DATA — global storage
//============================================================================================
JsonLevelExtra g_jsonLevelExtra[MAX_JSON_LEVELS];

//============================================================================================
// R.U.B.E. SCENE PATH DISCOVERY — global storage
//============================================================================================
char g_rubeScenePaths[MAX_RUBE_SCENES][256];
char g_rubeSceneNames[MAX_RUBE_SCENES][64];
LPDIRECT3DTEXTURE9 g_rubeSceneIcons[MAX_RUBE_SCENES];
int  g_numRubeScenes = 0;

//============================================================================================
// MINIMAL JSON PARSER IMPLEMENTATION
// Recursive-descent, cursor-based.  No heap allocation, no STL.
// Handles: objects, arrays, strings, numbers, booleans, null.
// Limitations: no unicode escapes, no scientific notation, 8 KB max string values.
//============================================================================================

void JsonReader::Init(const char* jsonText, int jsonLen) {
    buf = jsonText;
    len = jsonLen;
    pos = 0;
}

void JsonReader::SkipWhitespace() {
    while (pos < len && (buf[pos] == ' '  || buf[pos] == '\t' ||
                         buf[pos] == '\r' || buf[pos] == '\n'))
        pos++;
}

// Skip one complete JSON value (string, number, object, array, bool, null).
// Returns the position AFTER the skipped value.
int JsonReader::SkipValue() {
    SkipWhitespace();
    if (pos >= len) return pos;

    char c = buf[pos];

    // String
    if (c == '"') {
        pos++; // opening quote
        while (pos < len) {
            if (buf[pos] == '\\') { pos += 2; continue; } // escaped char
            if (buf[pos] == '"') { pos++; return pos; }
            pos++;
        }
        return pos;
    }

    // Object or array
    if (c == '{' || c == '[') {
        int end = FindMatchingBrace(pos);
        pos = end + 1;
        return pos;
    }

    // true / false / null / number
    while (pos < len && buf[pos] != ',' && buf[pos] != '}' && buf[pos] != ']'
           && buf[pos] != ' ' && buf[pos] != '\t' && buf[pos] != '\r' && buf[pos] != '\n')
        pos++;
    return pos;
}

// Find the matching closing brace/bracket for the one at 'from'.
int JsonReader::FindMatchingBrace(int from) {
    if (from >= len) return len;
    char open  = buf[from];
    char close = (open == '{') ? '}' : ']';
    int depth = 1;
    int i = from + 1;
    bool inString = false;

    while (i < len && depth > 0) {
        char c = buf[i];
        if (inString) {
            if (c == '\\') { i += 2; continue; }
            if (c == '"')  inString = false;
        } else {
            if (c == '"')  inString = true;
            else if (c == open)  depth++;
            else if (c == close) depth--;
        }
        if (depth > 0) i++;
    }
    return i;
}

// Find "key": in the current object scope and position cursor after the colon.
bool JsonReader::FindKey(const char* key) {
    return FindKeyIn(key, 0, len);
}

bool JsonReader::FindKeyIn(const char* key, int start, int end) {
    int keyLen = 0;
    while (key[keyLen]) keyLen++;

    int i = start;
    while (i < end) {
        // Find next quote
        while (i < end && buf[i] != '"') i++;
        if (i >= end) return false;
        i++; // skip opening quote

        // Compare key
        int ki = 0;
        bool match = true;
        while (i < end && buf[i] != '"') {
            if (ki < keyLen) {
                if (buf[i] != key[ki]) match = false;
            } else {
                match = false;
            }
            ki++;
            i++;
        }
        if (ki != keyLen) match = false;
        if (i >= end) return false;
        i++; // skip closing quote

        // Skip whitespace and colon
        while (i < end && (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\r' || buf[i] == '\n'))
            i++;
        if (i < end && buf[i] == ':') {
            i++; // skip colon
            while (i < end && (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\r' || buf[i] == '\n'))
                i++;
            if (match) {
                pos = i;
                return true;
            }
        }

        // Not a match or not a key — skip the value and continue
        if (i < end) {
            pos = i;
            SkipValue();
            i = pos;
        }
    }
    return false;
}

float JsonReader::ReadFloat() {
    SkipWhitespace();
    // Collect chars into a small buffer for atof
    char tmp[64];
    int ti = 0;
    bool neg = false;
    if (pos < len && buf[pos] == '-') { neg = true; pos++; }
    while (pos < len && ti < 62 &&
           ((buf[pos] >= '0' && buf[pos] <= '9') || buf[pos] == '.' ||
            buf[pos] == 'e' || buf[pos] == 'E' || buf[pos] == '+' || buf[pos] == '-')) {
        // Allow '-' only as first char or after e/E
        if (buf[pos] == '-' && ti > 0 && tmp[ti-1] != 'e' && tmp[ti-1] != 'E') break;
        if (buf[pos] == '+' && ti > 0 && tmp[ti-1] != 'e' && tmp[ti-1] != 'E') break;
        tmp[ti++] = buf[pos++];
    }
    tmp[ti] = '\0';
    float val = (float)atof(tmp);
    return neg ? -val : val;
}

int JsonReader::ReadInt() {
    return (int)ReadFloat();
}

bool JsonReader::ReadString(char* dst, int dstSize) {
    SkipWhitespace();
    if (pos >= len || buf[pos] != '"') { dst[0] = '\0'; return false; }
    pos++; // skip opening quote

    int di = 0;
    while (pos < len && buf[pos] != '"') {
        if (buf[pos] == '\\' && pos + 1 < len) {
            pos++; // skip backslash
            char esc = buf[pos++];
            char out = esc;
            if (esc == 'n') out = '\n';
            else if (esc == 't') out = '\t';
            else if (esc == 'r') out = '\r';
            if (di < dstSize - 1) dst[di++] = out;
        } else {
            if (di < dstSize - 1) dst[di++] = buf[pos];
            pos++;
        }
    }
    dst[di] = '\0';
    if (pos < len && buf[pos] == '"') pos++; // skip closing quote
    return true;
}

bool JsonReader::ReadBool() {
    SkipWhitespace();
    if (pos + 4 <= len && buf[pos] == 't' && buf[pos+1] == 'r' &&
        buf[pos+2] == 'u' && buf[pos+3] == 'e') {
        pos += 4;
        return true;
    }
    if (pos + 5 <= len && buf[pos] == 'f' && buf[pos+1] == 'a' &&
        buf[pos+2] == 'l' && buf[pos+3] == 's' && buf[pos+4] == 'e') {
        pos += 5;
        return false;
    }
    return false;
}

bool JsonReader::EnterArray() {
    SkipWhitespace();
    if (pos >= len || buf[pos] != '[') return false;
    pos++; // skip '['
    SkipWhitespace();
    return true;
}

bool JsonReader::NextElement() {
    SkipWhitespace();
    if (pos >= len) return false;
    if (buf[pos] == ']') return false; // end of array
    if (buf[pos] == ',') {
        pos++;
        SkipWhitespace();
    }
    if (pos >= len || buf[pos] == ']') return false;
    return true;
}

int JsonReader::ArrayCount() {
    int saved = pos;
    SkipWhitespace();
    if (pos >= len || buf[pos] != '[') { pos = saved; return 0; }
    int braceEnd = FindMatchingBrace(pos);
    pos++; // skip '['
    SkipWhitespace();
    if (pos >= braceEnd) { pos = saved; return 0; }

    int count = 0;
    while (pos < braceEnd) {
        SkipWhitespace();
        if (buf[pos] == ']') break;
        count++;
        SkipValue();
        SkipWhitespace();
        if (pos < braceEnd && buf[pos] == ',') pos++;
    }
    pos = saved;
    return count;
}

bool JsonReader::EnterObject() {
    SkipWhitespace();
    if (pos >= len || buf[pos] != '{') return false;
    pos++; // skip '{'
    SkipWhitespace();
    return true;
}

//============================================================================================
// HELPER: safe string copy (same as CfgStrCopy in config.cpp)
//============================================================================================
static void JsonStrCopy(char* dst, int dstSize, const char* src) {
    int i = 0;
    while (i < dstSize - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

//============================================================================================
// HELPER: read a file into a heap buffer (caller must delete[])
//============================================================================================
static char* ReadEntireFile(const char* path, int& outSize) {
    outSize = 0;
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize > 512 * 1024) { // 512 KB max
        CloseHandle(hFile);
        return NULL;
    }

    char* data = new char[fileSize + 1];
    DWORD bytesRead = 0;
    ReadFile(hFile, data, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);
    data[bytesRead] = '\0';
    outSize = (int)bytesRead;
    return data;
}

//============================================================================================
// DETECT FORMAT
// Returns:
//   1 = HCR native JSON        → load as custom level
//   2 = R.U.B.E. with hcr_     → load as custom level (R.U.B.E. level with our markers)
//   3 = R.U.B.E. raw scene     → skip here, handled by rube_loader.cpp as sandbox
//   4 = R.U.B.E. native (.rube)→ skip here, handled by rube_loader.cpp as sandbox
//   0 = unknown
//============================================================================================
static int DetectJsonFormat(JsonReader& jr) {
    int saved = jr.SavePos();

    // Check for R.U.B.E. native format (.rube files): has "metaworld" key
    jr.pos = 0;
    if (jr.FindKey("metaworld")) {
        jr.RestorePos(saved);
        return 4;  // R.U.B.E. native format
    }

    // Check for HCR format marker: "level_format"
    jr.pos = 0;
    if (jr.FindKey("level_format")) {
        char fmt[64];
        jr.ReadString(fmt, 64);
        jr.RestorePos(saved);
        if (strncmp(fmt, "hcr_level", 9) == 0) return 1;
    }

    // Check for R.U.B.E. JSON export: has "gravity" object AND "body" array
    jr.pos = 0;
    bool hasGravity = jr.FindKey("gravity");
    jr.pos = 0;
    bool hasBody = jr.FindKey("body");

    if (hasGravity && hasBody) {
        bool hasHcrMarkers = false;
        for (int i = 0; i < jr.len - 4; i++) {
            if (jr.buf[i] == 'h' && jr.buf[i+1] == 'c' &&
                jr.buf[i+2] == 'r' && jr.buf[i+3] == '_') {
                hasHcrMarkers = true;
                break;
            }
        }
        jr.RestorePos(saved);
        return hasHcrMarkers ? 2 : 3;
    }

    jr.RestorePos(saved);

    jr.pos = 0;
    if (jr.FindKey("terrain")) {
        jr.RestorePos(saved);
        return 1;
    }

    jr.RestorePos(saved);
    return 0;
}

//============================================================================================
// LOAD HCR NATIVE JSON FORMAT
//============================================================================================
// Expected structure:
// {
//     "level_format": "hcr_level_v1",
//     "name": "My Level",
//     "terrain": {
//         "amplitude": 15.0,
//         "frequency": 0.4,
//         "terrain_tex": "game:\\Textures\\...",
//         "bg_tex":      "game:\\Textures\\...",
//         "bg2_tex":     "game:\\Textures\\...",
//         "bg3_tex":     "game:\\Textures\\..."
//     },
//     "gravity": -9.81,
//     "spawn": { "x": 12.0, "y": 8.5 },
//     "bridge": { "enabled": true, "start_x": 150.0 },
//     "jumps": [
//         { "x": 60.0, "height": 2.5, "width": 5.0 },
//         ...
//     ],
//     "water": [
//         { "x": 75.0, "width": 18.0, "depth": 2.0 },
//         ...
//     ],
//     "music": "game:\\Audio\\Music\\custom.wav",
//     "icon_tex": "game:\\Textures\\icon.png"
// }
//============================================================================================
static bool LoadHcrJsonLevel(JsonReader& jr, int slotIndex) {
    CustomLevelDef& cl = g_customLevels[slotIndex];
    JsonLevelExtra& ex = g_jsonLevelExtra[slotIndex];

    // ── Name ──
    jr.pos = 0;
    if (jr.FindKey("name")) {
        jr.ReadString(cl.name, 64);
    }

    // ── Terrain block ──
    jr.pos = 0;
    if (jr.FindKey("terrain")) {
        // Find the object extent so we can search within it
        int objStart = jr.pos;
        int objEnd   = jr.FindMatchingBrace(objStart);

        jr.pos = objStart;
        if (jr.FindKeyIn("amplitude", objStart, objEnd))
            cl.amplitude = jr.ReadFloat();

        jr.pos = objStart;
        if (jr.FindKeyIn("frequency", objStart, objEnd))
            cl.frequencyBase = jr.ReadFloat();

        jr.pos = objStart;
        if (jr.FindKeyIn("terrain_tex", objStart, objEnd))
            jr.ReadString(cl.terrainTexPath, 128);

        jr.pos = objStart;
        if (jr.FindKeyIn("bg_tex", objStart, objEnd))
            jr.ReadString(cl.bgTexPath, 128);

        jr.pos = objStart;
        if (jr.FindKeyIn("bg2_tex", objStart, objEnd))
            jr.ReadString(cl.bg2TexPath, 128);

        jr.pos = objStart;
        if (jr.FindKeyIn("bg3_tex", objStart, objEnd))
            jr.ReadString(cl.bg3TexPath, 128);
    }

    // ── Gravity ──
    jr.pos = 0;
    if (jr.FindKey("gravity")) {
        ex.gravityY = jr.ReadFloat();
    }

    // ── Spawn point ──
    jr.pos = 0;
    if (jr.FindKey("spawn")) {
        int objStart = jr.pos;
        int objEnd   = jr.FindMatchingBrace(objStart);

        jr.pos = objStart;
        if (jr.FindKeyIn("x", objStart, objEnd))
            ex.spawnX = jr.ReadFloat();

        jr.pos = objStart;
        if (jr.FindKeyIn("y", objStart, objEnd))
            ex.spawnY = jr.ReadFloat();
    }

    // ── Bridge ──
    jr.pos = 0;
    if (jr.FindKey("bridge")) {
        int objStart = jr.pos;
        int objEnd   = jr.FindMatchingBrace(objStart);

        jr.pos = objStart;
        if (jr.FindKeyIn("enabled", objStart, objEnd)) {
            bool enabled = jr.ReadBool();
            cl.hasBridge = enabled ? 1.0f : 0.0f;
        }

        jr.pos = objStart;
        if (jr.FindKeyIn("start_x", objStart, objEnd))
            cl.bridgeStartX = jr.ReadFloat();
    }

    // ── Jumps array ──
    jr.pos = 0;
    if (jr.FindKey("jumps")) {
        if (jr.EnterArray()) {
            ex.numJumps = 0;
            while (ex.numJumps < MAX_JUMPS_PER_LEVEL) {
                jr.SkipWhitespace();
                if (jr.pos >= jr.len || jr.buf[jr.pos] == ']') break;

                // Each element is { "x": ..., "height": ..., "width": ... }
                int elemStart = jr.pos;
                int elemEnd   = jr.FindMatchingBrace(elemStart);
                JumpDef& jd = ex.jumps[ex.numJumps];

                jr.pos = elemStart;
                if (jr.FindKeyIn("x", elemStart, elemEnd))
                    jd.x = jr.ReadFloat();

                jr.pos = elemStart;
                if (jr.FindKeyIn("height", elemStart, elemEnd))
                    jd.height = jr.ReadFloat();

                jr.pos = elemStart;
                if (jr.FindKeyIn("width", elemStart, elemEnd))
                    jd.width = jr.ReadFloat();

                ex.numJumps++;
                jr.pos = elemEnd + 1; // skip past closing '}'
                jr.SkipWhitespace();
                if (jr.pos < jr.len && jr.buf[jr.pos] == ',') jr.pos++;
            }
        }
    }

    // ── Water array ──
    jr.pos = 0;
    if (jr.FindKey("water")) {
        if (jr.EnterArray()) {
            ex.numWater = 0;
            while (ex.numWater < MAX_WATER_REGIONS) {
                jr.SkipWhitespace();
                if (jr.pos >= jr.len || jr.buf[jr.pos] == ']') break;

                int elemStart = jr.pos;
                int elemEnd   = jr.FindMatchingBrace(elemStart);

                jr.pos = elemStart;
                if (jr.FindKeyIn("x", elemStart, elemEnd))
                    ex.waterX[ex.numWater] = jr.ReadFloat();

                jr.pos = elemStart;
                if (jr.FindKeyIn("width", elemStart, elemEnd))
                    ex.waterWidth[ex.numWater] = jr.ReadFloat();

                jr.pos = elemStart;
                if (jr.FindKeyIn("depth", elemStart, elemEnd))
                    ex.waterDepth[ex.numWater] = jr.ReadFloat();

                ex.numWater++;
                jr.pos = elemEnd + 1;
                jr.SkipWhitespace();
                if (jr.pos < jr.len && jr.buf[jr.pos] == ',') jr.pos++;
            }
        }
    }

    // ── Finish line (optional) ──
    jr.pos = 0;
    if (jr.FindKey("finish_line_x")) {
        ex.finishLineX = jr.ReadFloat();
    }

    // ── Music ──
    jr.pos = 0;
    if (jr.FindKey("music")) {
        jr.ReadString(g_customLevelMusic[slotIndex], 128);
    }

    // ── Icon texture ──
    jr.pos = 0;
    if (jr.FindKey("icon_tex")) {
        jr.ReadString(cl.iconTexPath, 128);
    }

    cl.loaded = true;
    ex.hasData = true;
    return true;
}

//============================================================================================
// LOAD R.U.B.E. (REALLY USEFUL BOX2D EDITOR) JSON FORMAT
//============================================================================================
// R.U.B.E. exports Box2D worlds as JSON with this structure:
// {
//     "gravity": { "x": 0.0, "y": -9.81 },
//     "body": [ { "name": "...", "type": 0/1/2, "position": {"x":...,"y":...},
//                  "fixture": [ { "chain": {"vertices": {"x":[...],"y":[...]}} } ],
//                  "customProperties": [ { "name": "hcr_jump_height", "float": 2.5 } ]
//               } ],
//     "image": [ { "file": "terrain.png", "customProperties": [...] } ],
//     "customProperties": [ { "name": "hcr_level_name", "string": "My Level" } ]
// }
//
// Mapping convention (custom properties on bodies/world):
//   World-level custom properties:
//     hcr_level_name    (string) → cl.name
//     hcr_amplitude     (float)  → cl.amplitude
//     hcr_frequency     (float)  → cl.frequencyBase
//     hcr_music         (string) → music path
//     hcr_terrain_tex   (string) → cl.terrainTexPath
//     hcr_bg_tex        (string) → cl.bgTexPath
//     hcr_bg2_tex       (string) → cl.bg2TexPath
//     hcr_bg3_tex       (string) → cl.bg3TexPath
//     hcr_icon_tex      (string) → cl.iconTexPath
//     hcr_spawn_x       (float)  → ex.spawnX
//     hcr_spawn_y       (float)  → ex.spawnY
//     hcr_finish_x      (float)  → ex.finishLineX
//
//   Body-level custom properties (on static bodies):
//     hcr_type = "jump"   → reads position.x as jump x, plus:
//       hcr_jump_height   (float)  → jump height
//       hcr_jump_width    (float)  → jump width
//     hcr_type = "water"  → reads position.x as water x, plus:
//       hcr_water_width   (float)  → water width
//       hcr_water_depth   (float)  → water depth
//     hcr_type = "bridge" → reads position.x as bridge start x
//       (sets cl.hasBridge = 1, cl.bridgeStartX = position.x)
//============================================================================================

// Helper: read a R.U.B.E. custom property by name from a customProperties array.
// The cursor must be positioned at the '[' of the customProperties array.
// Returns true if found, fills outFloat or outString depending on type.
static bool RubeReadCustomPropFloat(JsonReader& jr, int arrStart, int arrEnd,
                                     const char* propName, float& outVal) {
    int i = arrStart + 1; // skip '['
    while (i < arrEnd) {
        // Skip whitespace
        while (i < arrEnd && (jr.buf[i] == ' ' || jr.buf[i] == '\t' ||
               jr.buf[i] == '\r' || jr.buf[i] == '\n' || jr.buf[i] == ','))
            i++;
        if (i >= arrEnd || jr.buf[i] == ']') break;
        if (jr.buf[i] != '{') { i++; continue; }

        int objEnd = jr.FindMatchingBrace(i);

        // Check "name" matches propName
        jr.pos = i;
        if (jr.FindKeyIn("name", i, objEnd)) {
            char name[128];
            jr.ReadString(name, 128);
            if (strcmp(name, propName) == 0) {
                // Try to read "float" value
                jr.pos = i;
                if (jr.FindKeyIn("float", i, objEnd)) {
                    outVal = jr.ReadFloat();
                    return true;
                }
                // Also try "int" value (R.U.B.E. may store as int)
                jr.pos = i;
                if (jr.FindKeyIn("int", i, objEnd)) {
                    outVal = jr.ReadFloat();
                    return true;
                }
            }
        }
        i = objEnd + 1;
    }
    return false;
}

static bool RubeReadCustomPropString(JsonReader& jr, int arrStart, int arrEnd,
                                      const char* propName, char* outStr, int outSize) {
    int i = arrStart + 1;
    while (i < arrEnd) {
        while (i < arrEnd && (jr.buf[i] == ' ' || jr.buf[i] == '\t' ||
               jr.buf[i] == '\r' || jr.buf[i] == '\n' || jr.buf[i] == ','))
            i++;
        if (i >= arrEnd || jr.buf[i] == ']') break;
        if (jr.buf[i] != '{') { i++; continue; }

        int objEnd = jr.FindMatchingBrace(i);

        jr.pos = i;
        if (jr.FindKeyIn("name", i, objEnd)) {
            char name[128];
            jr.ReadString(name, 128);
            if (strcmp(name, propName) == 0) {
                jr.pos = i;
                if (jr.FindKeyIn("string", i, objEnd)) {
                    jr.ReadString(outStr, outSize);
                    return true;
                }
            }
        }
        i = objEnd + 1;
    }
    return false;
}

static bool LoadRubeJsonLevel(JsonReader& jr, int slotIndex) {
    CustomLevelDef& cl = g_customLevels[slotIndex];
    JsonLevelExtra& ex = g_jsonLevelExtra[slotIndex];

    // ── World gravity ──
    jr.pos = 0;
    if (jr.FindKey("gravity")) {
        int gStart = jr.pos;
        int gEnd   = jr.FindMatchingBrace(gStart);

        jr.pos = gStart;
        if (jr.FindKeyIn("y", gStart, gEnd))
            ex.gravityY = jr.ReadFloat();
    }

    // ── World-level custom properties ──
    jr.pos = 0;
    if (jr.FindKey("customProperties")) {
        int arrStart = jr.pos;
        int arrEnd   = jr.FindMatchingBrace(arrStart);

        RubeReadCustomPropString(jr, arrStart, arrEnd, "hcr_level_name", cl.name, 64);
        RubeReadCustomPropFloat (jr, arrStart, arrEnd, "hcr_amplitude",  cl.amplitude);
        RubeReadCustomPropFloat (jr, arrStart, arrEnd, "hcr_frequency",  cl.frequencyBase);
        RubeReadCustomPropString(jr, arrStart, arrEnd, "hcr_music",
                                 g_customLevelMusic[slotIndex], 128);
        RubeReadCustomPropString(jr, arrStart, arrEnd, "hcr_terrain_tex", cl.terrainTexPath, 128);
        RubeReadCustomPropString(jr, arrStart, arrEnd, "hcr_bg_tex",      cl.bgTexPath, 128);
        RubeReadCustomPropString(jr, arrStart, arrEnd, "hcr_bg2_tex",     cl.bg2TexPath, 128);
        RubeReadCustomPropString(jr, arrStart, arrEnd, "hcr_bg3_tex",     cl.bg3TexPath, 128);
        RubeReadCustomPropString(jr, arrStart, arrEnd, "hcr_icon_tex",    cl.iconTexPath, 128);
        RubeReadCustomPropFloat (jr, arrStart, arrEnd, "hcr_spawn_x",     ex.spawnX);
        RubeReadCustomPropFloat (jr, arrStart, arrEnd, "hcr_spawn_y",     ex.spawnY);
        RubeReadCustomPropFloat (jr, arrStart, arrEnd, "hcr_finish_x",    ex.finishLineX);
    }

    // ── Bodies: scan for jump/water/bridge markers ──
    jr.pos = 0;
    if (jr.FindKey("body")) {
        int bodyArrStart = jr.pos;
        int bodyArrEnd   = jr.FindMatchingBrace(bodyArrStart);

        int bi = bodyArrStart + 1; // skip '['
        while (bi < bodyArrEnd) {
            // Skip to next object
            while (bi < bodyArrEnd && (jr.buf[bi] == ' ' || jr.buf[bi] == '\t' ||
                   jr.buf[bi] == '\r' || jr.buf[bi] == '\n' || jr.buf[bi] == ','))
                bi++;
            if (bi >= bodyArrEnd || jr.buf[bi] == ']') break;
            if (jr.buf[bi] != '{') { bi++; continue; }

            int bodyObjEnd = jr.FindMatchingBrace(bi);

            // Read body position
            float bodyX = 0.0f, bodyY = 0.0f;
            jr.pos = bi;
            if (jr.FindKeyIn("position", bi, bodyObjEnd)) {
                int posStart = jr.pos;
                int posEnd   = jr.FindMatchingBrace(posStart);
                jr.pos = posStart;
                if (jr.FindKeyIn("x", posStart, posEnd)) bodyX = jr.ReadFloat();
                jr.pos = posStart;
                if (jr.FindKeyIn("y", posStart, posEnd)) bodyY = jr.ReadFloat();
            }

            // Check for customProperties on this body
            jr.pos = bi;
            if (jr.FindKeyIn("customProperties", bi, bodyObjEnd)) {
                int cpStart = jr.pos;
                int cpEnd   = jr.FindMatchingBrace(cpStart);

                char hcrType[64] = {0};
                RubeReadCustomPropString(jr, cpStart, cpEnd, "hcr_type", hcrType, 64);

                if (strcmp(hcrType, "jump") == 0 && ex.numJumps < MAX_JUMPS_PER_LEVEL) {
                    JumpDef& jd = ex.jumps[ex.numJumps];
                    jd.x = bodyX;
                    jd.height = 2.5f;  // defaults
                    jd.width  = 5.0f;
                    RubeReadCustomPropFloat(jr, cpStart, cpEnd, "hcr_jump_height", jd.height);
                    RubeReadCustomPropFloat(jr, cpStart, cpEnd, "hcr_jump_width",  jd.width);
                    ex.numJumps++;
                }
                else if (strcmp(hcrType, "water") == 0 && ex.numWater < MAX_WATER_REGIONS) {
                    int wi = ex.numWater;
                    ex.waterX[wi]     = bodyX;
                    ex.waterWidth[wi] = WATER_SPAN;  // default
                    ex.waterDepth[wi] = WATER_VALLEY_DEPTH;
                    RubeReadCustomPropFloat(jr, cpStart, cpEnd, "hcr_water_width", ex.waterWidth[wi]);
                    RubeReadCustomPropFloat(jr, cpStart, cpEnd, "hcr_water_depth", ex.waterDepth[wi]);
                    ex.numWater++;
                }
                else if (strcmp(hcrType, "bridge") == 0) {
                    cl.hasBridge    = 1.0f;
                    cl.bridgeStartX = bodyX;
                    // Optional override
                    RubeReadCustomPropFloat(jr, cpStart, cpEnd, "hcr_bridge_start_x", cl.bridgeStartX);
                }
                else if (strcmp(hcrType, "spawn") == 0) {
                    ex.spawnX = bodyX;
                    ex.spawnY = bodyY;
                }
            }

            bi = bodyObjEnd + 1;
        }
    }

    // ── If no name was set, use "R.U.B.E. Level" ──
    if (cl.name[0] == '\0') {
        JsonStrCopy(cl.name, 64, "RUBE Level");
    }

    cl.loaded  = true;
    ex.hasData = true;
    return true;
}

//============================================================================================
// LOAD A SINGLE JSON FILE INTO A CUSTOM LEVEL SLOT
// Detects format (HCR or R.U.B.E.) and dispatches to the appropriate loader.
//============================================================================================
static bool LoadSingleJsonLevel(const char* path, int slotIndex) {
    int fileSize = 0;
    char* data = ReadEntireFile(path, fileSize);
    if (!data) return false;

    // Initialise the extra data slot
    memset(&g_jsonLevelExtra[slotIndex], 0, sizeof(JsonLevelExtra));
    g_jsonLevelExtra[slotIndex].gravityY   = GRAVITY;    // default Earth gravity
    g_jsonLevelExtra[slotIndex].spawnX     = CAR_START_X;
    g_jsonLevelExtra[slotIndex].spawnY     = CAR_START_Y;

    JsonReader jr;
    jr.Init(data, fileSize);

    int format = DetectJsonFormat(jr);
    bool ok = false;

    if (format == 1) {
        ok = LoadHcrJsonLevel(jr, slotIndex);
        if (ok) {
            char dbg[256];
            sprintf_s(dbg, sizeof(dbg), "JsonLevel: loaded HCR format '%s' → slot %d\n",
                      g_customLevels[slotIndex].name, slotIndex);
            OutputDebugStringA(dbg);
        }
    }
    else if (format == 2) {
        ok = LoadRubeJsonLevel(jr, slotIndex);
        if (ok) {
            char dbg[256];
            sprintf_s(dbg, sizeof(dbg), "JsonLevel: loaded R.U.B.E. format '%s' → slot %d\n",
                      g_customLevels[slotIndex].name, slotIndex);
            OutputDebugStringA(dbg);
        }
    }
    else if (format == 3) {
        // Raw R.U.B.E. scene (no hcr_ markers) — not a level definition.
        // These are handled by rube_loader.cpp as sandbox scenes instead.
        char dbg[256];
        sprintf_s(dbg, sizeof(dbg),
            "JsonLevel: '%s' is a raw R.U.B.E. scene (no hcr_ markers) — "
            "use RubeLoadScene() to play it as sandbox\n", path);
        OutputDebugStringA(dbg);
    }
    else {
        char dbg[256];
        sprintf_s(dbg, sizeof(dbg), "JsonLevel: unknown format in '%s', skipping\n", path);
        OutputDebugStringA(dbg);
    }

    delete[] data;
    return ok;
}

//============================================================================================
// ScanLevelFiles — scan a directory pattern and process each file
// Returns the number of HCR/custom levels loaded (R.U.B.E. scenes are recorded separately)
//============================================================================================
static int ScanLevelFiles(const char* pattern, int& loaded) {
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(pattern, &findData);

    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        // Skip directories
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        // Build full path
        char fullPath[256];
        sprintf_s(fullPath, sizeof(fullPath), "game:\\Config\\Levels\\%s", findData.cFileName);

        // ── Peek at the file to determine format before committing a slot ──
        int peekSize = 0;
        char* peekData = ReadEntireFile(fullPath, peekSize);
        if (!peekData) continue;

        JsonReader peekJr;
        peekJr.Init(peekData, peekSize);
        int format = DetectJsonFormat(peekJr);
        delete[] peekData;

        // ── Format 3 or 4: raw R.U.B.E. scene → record path for sandbox mode ──
        if (format == 3 || format == 4) {
            if (g_numRubeScenes < MAX_RUBE_SCENES) {
                // Store full path
                int pi = 0;
                while (pi < 255 && fullPath[pi]) {
                    g_rubeScenePaths[g_numRubeScenes][pi] = fullPath[pi]; pi++;
                }
                g_rubeScenePaths[g_numRubeScenes][pi] = '\0';

                // Extract display name from filename (strip extension)
                char* fname = findData.cFileName;
                int ni = 0;
                while (ni < 63 && fname[ni] && fname[ni] != '.') {
                    g_rubeSceneNames[g_numRubeScenes][ni] = fname[ni]; ni++;
                }
                g_rubeSceneNames[g_numRubeScenes][ni] = '\0';

                char dbg[256];
                sprintf_s(dbg, sizeof(dbg), "JsonLevel: discovered R.U.B.E. scene '%s'\n",
                          g_rubeSceneNames[g_numRubeScenes]);
                OutputDebugStringA(dbg);
                g_numRubeScenes++;
            }
            continue; // don't load as a custom level
        }

        // ── Format 1 or 2: load as custom level ──
        if (format == 1 || format == 2) {
            int slotIndex = g_numCustomLevels;
            if (slotIndex >= MAX_CUSTOM_LEVELS) {
                OutputDebugStringA("JsonLevel: custom level slots full, stopping scan\n");
                break;
            }

            // Apply default values (same as LoadConfig does for cfg levels)
            memset(&g_customLevels[slotIndex], 0, sizeof(CustomLevelDef));
            g_customLevels[slotIndex].amplitude     = 15.0f;
            g_customLevels[slotIndex].frequencyBase = 0.4f;
            g_customLevels[slotIndex].bridgeStartX  = 150.0f;

            if (LoadSingleJsonLevel(fullPath, slotIndex)) {
                g_numCustomLevels++;
                g_totalLevelSlots = 7 + g_numCustomLevels;
                loaded++;
            }
        }
        // format == 0: unknown, silently skip

    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
    return loaded;
}

//============================================================================================
// LoadJsonLevels — scan game:\Config\Levels\ for *.json and *.rube files
//============================================================================================
int LoadJsonLevels() {
    // Zero-init all extra data slots (cfg-defined levels don't have JSON extras)
    for (int i = 0; i < MAX_JSON_LEVELS; i++) {
        memset(&g_jsonLevelExtra[i], 0, sizeof(JsonLevelExtra));
    }
    g_numRubeScenes = 0;

    int loaded = 0;

    // Scan *.json files
    ScanLevelFiles("game:\\Config\\Levels\\*.json", loaded);

    // Scan *.rube files (same JSON format, different extension)
    ScanLevelFiles("game:\\Config\\Levels\\*.rube", loaded);

    char dbg[256];
    sprintf_s(dbg, sizeof(dbg),
        "JsonLevel: loaded %d levels, discovered %d R.U.B.E. scenes (total custom: %d)\n",
        loaded, g_numRubeScenes, g_numCustomLevels);
    OutputDebugStringA(dbg);

    // ── Include R.U.B.E. scenes in the total level slot count ──
    // Layout: [0..6] built-in | [7..7+custom-1] custom levels | [7+custom..] R.U.B.E. scenes
    g_totalLevelSlots = 7 + g_numCustomLevels + g_numRubeScenes;

    return loaded;
}

//============================================================================================
// ApplyJsonLevelData — apply JSON extra data during LoadLevel()
//============================================================================================
// Called from LoadLevel() for custom levels.  Sets gravity, jumps, water, and spawn
// from the JsonLevelExtra struct.  Returns true if JSON data was applied.
//
// Integration point — add this call to LoadLevel() in physics.cpp after the existing
// SetLevelWater() / SetLevelJumps() calls for custom levels:
//
//   if ((int)level >= LEVEL_CUSTOM_1) {
//       int ci = (int)level - (int)LEVEL_CUSTOM_1;
//       if (ApplyJsonLevelData(ci)) {
//           // JSON level — gravity, jumps, water, spawn already set
//       }
//   }
//============================================================================================
bool ApplyJsonLevelData(int customIndex) {
    if (customIndex < 0 || customIndex >= MAX_JSON_LEVELS) return false;

    const JsonLevelExtra& ex = g_jsonLevelExtra[customIndex];
    if (!ex.hasData) return false;

    // ── Gravity ──
    if (g_pBox2DWorld != NULL) {
        g_pBox2DWorld->SetGravity(b2Vec2(0.0f, ex.gravityY));
    }

    // ── Jumps (override any existing jump data) ──
    g_numLevelJumps = ex.numJumps;
    for (int j = 0; j < ex.numJumps && j < MAX_JUMPS_PER_LEVEL; j++) {
        g_levelJumps[j] = ex.jumps[j];
    }

    // ── Water regions ──
    // Clear existing water state
    g_numWaterRegions = 0;
    for (int i = 0; i < MAX_WATER_REGIONS; i++) {
        g_waterRegions[i].active     = false;
        g_waterRegions[i].sensorBody = NULL;
    }

    // Register JSON-defined water regions
    for (int w = 0; w < ex.numWater && w < MAX_WATER_REGIONS; w++) {
        WaterRegion& wr = g_waterRegions[w];
        wr.x      = ex.waterX[w];
        wr.width  = ex.waterWidth[w];
        wr.depth  = ex.waterDepth[w];
        wr.restY  = 0.0f;   // set later by InitWaterSurface
        wr.active     = true;
        wr.sensorBody = NULL;

        int nc = (int)(wr.width / WATER_COL_WIDTH);
        if (nc > WATER_COLS_MAX) nc = WATER_COLS_MAX;
        if (nc < 2) nc = 2;
        wr.numCols = nc;
        for (int c = 0; c < nc; c++) {
            wr.cols[c].h   = 0.0f;
            wr.cols[c].vel = 0.0f;
        }
        g_numWaterRegions++;
    }

    // ── Spawn point ──
    // Applied later in LoadLevel when positioning the car chassis/wheels.
    // The caller should read ex.spawnX / ex.spawnY after this returns true.

    return true;
}

//============================================================================================
// LoadRubeSceneIcons — load icon textures for R.U.B.E. sandbox scenes.
// For each scene, checks for game:\Config\Levels\<scenename>_icon.png
// Must be called AFTER D3D device is created.
//============================================================================================
void LoadRubeSceneIcons() {
    for (int i = 0; i < g_numRubeScenes; i++) {
        g_rubeSceneIcons[i] = NULL;

        // Build icon path: game:\Config\Levels\<name>_icon.png
        char iconPath[256];
        sprintf_s(iconPath, sizeof(iconPath),
                  "game:\\Config\\Levels\\%s_icon.png", g_rubeSceneNames[i]);

        if (SUCCEEDED(D3DXCreateTextureFromFileA(g_pd3dDevice, iconPath, &g_rubeSceneIcons[i]))) {
            char dbg[256];
            sprintf_s(dbg, sizeof(dbg), "JsonLevel: loaded icon for R.U.B.E. scene '%s'\n",
                      g_rubeSceneNames[i]);
            OutputDebugStringA(dbg);
        }
    }
}
