#include "stdafx.h"
#include "rube_loader.h"
#include "json_level.h"   // for JsonReader

//============================================================================================
// GLOBAL SCENE STATE
//============================================================================================
RubeScene g_rubeScene;

//============================================================================================
// HELPER: read file into heap buffer
//============================================================================================
static char* RubeReadFile(const char* path, int& outSize) {
    outSize = 0;
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize > 2 * 1024 * 1024) { // 2 MB max
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
// HELPER: safe string copy
//============================================================================================
static void RubeStrCopy(char* dst, int dstSize, const char* src) {
    int i = 0;
    while (i < dstSize - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

//============================================================================================
// HELPER: read R.U.B.E. {x, y} vec2 objects
// R.U.B.E. encodes vec2 as either:
//   { "x": 1.0, "y": 2.0 }  (normal)
//   0                         (shorthand for {0,0})
//============================================================================================
static void ReadVec2(JsonReader& jr, float& outX, float& outY) {
    jr.SkipWhitespace();
    if (jr.pos < jr.len && (jr.buf[jr.pos] == '-' ||
        (jr.buf[jr.pos] >= '0' && jr.buf[jr.pos] <= '9'))) {
        // Scalar shorthand: both components are 0 (R.U.B.E. uses "0" for zero vectors)
        jr.ReadFloat();
        outX = 0.0f;
        outY = 0.0f;
        return;
    }
    // Object form: {"x": ..., "y": ...}
    int objStart = jr.pos;
    int objEnd   = jr.FindMatchingBrace(objStart);

    jr.pos = objStart;
    if (jr.FindKeyIn("x", objStart, objEnd)) outX = jr.ReadFloat();
    else outX = 0.0f;

    jr.pos = objStart;
    if (jr.FindKeyIn("y", objStart, objEnd)) outY = jr.ReadFloat();
    else outY = 0.0f;

    jr.pos = objEnd + 1;
}

//============================================================================================
// HELPER: read a JSON number array into a float buffer
// e.g. [ 1.0, 2.0, 3.0 ] → fills dst[0..2], returns 3
//============================================================================================
static int ReadFloatArray(JsonReader& jr, float* dst, int maxCount) {
    jr.SkipWhitespace();
    if (jr.pos >= jr.len || jr.buf[jr.pos] != '[') return 0;
    jr.pos++; // skip '['
    jr.SkipWhitespace();

    int count = 0;
    while (jr.pos < jr.len && jr.buf[jr.pos] != ']' && count < maxCount) {
        jr.SkipWhitespace();
        dst[count++] = jr.ReadFloat();
        jr.SkipWhitespace();
        if (jr.pos < jr.len && jr.buf[jr.pos] == ',') jr.pos++;
    }
    if (jr.pos < jr.len && jr.buf[jr.pos] == ']') jr.pos++;
    return count;
}

//============================================================================================
// HELPER: find body array index by R.U.B.E. body ID (for native .rube format)
//============================================================================================
static int FindBodyIndexById(RubeScene& scene, int bodyId) {
    for (int i = 0; i < scene.numBodies; i++) {
        if (scene.bodyIds[i] == bodyId) return i;
    }
    return -1;
}

//============================================================================================
// HELPER: parse fixture shapes from a fixture object (shared between formats)
// The caller must position jr.pos at the fixture object start.
// isNative: true = .rube format (shapes array + vertices at fixture level)
//           false = .json format (polygon/circle keys with nested vertices)
//============================================================================================
static void ParseFixture(JsonReader& jr, int fStart, int fEnd, b2Body* body) {
    float density     = 0.0f;
    float friction    = 0.2f;
    float restitution = 0.0f;
    int   groupIndex  = 0;
    int   categoryBits = 0x0001;
    int   maskBits     = 0xFFFF;
    bool  isSensor    = false;

    jr.pos = fStart;
    if (jr.FindKeyIn("density", fStart, fEnd)) density = jr.ReadFloat();
    jr.pos = fStart;
    if (jr.FindKeyIn("friction", fStart, fEnd)) friction = jr.ReadFloat();
    jr.pos = fStart;
    if (jr.FindKeyIn("restitution", fStart, fEnd)) restitution = jr.ReadFloat();
    jr.pos = fStart;
    if (jr.FindKeyIn("filter-groupIndex", fStart, fEnd)) groupIndex = jr.ReadInt();
    jr.pos = fStart;
    if (jr.FindKeyIn("filter-categoryBits", fStart, fEnd)) categoryBits = jr.ReadInt();
    jr.pos = fStart;
    if (jr.FindKeyIn("filter-maskBits", fStart, fEnd)) maskBits = jr.ReadInt();
    jr.pos = fStart;
    if (jr.FindKeyIn("sensor", fStart, fEnd)) isSensor = jr.ReadBool();

    // ── Determine shape type ──
    // .rube native: "shapes": [{"type":"polygon"}] or [{"type":"circle","radius":0.5}]
    // .json export: has "polygon" or "circle" key directly
    bool isPolygon = false;
    bool isCircle  = false;
    float circleRadius = 0.5f;

    // Check for native format "shapes" array
    jr.pos = fStart;
    if (jr.FindKeyIn("shapes", fStart, fEnd)) {
        int shapesStart = jr.pos;
        int shapesEnd   = jr.FindMatchingBrace(shapesStart);
        // Look for "type" inside first element
        jr.pos = shapesStart;
        if (jr.FindKeyIn("type", shapesStart, shapesEnd)) {
            char shapeType[32] = {0};
            jr.ReadString(shapeType, 32);
            if (strcmp(shapeType, "polygon") == 0) isPolygon = true;
            else if (strcmp(shapeType, "circle") == 0) {
                isCircle = true;
                jr.pos = shapesStart;
                if (jr.FindKeyIn("radius", shapesStart, shapesEnd))
                    circleRadius = jr.ReadFloat();
            }
        }
    }

    // Check for .json export format (no "shapes" key — polygon/circle keys directly)
    if (!isPolygon && !isCircle) {
        jr.pos = fStart;
        if (jr.FindKeyIn("polygon", fStart, fEnd)) isPolygon = true;
    }
    if (!isPolygon && !isCircle) {
        jr.pos = fStart;
        if (jr.FindKeyIn("circle", fStart, fEnd)) isCircle = true;
    }

    // ── Read vertices ──
    // Native format: vertices at fixture level
    // Export format: vertices nested inside polygon object
    float xCoords[RUBE_MAX_VERTS];
    float yCoords[RUBE_MAX_VERTS];
    int numX = 0, numY = 0;

    if (isPolygon) {
        // Try fixture-level vertices first (native format)
        jr.pos = fStart;
        if (jr.FindKeyIn("vertices", fStart, fEnd)) {
            int vertsStart = jr.pos;
            int vertsEnd   = jr.FindMatchingBrace(vertsStart);
            jr.pos = vertsStart;
            if (jr.FindKeyIn("x", vertsStart, vertsEnd))
                numX = ReadFloatArray(jr, xCoords, RUBE_MAX_VERTS);
            jr.pos = vertsStart;
            if (jr.FindKeyIn("y", vertsStart, vertsEnd))
                numY = ReadFloatArray(jr, yCoords, RUBE_MAX_VERTS);
        }

        // If no vertices found, try export format (vertices inside polygon object)
        if (numX == 0) {
            jr.pos = fStart;
            if (jr.FindKeyIn("polygon", fStart, fEnd)) {
                int polyStart = jr.pos;
                int polyEnd   = jr.FindMatchingBrace(polyStart);
                jr.pos = polyStart;
                if (jr.FindKeyIn("vertices", polyStart, polyEnd)) {
                    int vertsStart = jr.pos;
                    int vertsEnd   = jr.FindMatchingBrace(vertsStart);
                    jr.pos = vertsStart;
                    if (jr.FindKeyIn("x", vertsStart, vertsEnd))
                        numX = ReadFloatArray(jr, xCoords, RUBE_MAX_VERTS);
                    jr.pos = vertsStart;
                    if (jr.FindKeyIn("y", vertsStart, vertsEnd))
                        numY = ReadFloatArray(jr, yCoords, RUBE_MAX_VERTS);
                }
            }
        }

        int numVerts = (numX < numY) ? numX : numY;
        if (numVerts > b2_maxPolygonVertices) numVerts = b2_maxPolygonVertices;
        if (numVerts >= 3) {
            b2Vec2 verts[RUBE_MAX_VERTS];
            for (int vi = 0; vi < numVerts; vi++)
                verts[vi].Set(xCoords[vi], yCoords[vi]);
            b2PolygonShape shape;
            shape.Set(verts, numVerts);
            b2FixtureDef fd;
            fd.shape = &shape; fd.density = density; fd.friction = friction;
            fd.restitution = restitution; fd.isSensor = isSensor;
            fd.filter.groupIndex = (int16)groupIndex;
            fd.filter.categoryBits = (uint16)categoryBits;
            fd.filter.maskBits = (uint16)maskBits;
            body->CreateFixture(&fd);
        }
    }
    else if (isCircle) {
        float cx = 0, cy = 0;

        // Native format: center from vertices x[0]/y[0], radius from shapes
        jr.pos = fStart;
        if (jr.FindKeyIn("vertices", fStart, fEnd)) {
            int vertsStart = jr.pos;
            int vertsEnd   = jr.FindMatchingBrace(vertsStart);
            float tmpX[1], tmpY[1];
            jr.pos = vertsStart;
            if (jr.FindKeyIn("x", vertsStart, vertsEnd))
                if (ReadFloatArray(jr, tmpX, 1) > 0) cx = tmpX[0];
            jr.pos = vertsStart;
            if (jr.FindKeyIn("y", vertsStart, vertsEnd))
                if (ReadFloatArray(jr, tmpY, 1) > 0) cy = tmpY[0];
        }

        // Export format: radius/center from circle object
        jr.pos = fStart;
        if (jr.FindKeyIn("circle", fStart, fEnd)) {
            int circStart = jr.pos;
            int circEnd   = jr.FindMatchingBrace(circStart);
            jr.pos = circStart;
            if (jr.FindKeyIn("radius", circStart, circEnd))
                circleRadius = jr.ReadFloat();
            jr.pos = circStart;
            if (jr.FindKeyIn("center", circStart, circEnd))
                ReadVec2(jr, cx, cy);
        }

        b2CircleShape shape;
        shape.m_radius = circleRadius;
        shape.m_p.Set(cx, cy);
        b2FixtureDef fd;
        fd.shape = &shape; fd.density = density; fd.friction = friction;
        fd.restitution = restitution; fd.isSensor = isSensor;
        fd.filter.groupIndex = (int16)groupIndex;
        fd.filter.categoryBits = (uint16)categoryBits;
        fd.filter.maskBits = (uint16)maskBits;
        body->CreateFixture(&fd);
    }
}

//============================================================================================
// PARSE ALL BODIES (unified — handles both .json export and .rube native format)
// bodyKey: "body" for .json export, "metabody" for .rube native
//============================================================================================
static int ParseBodiesUnified(JsonReader& jr, RubeScene& scene, const char* bodyKey) {
    jr.pos = 0;
    if (!jr.FindKey(bodyKey)) return 0;
    if (!jr.EnterArray()) return 0;

    int count = 0;
    while (count < RUBE_MAX_BODIES) {
        jr.SkipWhitespace();
        if (jr.pos >= jr.len || jr.buf[jr.pos] == ']') break;

        int bodyStart = jr.pos;
        int bodyEnd   = jr.FindMatchingBrace(bodyStart);

        // ── Read body properties ──
        float posX = 0, posY = 0, angle = 0;
        float velX = 0, velY = 0, angVel = 0;
        float linDamp = 0, angDamp = 0, gravScale = 1.0f;
        int   bodyType = 0;
        int   bodyId   = count;  // default to index if no ID
        char  name[64] = {0};
        bool  awake = true, fixedRot = false, bullet = false;

        jr.pos = bodyStart;
        if (jr.FindKeyIn("position", bodyStart, bodyEnd)) ReadVec2(jr, posX, posY);
        jr.pos = bodyStart;
        if (jr.FindKeyIn("angle", bodyStart, bodyEnd)) angle = jr.ReadFloat();
        jr.pos = bodyStart;
        if (jr.FindKeyIn("linearVelocity", bodyStart, bodyEnd)) ReadVec2(jr, velX, velY);
        jr.pos = bodyStart;
        if (jr.FindKeyIn("angularVelocity", bodyStart, bodyEnd)) angVel = jr.ReadFloat();

        // ── Body type: scan BACKWARDS from bodyEnd to find the LAST "type" key ──
        // This avoids hitting fixture→shapes→"type":"polygon" which appears earlier.
        // The body-level "type" is always the last or near-last key in both formats.
        {
            // Scan backwards for the pattern "type" (with quotes)
            for (int si = bodyEnd - 5; si > bodyStart; si--) {
                if (jr.buf[si] == 't' && jr.buf[si+1] == 'y' &&
                    jr.buf[si+2] == 'p' && jr.buf[si+3] == 'e' &&
                    si > 0 && jr.buf[si-1] == '"') {
                    // Found "type — now find the value after the colon
                    int vi = si + 5; // skip past 'e"'
                    while (vi < bodyEnd && (jr.buf[vi] == ' ' || jr.buf[vi] == '\t' ||
                           jr.buf[vi] == '"' || jr.buf[vi] == ':' ||
                           jr.buf[vi] == '\r' || jr.buf[vi] == '\n')) vi++;
                    if (vi < bodyEnd) {
                        // String values: static, dynamic, kinematic
                        if (jr.buf[vi] == 's') bodyType = 0;      // "static"
                        else if (jr.buf[vi] == 'k') bodyType = 1;  // "kinematic"
                        else if (jr.buf[vi] == 'd') bodyType = 2;  // "dynamic"
                        // Integer values: 0, 1, 2
                        else if (jr.buf[vi] == '0') bodyType = 0;
                        else if (jr.buf[vi] == '1') bodyType = 1;
                        else bodyType = 2;
                    }
                    break;
                }
            }
        }
        jr.pos = bodyStart;
        if (jr.FindKeyIn("name", bodyStart, bodyEnd)) jr.ReadString(name, 64);
        jr.pos = bodyStart;
        if (jr.FindKeyIn("id", bodyStart, bodyEnd)) bodyId = jr.ReadInt();
        jr.pos = bodyStart;
        if (jr.FindKeyIn("awake", bodyStart, bodyEnd)) awake = jr.ReadBool();
        jr.pos = bodyStart;
        if (jr.FindKeyIn("linearDamping", bodyStart, bodyEnd)) linDamp = jr.ReadFloat();
        jr.pos = bodyStart;
        if (jr.FindKeyIn("angularDamping", bodyStart, bodyEnd)) angDamp = jr.ReadFloat();
        jr.pos = bodyStart;
        if (jr.FindKeyIn("gravityScale", bodyStart, bodyEnd)) gravScale = jr.ReadFloat();
        jr.pos = bodyStart;
        if (jr.FindKeyIn("fixedRotation", bodyStart, bodyEnd)) fixedRot = jr.ReadBool();
        jr.pos = bodyStart;
        if (jr.FindKeyIn("bullet", bodyStart, bodyEnd)) bullet = jr.ReadBool();

        // ── Create Box2D body ──
        b2BodyDef bd;
        if      (bodyType == 0) bd.type = b2_staticBody;
        else if (bodyType == 1) bd.type = b2_kinematicBody;
        else                    bd.type = b2_dynamicBody;
        bd.position.Set(posX, posY);
        bd.angle           = angle;
        bd.linearVelocity.Set(velX, velY);
        bd.angularVelocity = angVel;
        bd.awake           = awake;
        bd.linearDamping   = linDamp;
        bd.angularDamping  = angDamp;
        bd.gravityScale    = gravScale;
        bd.fixedRotation   = fixedRot;
        bd.bullet          = bullet;

        b2Body* body = scene.world->CreateBody(&bd);
        scene.bodies[count]    = body;
        scene.bodyTypes[count] = bodyType;
        scene.bodyIds[count]   = bodyId;
        RubeStrCopy(scene.bodyNames[count], 64, name);

        // ── Track scene bounds ──
        if (posX < scene.minX) scene.minX = posX;
        if (posX > scene.maxX) scene.maxX = posX;
        if (posY < scene.minY) scene.minY = posY;
        if (posY > scene.maxY) scene.maxY = posY;

        // ── Parse fixtures (shared handler for both formats) ──
        jr.pos = bodyStart;
        if (jr.FindKeyIn("fixture", bodyStart, bodyEnd)) {
            int fixArrStart = jr.pos;
            int fixArrEnd   = jr.FindMatchingBrace(fixArrStart);
            int fi = fixArrStart + 1;

            while (fi < fixArrEnd) {
                while (fi < fixArrEnd && (jr.buf[fi] == ' ' || jr.buf[fi] == '\t' ||
                       jr.buf[fi] == '\r' || jr.buf[fi] == '\n' || jr.buf[fi] == ','))
                    fi++;
                if (fi >= fixArrEnd || jr.buf[fi] == ']') break;
                if (jr.buf[fi] != '{') { fi++; continue; }

                int fStart = fi;
                int fEnd   = jr.FindMatchingBrace(fStart);

                ParseFixture(jr, fStart, fEnd, body);

                fi = fEnd + 1;
            }
        }

        // ── Override mass data with values from JSON ────────────────────
        // R.U.B.E. exports massData-mass, massData-center, massData-I for
        // each body.  Without this call, Box2D uses auto-calculated mass
        // from fixture densities, which may differ from what the scene
        // author intended.  This is critical for vehicle stability.
        if (bodyType == 2) { // dynamic bodies only
            b2MassData massData;
            float massCx = 0, massCy = 0;
            massData.mass = 0;
            massData.I    = 0;

            jr.pos = bodyStart;
            if (jr.FindKeyIn("massData-mass", bodyStart, bodyEnd))
                massData.mass = jr.ReadFloat();
            jr.pos = bodyStart;
            if (jr.FindKeyIn("massData-center", bodyStart, bodyEnd))
                ReadVec2(jr, massCx, massCy);
            jr.pos = bodyStart;
            if (jr.FindKeyIn("massData-I", bodyStart, bodyEnd))
                massData.I = jr.ReadFloat();

            massData.center.Set(massCx, massCy);
            if (massData.mass > 0)
                body->SetMassData(&massData);
        }

        count++;
        jr.pos = bodyEnd + 1;
        jr.SkipWhitespace();
        if (jr.pos < jr.len && jr.buf[jr.pos] == ',') jr.pos++;
    }

    scene.numBodies = count;
    return count;
}

//============================================================================================
// PARSE ALL JOINTS
// R.U.B.E. joints reference bodies by their array index in the "body" array.
//============================================================================================
static int ParseJoints(JsonReader& jr, RubeScene& scene, const char* jointKey, bool useBodyIds) {
    jr.pos = 0;
    if (!jr.FindKey(jointKey)) return 0;
    if (!jr.EnterArray()) return 0;

    int count = 0;
    while (count < RUBE_MAX_JOINTS) {
        jr.SkipWhitespace();
        if (jr.pos >= jr.len || jr.buf[jr.pos] == ']') break;
        if (jr.buf[jr.pos] == ',') { jr.pos++; jr.SkipWhitespace(); }
        if (jr.pos >= jr.len || jr.buf[jr.pos] == ']') break;
        if (jr.buf[jr.pos] != '{') { jr.pos++; continue; }

        int jStart = jr.pos;
        int jEnd   = jr.FindMatchingBrace(jStart);

        // Read common joint properties
        char jType[32] = {0};
        int  bodyAIdx = -1, bodyBIdx = -1;
        float anchorAx = 0, anchorAy = 0;
        float anchorBx = 0, anchorBy = 0;

        jr.pos = jStart;
        if (jr.FindKeyIn("type", jStart, jEnd)) jr.ReadString(jType, 32);
        jr.pos = jStart;
        if (jr.FindKeyIn("bodyA", jStart, jEnd)) {
            int rawA = jr.ReadInt();
            bodyAIdx = useBodyIds ? FindBodyIndexById(scene, rawA) : rawA;
        }
        jr.pos = jStart;
        if (jr.FindKeyIn("bodyB", jStart, jEnd)) {
            int rawB = jr.ReadInt();
            bodyBIdx = useBodyIds ? FindBodyIndexById(scene, rawB) : rawB;
        }
        jr.pos = jStart;
        if (jr.FindKeyIn("anchorA", jStart, jEnd)) ReadVec2(jr, anchorAx, anchorAy);
        jr.pos = jStart;
        if (jr.FindKeyIn("anchorB", jStart, jEnd)) ReadVec2(jr, anchorBx, anchorBy);

        // Validate body indices
        if (bodyAIdx < 0 || bodyAIdx >= scene.numBodies ||
            bodyBIdx < 0 || bodyBIdx >= scene.numBodies) {
            jr.pos = jEnd + 1;
            continue;
        }

        b2Body* bodyA = scene.bodies[bodyAIdx];
        b2Body* bodyB = scene.bodies[bodyBIdx];
        if (!bodyA || !bodyB) { jr.pos = jEnd + 1; continue; }

        // ── Wheel joint ──
        if (strcmp(jType, "wheel") == 0) {
            float localAxisAx = 0, localAxisAy = 1;
            bool  enableMotor = false;
            float motorSpeed  = 0, maxMotorTorque = 0;
            float springFreq  = 2.0f, springDamping = 0.7f;

            jr.pos = jStart;
            if (jr.FindKeyIn("localAxisA", jStart, jEnd))
                ReadVec2(jr, localAxisAx, localAxisAy);
            jr.pos = jStart;
            if (jr.FindKeyIn("enableMotor", jStart, jEnd))
                enableMotor = jr.ReadBool();
            jr.pos = jStart;
            if (jr.FindKeyIn("motorSpeed", jStart, jEnd))
                motorSpeed = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("maxMotorTorque", jStart, jEnd))
                maxMotorTorque = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("springFrequency", jStart, jEnd))
                springFreq = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("springDampingRatio", jStart, jEnd))
                springDamping = jr.ReadFloat();

            b2WheelJointDef wjd;
            // Set fields directly — DO NOT use Initialize() because it
            // double-transforms localAxisA (JSON already has body-local axis,
            // Initialize calls GetLocalVector which rotates it again).
            wjd.bodyA = bodyA;
            wjd.bodyB = bodyB;
            wjd.localAnchorA.Set(anchorAx, anchorAy);
            wjd.localAnchorB.Set(anchorBx, anchorBy);
            wjd.localAxisA.Set(localAxisAx, localAxisAy);
            wjd.enableMotor     = enableMotor;
            wjd.motorSpeed      = motorSpeed;
            wjd.maxMotorTorque  = maxMotorTorque;
            wjd.frequencyHz     = springFreq;
            wjd.dampingRatio    = springDamping;

            b2WheelJoint* joint = (b2WheelJoint*)scene.world->CreateJoint(&wjd);

            // If this wheel joint has motor capacity AND is attached to the player,
            // track it. Many R.U.B.E. scenes export with enableMotor=false but
            // maxMotorTorque > 0, expecting the game to enable motors at runtime.
            if (maxMotorTorque > 0 &&
                (bodyAIdx == scene.playerBodyIdx || bodyBIdx == scene.playerBodyIdx)) {
                if (scene.numMotorJoints < 16) {
                    scene.motorJoints[scene.numMotorJoints].joint     = joint;
                    scene.motorJoints[scene.numMotorJoints].jointType = 0; // wheel
                    scene.motorJoints[scene.numMotorJoints].maxTorque = maxMotorTorque;
                    scene.numMotorJoints++;
                }
            }
        }

        // ── Revolute joint ──
        else if (strcmp(jType, "revolute") == 0) {
            bool  enableLimit = false, enableMotor = false;
            float lowerLimit = 0, upperLimit = 0;
            float motorSpeed = 0, maxMotorTorque = 0;
            float refAngle   = 0;

            jr.pos = jStart;
            if (jr.FindKeyIn("enableLimit", jStart, jEnd))
                enableLimit = jr.ReadBool();
            jr.pos = jStart;
            if (jr.FindKeyIn("enableMotor", jStart, jEnd))
                enableMotor = jr.ReadBool();
            jr.pos = jStart;
            if (jr.FindKeyIn("lowerLimit", jStart, jEnd))
                lowerLimit = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("upperLimit", jStart, jEnd))
                upperLimit = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("motorSpeed", jStart, jEnd))
                motorSpeed = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("maxMotorTorque", jStart, jEnd))
                maxMotorTorque = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("refAngle", jStart, jEnd) || jr.FindKeyIn("referenceAngle", jStart, jEnd))
                refAngle = jr.ReadFloat();

            b2RevoluteJointDef rjd;
            rjd.bodyA = bodyA;
            rjd.bodyB = bodyB;
            rjd.localAnchorA.Set(anchorAx, anchorAy);
            rjd.localAnchorB.Set(anchorBx, anchorBy);
            rjd.enableLimit      = enableLimit;
            rjd.lowerAngle       = lowerLimit;
            rjd.upperAngle       = upperLimit;
            rjd.enableMotor      = enableMotor;
            rjd.motorSpeed       = motorSpeed;
            rjd.maxMotorTorque   = maxMotorTorque;
            rjd.referenceAngle   = refAngle;

            b2Joint* joint = scene.world->CreateJoint(&rjd);

            // Track motorised revolute joints on the player body
            // Same as wheel joints: capture any with torque capacity regardless of enableMotor
            if (maxMotorTorque > 0 &&
                (bodyAIdx == scene.playerBodyIdx || bodyBIdx == scene.playerBodyIdx)) {
                if (scene.numMotorJoints < 16) {
                    scene.motorJoints[scene.numMotorJoints].joint     = joint;
                    scene.motorJoints[scene.numMotorJoints].jointType = 1; // revolute
                    scene.motorJoints[scene.numMotorJoints].maxTorque = maxMotorTorque;
                    scene.numMotorJoints++;
                }
            }
        }

        // ── Distance joint (walker leg linkages, spring connections) ──
        else if (strcmp(jType, "distance") == 0) {
            float length   = 1.0f;
            float frequency = 0.0f;
            float damping   = 0.0f;

            jr.pos = jStart;
            if (jr.FindKeyIn("length", jStart, jEnd))
                length = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("frequency", jStart, jEnd))
                frequency = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("dampingRatio", jStart, jEnd))
                damping = jr.ReadFloat();

            b2DistanceJointDef djd;
            djd.bodyA = bodyA;
            djd.bodyB = bodyB;
            djd.localAnchorA.Set(anchorAx, anchorAy);
            djd.localAnchorB.Set(anchorBx, anchorBy);
            djd.length        = length;
            djd.frequencyHz   = frequency;
            djd.dampingRatio  = damping;

            scene.world->CreateJoint(&djd);
        }

        // ── Prismatic joint (sliders, pistons, walker leg guides) ──
        else if (strcmp(jType, "prismatic") == 0) {
            float localAxisAx = 1, localAxisAy = 0;
            bool  enableLimit = false, enableMotor = false;
            float lowerLimit = 0, upperLimit = 0;
            float motorSpeed = 0, maxMotorForce = 0;
            float refAngle   = 0;

            jr.pos = jStart;
            if (jr.FindKeyIn("localAxisA", jStart, jEnd))
                ReadVec2(jr, localAxisAx, localAxisAy);
            jr.pos = jStart;
            if (jr.FindKeyIn("enableLimit", jStart, jEnd))
                enableLimit = jr.ReadBool();
            jr.pos = jStart;
            if (jr.FindKeyIn("enableMotor", jStart, jEnd))
                enableMotor = jr.ReadBool();
            jr.pos = jStart;
            if (jr.FindKeyIn("lowerLimit", jStart, jEnd))
                lowerLimit = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("upperLimit", jStart, jEnd))
                upperLimit = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("motorSpeed", jStart, jEnd))
                motorSpeed = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("maxMotorForce", jStart, jEnd))
                maxMotorForce = jr.ReadFloat();
            jr.pos = jStart;
            if (jr.FindKeyIn("refAngle", jStart, jEnd) || jr.FindKeyIn("referenceAngle", jStart, jEnd))
                refAngle = jr.ReadFloat();

            b2PrismaticJointDef pjd;
            pjd.bodyA = bodyA;
            pjd.bodyB = bodyB;
            pjd.localAnchorA.Set(anchorAx, anchorAy);
            pjd.localAnchorB.Set(anchorBx, anchorBy);
            pjd.localAxisA.Set(localAxisAx, localAxisAy);
            pjd.enableLimit     = enableLimit;
            pjd.lowerTranslation = lowerLimit;
            pjd.upperTranslation = upperLimit;
            pjd.enableMotor     = enableMotor;
            pjd.motorSpeed      = motorSpeed;
            pjd.maxMotorForce   = maxMotorForce;
            pjd.referenceAngle  = refAngle;

            scene.world->CreateJoint(&pjd);
        }

        count++;
        jr.pos = jEnd + 1;
    }

    return count;
}

//============================================================================================
// PARSE IMAGES
// Reads the image array (metaimage for .rube, image for .json export)
// and resolves body references. Textures are loaded separately after.
//============================================================================================
static int ParseImages(JsonReader& jr, RubeScene& scene, const char* imageKey, bool useBodyIds) {
    jr.pos = 0;
    if (!jr.FindKey(imageKey)) return 0;
    if (!jr.EnterArray()) return 0;

    int count = 0;
    while (count < RUBE_MAX_IMAGES) {
        jr.SkipWhitespace();
        if (jr.pos >= jr.len || jr.buf[jr.pos] == ']') break;
        if (jr.buf[jr.pos] != '{') { jr.pos++; continue; }

        int iStart = jr.pos;
        int iEnd   = jr.FindMatchingBrace(iStart);

        RubeImage& img = scene.images[count];
        memset(&img, 0, sizeof(RubeImage));
        img.bodyId  = -1;
        img.bodyIdx = -1;
        img.opacity = 1.0f;
        img.scale   = 1.0f;

        jr.pos = iStart;
        if (jr.FindKeyIn("file", iStart, iEnd))
            jr.ReadString(img.file, 128);
        jr.pos = iStart;
        if (jr.FindKeyIn("body", iStart, iEnd))
            img.bodyId = jr.ReadInt();
        jr.pos = iStart;
        if (jr.FindKeyIn("center", iStart, iEnd)) {
            jr.SkipWhitespace();
            if (jr.buf[jr.pos] == '{')
                ReadVec2(jr, img.centerX, img.centerY);
            // else center=0 means (0,0)
        }
        jr.pos = iStart;
        if (jr.FindKeyIn("angle", iStart, iEnd))
            img.angle = jr.ReadFloat() * 0.01745329f;  // degrees → radians
        jr.pos = iStart;
        if (jr.FindKeyIn("scale", iStart, iEnd))
            img.scale = jr.ReadFloat();
        jr.pos = iStart;
        if (jr.FindKeyIn("opacity", iStart, iEnd))
            img.opacity = jr.ReadFloat();
        jr.pos = iStart;
        if (jr.FindKeyIn("renderOrder", iStart, iEnd))
            img.renderOrder = jr.ReadFloat();

        // Resolve body reference
        if (img.bodyId >= 0) {
            if (useBodyIds)
                img.bodyIdx = FindBodyIndexById(scene, img.bodyId);
            else
                img.bodyIdx = img.bodyId;  // .json export uses array index
        }

        count++;
        jr.pos = iEnd + 1;
    }

    scene.numImages = count;
    return count;
}

//============================================================================================
// LOAD TEXTURES for R.U.B.E. images
// Resolves paths relative to the scene file's directory.
// e.g. scene at "game:\Config\Levels\tank.rube" + image "images/tankbody.bmp"
//   → "game:\Config\Levels\images\tankbody.bmp"
//============================================================================================
static void LoadRubeTextures(RubeScene& scene, const char* scenePath) {
    // Extract directory from scene path
    char dir[256] = {0};
    int lastSlash = -1;
    for (int i = 0; scenePath[i]; i++) {
        if (scenePath[i] == '\\' || scenePath[i] == '/') lastSlash = i;
    }
    if (lastSlash >= 0) {
        for (int i = 0; i <= lastSlash && i < 255; i++) dir[i] = scenePath[i];
        dir[lastSlash + 1] = '\0';
    }

    for (int i = 0; i < scene.numImages; i++) {
        RubeImage& img = scene.images[i];
        if (img.file[0] == '\0') continue;

        // Convert forward slashes to backslashes
        char cleanFile[128];
        int cleanLen = 0;
        for (int ci = 0; ci < 127 && img.file[ci]; ci++) {
            cleanFile[ci] = (img.file[ci] == '/') ? '\\' : img.file[ci];
            cleanLen = ci + 1;
        }
        cleanFile[cleanLen] = '\0';

        // Build full path: dir + relative file path
        char fullPath[256];
        sprintf_s(fullPath, sizeof(fullPath), "%s%s", dir, cleanFile);

        // Try loading as-is first
        D3DXIMAGE_INFO info;
        bool loaded = false;
        if (SUCCEEDED(D3DXCreateTextureFromFileExA(
                g_pd3dDevice, fullPath,
                D3DX_DEFAULT, D3DX_DEFAULT, 1, 0,
                D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                D3DX_DEFAULT, D3DX_DEFAULT, 0,
                &info, NULL, &img.texture))) {
            loaded = true;
        }

        // If failed, try swapping extension to .png
        if (!loaded) {
            // Find last dot in fullPath
            int dotPos = -1;
            for (int di = 0; fullPath[di]; di++) {
                if (fullPath[di] == '.') dotPos = di;
            }
            if (dotPos >= 0) {
                fullPath[dotPos] = '\0';
                char pngPath[256];
                sprintf_s(pngPath, sizeof(pngPath), "%s.png", fullPath);
                fullPath[dotPos] = '.'; // restore for debug output
                if (SUCCEEDED(D3DXCreateTextureFromFileExA(
                        g_pd3dDevice, pngPath,
                        D3DX_DEFAULT, D3DX_DEFAULT, 1, 0,
                        D3DFMT_UNKNOWN, D3DPOOL_MANAGED,
                        D3DX_DEFAULT, D3DX_DEFAULT, 0,
                        &info, NULL, &img.texture))) {
                    loaded = true;
                    // Update fullPath for debug output
                    int ppi = 0;
                    while (pngPath[ppi]) { fullPath[ppi] = pngPath[ppi]; ppi++; }
                    fullPath[ppi] = '\0';
                }
            }
        }

        if (loaded) {
            img.texW = info.Width;
            img.texH = info.Height;
            char dbg[256];
            sprintf_s(dbg, sizeof(dbg), "RUBE: loaded texture '%s' (%dx%d)\n",
                      fullPath, img.texW, img.texH);
            OutputDebugStringA(dbg);
        } else {
            img.texture = NULL;
            img.texW = 1;
            img.texH = 1;
        }
    }
}

//============================================================================================
// SORT IMAGES by renderOrder (simple insertion sort — max 128 images)
//============================================================================================
static void SortImagesByRenderOrder(RubeScene& scene) {
    for (int i = 1; i < scene.numImages; i++) {
        RubeImage tmp = scene.images[i];
        int j = i - 1;
        while (j >= 0 && scene.images[j].renderOrder > tmp.renderOrder) {
            scene.images[j + 1] = scene.images[j];
            j--;
        }
        scene.images[j + 1] = tmp;
    }
}

//============================================================================================
// IDENTIFY PLAYER BODY
// Strategy: 
//   1. Look for body named "truckchassis", "chassis", "car", or "player"
//   2. Fallback: the dynamic body with the highest mass
//============================================================================================
static void IdentifyPlayerBody(RubeScene& scene) {
    scene.playerBodyIdx = -1;
    scene.playerBody    = NULL;

    // Pass 1: name match
    const char* playerNames[] = {
        "truckchassis", "chassis", "carchassis", "car", "player",
        "vehicle", "hull", "carbody", "truck",
        "walkerchassis", "walker", "robot", "body",
        "bikechassis", "bike", "motorcyclechassis", "motorcycle"
    };
    int numNames = 17;

    for (int ni = 0; ni < numNames && scene.playerBodyIdx < 0; ni++) {
        for (int bi = 0; bi < scene.numBodies; bi++) {
            if (scene.bodyTypes[bi] != 2) continue; // dynamic only
            // Case-insensitive prefix match
            const char* bname = scene.bodyNames[bi];
            const char* target = playerNames[ni];
            bool match = true;
            int ci = 0;
            while (target[ci]) {
                char a = bname[ci]; if (a >= 'A' && a <= 'Z') a += 32;
                char b = target[ci]; if (b >= 'A' && b <= 'Z') b += 32;
                if (a != b) { match = false; break; }
                ci++;
            }
            if (match) {
                scene.playerBodyIdx = bi;
                break;
            }
        }
    }

    // Pass 2: fallback — heaviest dynamic body
    if (scene.playerBodyIdx < 0) {
        float maxMass = 0;
        for (int bi = 0; bi < scene.numBodies; bi++) {
            if (scene.bodyTypes[bi] != 2) continue;
            if (!scene.bodies[bi]) continue;
            float m = scene.bodies[bi]->GetMass();
            if (m > maxMass) {
                maxMass = m;
                scene.playerBodyIdx = bi;
            }
        }
    }

    if (scene.playerBodyIdx >= 0) {
        scene.playerBody = scene.bodies[scene.playerBodyIdx];
        char dbg[128];
        sprintf_s(dbg, sizeof(dbg), "RUBE: player body = '%s' (index %d, mass %.1f)\n",
                  scene.bodyNames[scene.playerBodyIdx], scene.playerBodyIdx,
                  scene.playerBody ? scene.playerBody->GetMass() : 0.0f);
        OutputDebugStringA(dbg);
    }
}

//============================================================================================
// RubeLoadScene
//============================================================================================
bool RubeLoadScene(const char* jsonPath) {
    // Clean up any existing scene
    RubeUnloadScene();

    int fileSize = 0;
    char* data = RubeReadFile(jsonPath, fileSize);
    if (!data) {
        char dbg[256];
        sprintf_s(dbg, sizeof(dbg), "RUBE: failed to read '%s'\n", jsonPath);
        OutputDebugStringA(dbg);
        return false;
    }

    JsonReader jr;
    jr.Init(data, fileSize);

    // ── Read world settings ──
    g_rubeScene.gravityX       = 0.0f;
    g_rubeScene.gravityY       = -9.81f;
    g_rubeScene.stepsPerSecond = 60.0f;
    g_rubeScene.velIterations  = 8;
    g_rubeScene.posIterations  = 3;
    g_rubeScene.minX = 999999.0f; g_rubeScene.maxX = -999999.0f;
    g_rubeScene.minY = 999999.0f; g_rubeScene.maxY = -999999.0f;
    g_rubeScene.numMotorJoints = 0;

    // ── Detect .rube native format (has "metaworld" wrapper) ──
    // CRITICAL: FindKey searches from pos=0 and when it hits "metaworld",
    // SkipValue skips the ENTIRE metaworld object. All nested keys
    // (metabody, metajoint, gravity, etc.) are inside that object and
    // would be skipped. Fix: offset the buffer pointer INTO the metaworld
    // object so subsequent FindKey calls search within it directly.
    bool isNative = false;
    jr.pos = 0;
    if (jr.FindKey("metaworld")) {
        isNative = true;
        // jr.pos is now at the '{' of metaworld's value
        int offset = jr.pos;
        jr.buf += offset;
        jr.len -= offset;
        jr.pos = 0;
        OutputDebugStringA("RUBE: detected native .rube format — rebased into metaworld\n");
    }

    jr.pos = 0;
    if (jr.FindKey("gravity")) {
        ReadVec2(jr, g_rubeScene.gravityX, g_rubeScene.gravityY);
    }
    jr.pos = 0;
    if (jr.FindKey("stepsPerSecond")) g_rubeScene.stepsPerSecond = jr.ReadFloat();
    jr.pos = 0;
    if (jr.FindKey("velocityIterations")) g_rubeScene.velIterations = jr.ReadInt();
    jr.pos = 0;
    if (jr.FindKey("positionIterations")) g_rubeScene.posIterations = jr.ReadInt();

    // ── Ensure minimum iteration counts for stability ──
    // Complex scenes (track links, walker linkages) need more solver passes
    // than the defaults (8/3) to prevent constraint explosion.
    if (g_rubeScene.velIterations < 16) g_rubeScene.velIterations = 16;
    if (g_rubeScene.posIterations < 6) g_rubeScene.posIterations = 6;

    // ── Create world ──
    b2Vec2 gravity(g_rubeScene.gravityX, g_rubeScene.gravityY);
    g_rubeScene.world = new b2World(gravity);
    g_rubeScene.world->SetAllowSleeping(true);

    // ── Parse bodies ──
    const char* bodyKey  = isNative ? "metabody"  : "body";
    const char* jointKey = isNative ? "metajoint" : "joint";

    int numBodies = ParseBodiesUnified(jr, g_rubeScene, bodyKey);
    char dbg[256];
    sprintf_s(dbg, sizeof(dbg), "RUBE: parsed %d bodies (%s format)\n",
              numBodies, isNative ? "native" : "export");
    OutputDebugStringA(dbg);

    // ── Identify player before parsing joints (joints need playerBodyIdx) ──
    IdentifyPlayerBody(g_rubeScene);

    // ── Init adjacency array ──
    for (int ai = 0; ai < RUBE_MAX_BODIES; ai++)
        g_rubeScene.isPlayerAdjacent[ai] = false;

    // ── Parse joints (native format uses body IDs, export uses indices) ──
    int numJoints = ParseJoints(jr, g_rubeScene, jointKey, isNative);

    // ── Mark player-adjacent bodies (3 levels deep) ─────────────────────
    // Level 1: bodies directly jointed to player (wheels, turret, fork)
    // Level 2: bodies jointed to level-1 (antenna segments, chain links)
    // Level 3: bodies jointed to level-2 (antenna tips, sub-assemblies)
    // This ensures all vehicle sub-parts are treated as one unit.
    if (g_rubeScene.playerBody) {
        // Level 1: direct connections from player
        for (b2JointEdge* je = g_rubeScene.playerBody->GetJointList(); je; je = je->next) {
            b2Body* other = je->other;
            for (int bi = 0; bi < g_rubeScene.numBodies; bi++) {
                if (g_rubeScene.bodies[bi] == other) {
                    g_rubeScene.isPlayerAdjacent[bi] = true;
                    break;
                }
            }
        }

        // Levels 2-3: walk outward from already-marked adjacent bodies
        for (int level = 0; level < 2; level++) {
            for (int bi = 0; bi < g_rubeScene.numBodies; bi++) {
                if (!g_rubeScene.isPlayerAdjacent[bi]) continue;
                if (!g_rubeScene.bodies[bi]) continue;
                for (b2JointEdge* je = g_rubeScene.bodies[bi]->GetJointList(); je; je = je->next) {
                    b2Body* other = je->other;
                    if (other == g_rubeScene.playerBody) continue; // skip back-link to player
                    for (int oi = 0; oi < g_rubeScene.numBodies; oi++) {
                        if (g_rubeScene.bodies[oi] == other) {
                            g_rubeScene.isPlayerAdjacent[oi] = true;
                            break;
                        }
                    }
                }
            }
        }

        // ── Capture motor joints on player-adjacent bodies ──
        for (b2Joint* joint = g_rubeScene.world->GetJointList(); joint; joint = joint->GetNext()) {
            b2Body* bA = joint->GetBodyA();
            b2Body* bB = joint->GetBodyB();

            // Skip if already captured
            bool alreadyCaptured = false;
            for (int mi = 0; mi < g_rubeScene.numMotorJoints; mi++) {
                if (g_rubeScene.motorJoints[mi].joint == joint) {
                    alreadyCaptured = true;
                    break;
                }
            }
            if (alreadyCaptured) continue;

            // Check if either body is player or player-adjacent
            int idxA = -1, idxB = -1;
            for (int bi = 0; bi < g_rubeScene.numBodies; bi++) {
                if (g_rubeScene.bodies[bi] == bA) idxA = bi;
                if (g_rubeScene.bodies[bi] == bB) idxB = bi;
            }
            bool onPlayer = (idxA == g_rubeScene.playerBodyIdx || idxB == g_rubeScene.playerBodyIdx);
            bool adjacent = (idxA >= 0 && g_rubeScene.isPlayerAdjacent[idxA])
                         || (idxB >= 0 && g_rubeScene.isPlayerAdjacent[idxB]);
            if (!onPlayer && !adjacent) continue;

            if (joint->GetType() == e_wheelJoint) {
                b2WheelJoint* wj = (b2WheelJoint*)joint;
                if (wj->GetMaxMotorTorque() > 0 && g_rubeScene.numMotorJoints < 16) {
                    g_rubeScene.motorJoints[g_rubeScene.numMotorJoints].joint     = joint;
                    g_rubeScene.motorJoints[g_rubeScene.numMotorJoints].jointType = 0;
                    g_rubeScene.motorJoints[g_rubeScene.numMotorJoints].maxTorque = wj->GetMaxMotorTorque();
                    g_rubeScene.numMotorJoints++;
                }
            }
            else if (joint->GetType() == e_revoluteJoint) {
                b2RevoluteJoint* rj = (b2RevoluteJoint*)joint;
                if (rj->GetMaxMotorTorque() > 0 && g_rubeScene.numMotorJoints < 16) {
                    g_rubeScene.motorJoints[g_rubeScene.numMotorJoints].joint     = joint;
                    g_rubeScene.motorJoints[g_rubeScene.numMotorJoints].jointType = 1;
                    g_rubeScene.motorJoints[g_rubeScene.numMotorJoints].maxTorque = rj->GetMaxMotorTorque();
                    g_rubeScene.numMotorJoints++;
                }
            }
        }
    }

    sprintf_s(dbg, sizeof(dbg), "RUBE: parsed %d joints, %d motor joints on player+adjacent\n",
              numJoints, g_rubeScene.numMotorJoints);
    OutputDebugStringA(dbg);

    // ── Fix antenna body fixtures ──────────────────────────────────────
    // Box2D's b2PolygonShape::Set() can modify very thin polygons
    // (< 0.04 units wide) into larger shapes. Replace antenna fixtures
    // with correctly-sized thin rectangles and set wobble-friendly properties.
    for (int bi = 0; bi < g_rubeScene.numBodies; bi++) {
        const char* bn = g_rubeScene.bodyNames[bi];
        if (!(bn[0]=='a' && bn[1]=='n' && bn[2]=='t' && bn[3]=='e' &&
              bn[4]=='n' && bn[5]=='n' && bn[6]=='a')) continue;

        b2Body* body = g_rubeScene.bodies[bi];
        if (!body) continue;

        // Destroy all existing fixtures on this antenna body
        b2Fixture* fix = body->GetFixtureList();
        while (fix) {
            b2Fixture* next = fix->GetNext();
            body->DestroyFixture(fix);
            fix = next;
        }

        // Create a properly thin rectangle fixture
        b2Vec2 verts[4];
        float hw = 0.015f;  // half-width (0.03 total — matches JSON data)
        float hh = 0.20f;   // half-height (0.40 total — matches JSON data)
        verts[0].Set(-hw, -hh);
        verts[1].Set( hw, -hh);
        verts[2].Set( hw,  hh);
        verts[3].Set(-hw,  hh);

        b2PolygonShape shape;
        shape.Set(verts, 4);

        b2FixtureDef fd;
        fd.shape       = &shape;
        fd.density     = 1.0f;
        fd.friction    = 0.3f;
        fd.restitution = 0.0f;
        // Disable ALL collisions — antenna is constrained by joints only.
        // This eliminates jitter from overlapping with chassis/turret.
        fd.filter.categoryBits = 0x0004;
        fd.filter.maskBits     = 0x0000;  // collide with nothing
        body->CreateFixture(&fd);

        // Set damping for natural antenna wobble (not jittery)
        body->SetLinearDamping(0.5f);
        body->SetAngularDamping(2.0f);  // moderate angular damping = smooth wobble

        sprintf_s(dbg, sizeof(dbg), "RUBE: replaced antenna fixture on '%s' (body %d)\n", bn, bi);
        OutputDebugStringA(dbg);
    }

    // ── Zero all body velocities ─────────────────────────────────────────
    // R.U.B.E. exports mid-simulation snapshots with non-zero velocities.
    // Starting from rest is much more stable for gameplay — prevents the
    // truck front wheel wobble and other initial impulse artifacts.
    for (b2Body* body = g_rubeScene.world->GetBodyList(); body; body = body->GetNext()) {
        body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
        body->SetAngularVelocity(0.0f);
    }

    // ── Parse images ──
    const char* imageKey = isNative ? "metaimage" : "image";
    g_rubeScene.numImages = 0;
    int numImages = ParseImages(jr, g_rubeScene, imageKey, isNative);
    if (numImages > 0) {
        SortImagesByRenderOrder(g_rubeScene);
        sprintf_s(dbg, sizeof(dbg), "RUBE: parsed %d images\n", numImages);
        OutputDebugStringA(dbg);
    }

    delete[] data;

    g_rubeScene.active = true;

    // ── Load textures (needs D3D device — must happen after data freed) ──
    if (g_rubeScene.numImages > 0) {
        LoadRubeTextures(g_rubeScene, jsonPath);
    }

    // Store path for restart
    int pi = 0;
    while (pi < 255 && jsonPath[pi]) { g_rubeScene.loadedPath[pi] = jsonPath[pi]; pi++; }
    g_rubeScene.loadedPath[pi] = '\0';

    sprintf_s(dbg, sizeof(dbg), "RUBE: scene loaded from '%s' — %d bodies, %d joints\n",
              jsonPath, numBodies, numJoints);
    OutputDebugStringA(dbg);

    return true;
}

//============================================================================================
// RubeUnloadScene
//============================================================================================
void RubeUnloadScene() {
    // Release image textures before clearing the scene
    for (int i = 0; i < g_rubeScene.numImages; i++) {
        if (g_rubeScene.images[i].texture) {
            g_rubeScene.images[i].texture->Release();
            g_rubeScene.images[i].texture = NULL;
        }
    }

    if (g_rubeScene.world) {
        delete g_rubeScene.world;
    }
    // Preserve loadedPath for potential restart
    char savedPath[256];
    int si = 0;
    while (si < 255 && g_rubeScene.loadedPath[si]) {
        savedPath[si] = g_rubeScene.loadedPath[si]; si++;
    }
    savedPath[si] = '\0';

    memset(&g_rubeScene, 0, sizeof(RubeScene));
    g_rubeScene.active = false;
    g_rubeScene.world  = NULL;

    // Restore path
    si = 0;
    while (si < 255 && savedPath[si]) {
        g_rubeScene.loadedPath[si] = savedPath[si]; si++;
    }
    g_rubeScene.loadedPath[si] = '\0';
}

//============================================================================================
// RubeRestartScene — reload the current scene from the same JSON file
//============================================================================================
bool RubeRestartScene() {
    if (g_rubeScene.loadedPath[0] == '\0') return false;
    char path[256];
    int pi = 0;
    while (pi < 255 && g_rubeScene.loadedPath[pi]) {
        path[pi] = g_rubeScene.loadedPath[pi]; pi++;
    }
    path[pi] = '\0';
    RubeUnloadScene();
    return RubeLoadScene(path);
}

//============================================================================================
// RubeUpdate
//   throttleInput: -1.0 (full reverse) to +1.0 (full forward), 0 = coast
//   turretInput:   -1.0 to +1.0 from right stick Y axis (0 = no turret movement)
//============================================================================================
void RubeUpdate(float dt, float throttleInput, float turretInput) {
    if (!g_rubeScene.active || !g_rubeScene.world) return;

    // ── Apply motor input to all player motor joints ──
    for (int mi = 0; mi < g_rubeScene.numMotorJoints; mi++) {
        b2Joint* joint = g_rubeScene.motorJoints[mi].joint;
        int      jtype = g_rubeScene.motorJoints[mi].jointType;
        float    torque = g_rubeScene.motorJoints[mi].maxTorque;
        if (!joint) continue;

        if (jtype == 0) {
            // ── Wheel joints: driven by triggers (throttle) ──
            b2WheelJoint* wj = (b2WheelJoint*)joint;
            if (fabsf(throttleInput) < 0.05f) {
                wj->EnableMotor(false);
            } else {
                float targetSpeed = throttleInput * 20.0f;
                float cappedTorque = (torque > 80.0f) ? 80.0f : torque;
                wj->SetMotorSpeed(targetSpeed);
                wj->SetMaxMotorTorque(cappedTorque);
                wj->EnableMotor(true);
            }
        }
        else if (jtype == 1) {
            // ── Revolute joints: driven by right stick (turret/arms) ──
            b2RevoluteJoint* rj = (b2RevoluteJoint*)joint;
            if (fabsf(turretInput) < 0.05f) {
                rj->EnableMotor(false);
            } else {
                float targetSpeed = turretInput * 5.0f;  // slower for turret precision
                rj->SetMotorSpeed(targetSpeed);
                rj->SetMaxMotorTorque(torque);
                rj->EnableMotor(true);
            }
        }
    }

    // ── Step physics (2 sub-steps for constraint chain stability) ──
    // Complex track link chains and walker linkages need smaller time steps
    // to prevent constraint explosion when hitting bumps at speed.
    float timeStep = 1.0f / g_rubeScene.stepsPerSecond;
    float halfStep = timeStep * 0.5f;
    g_rubeScene.world->Step(halfStep, g_rubeScene.velIterations, g_rubeScene.posIterations);
    g_rubeScene.world->Step(halfStep, g_rubeScene.velIterations, g_rubeScene.posIterations);
}

//============================================================================================
// RubeGetCameraTarget
//============================================================================================
bool RubeGetCameraTarget(float& outX, float& outY) {
    if (!g_rubeScene.active || !g_rubeScene.playerBody) return false;
    b2Vec2 pos = g_rubeScene.playerBody->GetPosition();
    outX = pos.x;
    outY = pos.y;
    return true;
}

//============================================================================================
// RubeGetPlayerSpeed
//============================================================================================
float RubeGetPlayerSpeed() {
    if (!g_rubeScene.active || !g_rubeScene.playerBody) return 0.0f;
    b2Vec2 vel = g_rubeScene.playerBody->GetLinearVelocity();
    return sqrtf(vel.x * vel.x + vel.y * vel.y);
}

//============================================================================================
// RubeGetPlayerAngle
//============================================================================================
float RubeGetPlayerAngle() {
    if (!g_rubeScene.active || !g_rubeScene.playerBody) return 0.0f;
    return g_rubeScene.playerBody->GetAngle();
}

//============================================================================================
// RubeIsActive
//============================================================================================
bool RubeIsActive() {
    return g_rubeScene.active;
}

//============================================================================================
// RENDER HELPERS
// Draws all R.U.B.E. bodies using the game's existing D3D pipeline:
//   - Vertex shader with WVP matrix in VS constant c0 (4 float4s)
//   - Solid colour pixel shader with RGBA in PS constant c0
//   - TEXVERTEX format (x, y, z, u, v) — u/v unused with solid shader
// Each body gets its own WVP = bodyRotation * bodyTranslation * view * proj
// so polygon vertices stay in local body space — the matrix does the rest.
//============================================================================================

void RubeRenderScene() {
    if (!g_rubeScene.active || !g_rubeScene.world || !g_pd3dDevice) return;
    if (!g_pSolidColourShader || !g_pVertexShader) return;

    float camX = g_fScrollPos;

    g_pd3dDevice->SetPixelShader(g_pSolidColourShader);
    g_pd3dDevice->SetTexture(0, NULL);

    // ── R.U.B.E. editor color palette ───────────────────────────────────
    // Fill colors (semi-transparent)
    float colTerrainFill[4]  = { 0.22f, 0.35f, 0.18f, 0.70f }; // dark green terrain
    float colDynamicFill[4]  = { 0.55f, 0.38f, 0.32f, 0.45f }; // salmon/brown bodies
    float colPlayerFill[4]   = { 0.60f, 0.40f, 0.33f, 0.50f }; // player chassis
    float colCircleFill[4]   = { 0.45f, 0.30f, 0.25f, 0.40f }; // wheel fill
    // Outline colors (opaque, lighter)
    float colTerrainLine[4]  = { 0.40f, 0.55f, 0.30f, 0.90f }; // green outline
    float colDynamicLine[4]  = { 0.75f, 0.55f, 0.45f, 0.85f }; // salmon outline
    float colPlayerLine[4]   = { 0.80f, 0.58f, 0.45f, 0.90f }; // player outline
    float colCircleLine[4]   = { 0.70f, 0.50f, 0.40f, 0.80f }; // wheel outline

    // ── R.U.B.E. editor grid ──────────────────────────────────────────────
    // Subtle grid lines matching the R.U.B.E. editor background.
    {
        D3DXMATRIX matTrans, matGridWVP;
        D3DXMatrixTranslation(&matTrans, 0.0f, 0.0f, 0.0f);
        matGridWVP = matTrans * g_matView * g_matProj;
        g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matGridWVP, 4);

        float gridSpacing = 5.0f;
        float gridExtent  = 60.0f;

        // Snap grid origin to nearest grid line
        float gx0 = floorf((camX - gridExtent) / gridSpacing) * gridSpacing;
        float gy0 = floorf((g_camY - gridExtent) / gridSpacing) * gridSpacing;

        // Grid line color (subtle dark red/brown like R.U.B.E.)
        float gridCol[4] = { 0.20f, 0.14f, 0.12f, 0.30f };
        g_pd3dDevice->SetPixelShaderConstantF(0, gridCol, 1);

        // Vertical lines
        for (float gx = gx0; gx < camX + gridExtent; gx += gridSpacing) {
            TEXVERTEX ln[2];
            ln[0].x = gx - camX; ln[0].y = g_camY - gridExtent; ln[0].z = 0; ln[0].u = 0; ln[0].v = 0;
            ln[1].x = gx - camX; ln[1].y = g_camY + gridExtent; ln[1].z = 0; ln[1].u = 0; ln[1].v = 0;
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 1, ln, sizeof(TEXVERTEX));
        }
        // Horizontal lines
        for (float gy = gy0; gy < g_camY + gridExtent; gy += gridSpacing) {
            TEXVERTEX ln[2];
            ln[0].x = -gridExtent; ln[0].y = gy; ln[0].z = 0; ln[0].u = 0; ln[0].v = 0;
            ln[1].x =  gridExtent; ln[1].y = gy; ln[1].z = 0; ln[1].u = 0; ln[1].v = 0;
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 1, ln, sizeof(TEXVERTEX));
        }
    }

    // ════════════════════════════════════════════════════════════════════
    // R.U.B.E. EDITOR STYLE RENDERING
    // Each body: semi-transparent fill + wireframe outline on top
    // Draw order: static terrain → dynamic objects → player → circles
    // ════════════════════════════════════════════════════════════════════

    // Helper lambda-style macro for drawing a polygon with fill + outline
    // (can't use lambdas in C++03, so we inline it)

    // ── PASS 1: Static terrain (fill + outline) ─────────────────────────
    for (b2Body* body = g_rubeScene.world->GetBodyList(); body; body = body->GetNext()) {
        if (body->GetType() != b2_staticBody) continue;

        b2Vec2 bodyPos = body->GetPosition();
        float bodyAngle = body->GetAngle();

        D3DXMATRIX matRot, matTrans, matWVP;
        D3DXMatrixRotationZ(&matRot, bodyAngle);
        D3DXMatrixTranslation(&matTrans, bodyPos.x - camX, bodyPos.y, 0.0f);
        matWVP = matRot * matTrans * g_matView * g_matProj;
        g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

        for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()) {
            if (fix->GetShape()->GetType() != b2Shape::e_polygon) continue;
            b2PolygonShape* poly = (b2PolygonShape*)fix->GetShape();
            int vc = poly->GetVertexCount();
            if (vc < 3 || vc > 8) continue;

            // Boundary wall filter
            float mnX = 99999, mxX = -99999, mnY = 99999, mxY = -99999;
            for (int ci = 0; ci < vc; ci++) {
                b2Vec2 v = poly->GetVertex(ci);
                if (v.x < mnX) mnX = v.x; if (v.x > mxX) mxX = v.x;
                if (v.y < mnY) mnY = v.y; if (v.y > mxY) mxY = v.y;
            }
            if ((mxX - mnX) < 5.0f && (mxY - mnY) > 15.0f) continue;

            TEXVERTEX verts[8];
            for (int vi = 0; vi < vc; vi++) {
                b2Vec2 v = poly->GetVertex(vi);
                verts[vi].x = v.x; verts[vi].y = v.y; verts[vi].z = 0;
                verts[vi].u = 0; verts[vi].v = 0;
            }

            // Fill
            g_pd3dDevice->SetPixelShaderConstantF(0, colTerrainFill, 1);
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, vc - 2, verts, sizeof(TEXVERTEX));

            // Outline
            TEXVERTEX outline[9]; // max 8 verts + closing vert
            for (int oi = 0; oi < vc; oi++) outline[oi] = verts[oi];
            outline[vc] = verts[0]; // close the loop
            g_pd3dDevice->SetPixelShaderConstantF(0, colTerrainLine, 1);
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, vc, outline, sizeof(TEXVERTEX));
        }
    }

    // ── PASS 2: Dynamic bodies (NOT player) ─────────────────────────────
    for (b2Body* body = g_rubeScene.world->GetBodyList(); body; body = body->GetNext()) {
        if (body->GetType() == b2_staticBody) continue;
        if (body == g_rubeScene.playerBody) continue;

        b2Vec2 bodyPos = body->GetPosition();
        float bodyAngle = body->GetAngle();

        // ── Detect antenna bodies by name and render as thin strips ──
        // Box2D's b2PolygonShape::Set() can modify very thin polygons,
        // so we bypass the polygon entirely and draw a thin strip directly.
        bool isAntenna = false;
        for (int bi = 0; bi < g_rubeScene.numBodies; bi++) {
            if (g_rubeScene.bodies[bi] == body) {
                // Check if name starts with "antenna"
                const char* bn = g_rubeScene.bodyNames[bi];
                if (bn[0]=='a' && bn[1]=='n' && bn[2]=='t' && bn[3]=='e' &&
                    bn[4]=='n' && bn[5]=='n' && bn[6]=='a') {
                    isAntenna = true;
                }
                break;
            }
        }
        if (isAntenna) {
            // Draw a thin vertical strip at the body's position
            // Each antenna segment is ~0.4 units tall, ~0.02 units wide
            D3DXMATRIX matRot, matTrans, matWVP;
            D3DXMatrixRotationZ(&matRot, bodyAngle);
            D3DXMatrixTranslation(&matTrans, bodyPos.x - camX, bodyPos.y, 0.0f);
            matWVP = matRot * matTrans * g_matView * g_matProj;
            g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

            float hw = 0.012f;  // half-width of antenna strip
            float hh = 0.20f;   // half-height of antenna strip
            TEXVERTEX strip[4];
            strip[0].x = -hw; strip[0].y =  hh; strip[0].z = 0; strip[0].u = 0; strip[0].v = 0;
            strip[1].x =  hw; strip[1].y =  hh; strip[1].z = 0; strip[1].u = 0; strip[1].v = 0;
            strip[2].x = -hw; strip[2].y = -hh; strip[2].z = 0; strip[2].u = 0; strip[2].v = 0;
            strip[3].x =  hw; strip[3].y = -hh; strip[3].z = 0; strip[3].u = 0; strip[3].v = 0;

            g_pd3dDevice->SetPixelShaderConstantF(0, colDynamicLine, 1);
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, strip, sizeof(TEXVERTEX));
            continue;
        }

        D3DXMATRIX matRot, matTrans, matWVP;
        D3DXMatrixRotationZ(&matRot, bodyAngle);
        D3DXMatrixTranslation(&matTrans, bodyPos.x - camX, bodyPos.y, 0.0f);
        matWVP = matRot * matTrans * g_matView * g_matProj;
        g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

        for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()) {
            b2Shape* shape = fix->GetShape();
            if (shape->GetType() == b2Shape::e_polygon) {
                b2PolygonShape* poly = (b2PolygonShape*)shape;
                int vc = poly->GetVertexCount();
                if (vc < 3 || vc > 8) continue;

                TEXVERTEX verts[8];
                for (int vi = 0; vi < vc; vi++) {
                    b2Vec2 v = poly->GetVertex(vi);
                    verts[vi].x = v.x; verts[vi].y = v.y; verts[vi].z = 0;
                    verts[vi].u = 0; verts[vi].v = 0;
                }

                // Fill
                g_pd3dDevice->SetPixelShaderConstantF(0, colDynamicFill, 1);
                g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, vc - 2, verts, sizeof(TEXVERTEX));

                // Outline
                TEXVERTEX outline[9];
                for (int oi = 0; oi < vc; oi++) outline[oi] = verts[oi];
                outline[vc] = verts[0];
                g_pd3dDevice->SetPixelShaderConstantF(0, colDynamicLine, 1);
                g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, vc, outline, sizeof(TEXVERTEX));
            }
        }
    }

    // ── PASS 3: Player body (fill + outline) ────────────────────────────
    if (g_rubeScene.playerBody) {
        b2Body* body = g_rubeScene.playerBody;
        b2Vec2 bodyPos = body->GetPosition();
        float bodyAngle = body->GetAngle();

        D3DXMATRIX matRot, matTrans, matWVP;
        D3DXMatrixRotationZ(&matRot, bodyAngle);
        D3DXMatrixTranslation(&matTrans, bodyPos.x - camX, bodyPos.y, 0.0f);
        matWVP = matRot * matTrans * g_matView * g_matProj;
        g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

        for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()) {
            if (fix->GetShape()->GetType() != b2Shape::e_polygon) continue;
            b2PolygonShape* poly = (b2PolygonShape*)fix->GetShape();
            int vc = poly->GetVertexCount();
            if (vc < 3 || vc > 8) continue;

            TEXVERTEX verts[8];
            for (int vi = 0; vi < vc; vi++) {
                b2Vec2 v = poly->GetVertex(vi);
                verts[vi].x = v.x; verts[vi].y = v.y; verts[vi].z = 0;
                verts[vi].u = 0; verts[vi].v = 0;
            }

            g_pd3dDevice->SetPixelShaderConstantF(0, colPlayerFill, 1);
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, vc - 2, verts, sizeof(TEXVERTEX));

            TEXVERTEX outline[9];
            for (int oi = 0; oi < vc; oi++) outline[oi] = verts[oi];
            outline[vc] = verts[0];
            g_pd3dDevice->SetPixelShaderConstantF(0, colPlayerLine, 1);
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, vc, outline, sizeof(TEXVERTEX));
        }
    }

    // ── PASS 4: ALL circles (fill + outline) ────────────────────────────
    for (b2Body* body = g_rubeScene.world->GetBodyList(); body; body = body->GetNext()) {
        b2Vec2 bodyPos = body->GetPosition();
        float bodyAngle = body->GetAngle();

        D3DXMATRIX matRot, matTrans, matWVP;
        D3DXMatrixRotationZ(&matRot, bodyAngle);
        D3DXMatrixTranslation(&matTrans, bodyPos.x - camX, bodyPos.y, 0.0f);
        matWVP = matRot * matTrans * g_matView * g_matProj;

        for (b2Fixture* fix = body->GetFixtureList(); fix; fix = fix->GetNext()) {
            if (fix->GetShape()->GetType() != b2Shape::e_circle) continue;
            b2CircleShape* circ = (b2CircleShape*)fix->GetShape();
            float cx = circ->m_p.x, cy = circ->m_p.y, r = circ->m_radius;

            g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

            const int SEGS = 24;

            // Fill
            TEXVERTEX cv[SEGS + 2];
            cv[0].x = cx; cv[0].y = cy; cv[0].z = 0; cv[0].u = 0; cv[0].v = 0;
            for (int si = 0; si <= SEGS; si++) {
                float a = (float)si / (float)SEGS * 6.283185f;
                cv[si+1].x = cx + cosf(a)*r; cv[si+1].y = cy + sinf(a)*r;
                cv[si+1].z = 0; cv[si+1].u = 0; cv[si+1].v = 0;
            }
            g_pd3dDevice->SetPixelShaderConstantF(0, colCircleFill, 1);
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, SEGS, cv, sizeof(TEXVERTEX));

            // Outline ring
            TEXVERTEX ring[SEGS + 1];
            for (int si = 0; si <= SEGS; si++) {
                float a = (float)si / (float)SEGS * 6.283185f;
                ring[si].x = cx + cosf(a)*r; ring[si].y = cy + sinf(a)*r;
                ring[si].z = 0; ring[si].u = 0; ring[si].v = 0;
            }
            g_pd3dDevice->SetPixelShaderConstantF(0, colCircleLine, 1);
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, SEGS, ring, sizeof(TEXVERTEX));

            // Radius line (rotation indicator)
            TEXVERTEX radLine[2];
            radLine[0].x = cx; radLine[0].y = cy; radLine[0].z = 0; radLine[0].u = 0; radLine[0].v = 0;
            radLine[1].x = cx + r; radLine[1].y = cy; radLine[1].z = 0; radLine[1].u = 0; radLine[1].v = 0;
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 1, radLine, sizeof(TEXVERTEX));
        }
    }

    // ── PASS 5: R.U.B.E. images (textured quads) ───────────────────────
    if (g_rubeScene.numImages > 0) {
        g_pd3dDevice->SetPixelShader(g_pPixelShader);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

        for (int ii = 0; ii < g_rubeScene.numImages; ii++) {
            RubeImage& img = g_rubeScene.images[ii];
            if (!img.texture) continue;

            float bx = 0, by = 0, bAngle = 0;
            if (img.bodyIdx >= 0 && img.bodyIdx < g_rubeScene.numBodies &&
                g_rubeScene.bodies[img.bodyIdx]) {
                b2Vec2 bpos = g_rubeScene.bodies[img.bodyIdx]->GetPosition();
                bx = bpos.x; by = bpos.y;
                bAngle = g_rubeScene.bodies[img.bodyIdx]->GetAngle();
            }

            float halfH = img.scale * 0.5f;
            float aspect = (img.texH > 0) ? (float)img.texW / (float)img.texH : 1.0f;
            float halfW = halfH * aspect;

            D3DXMATRIX matImgRot, matImgTrans, matBodyRot, matBodyTrans, matWVP;
            D3DXMatrixRotationZ(&matImgRot, img.angle);
            D3DXMatrixTranslation(&matImgTrans, img.centerX, img.centerY, 0.0f);
            D3DXMatrixRotationZ(&matBodyRot, bAngle);
            D3DXMatrixTranslation(&matBodyTrans, bx - camX, by, 0.0f);
            matWVP = matImgRot * matImgTrans * matBodyRot * matBodyTrans * g_matView * g_matProj;
            g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);
            g_pd3dDevice->SetTexture(0, img.texture);

            TEXVERTEX quad[4];
            quad[0].x = -halfW; quad[0].y =  halfH; quad[0].z = 0; quad[0].u = 0; quad[0].v = 0;
            quad[1].x =  halfW; quad[1].y =  halfH; quad[1].z = 0; quad[1].u = 1; quad[1].v = 0;
            quad[2].x = -halfW; quad[2].y = -halfH; quad[2].z = 0; quad[2].u = 0; quad[2].v = 1;
            quad[3].x =  halfW; quad[3].y = -halfH; quad[3].z = 0; quad[3].u = 1; quad[3].v = 1;
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(TEXVERTEX));
        }

        g_pd3dDevice->SetPixelShader(g_pSolidColourShader);
        g_pd3dDevice->SetTexture(0, NULL);
    }
}
