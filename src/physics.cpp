#include "stdafx.h"
#include "physics.h"
#include "terrain.h"
#include "bridge.h"
#include "audio.h"
#include "config.h"

// Forward declarations — defined later in this file
static void InitWaterSurface();
void CreateWaterRegions();

//============================================================================================
// BOX2D INITIALIZATION
//============================================================================================

void InitBox2DPhysics() {
    b2Vec2 gravity(0.0f, GRAVITY);
    g_pBox2DWorld = new b2World(gravity);
    g_pBox2DWorld->SetAllowSleeping(false);
    
    b2BodyDef groundBodyDef;
    groundBodyDef.type = b2_staticBody;
    groundBodyDef.position.Set(0.0f, 0.0f);
    g_groundBody = g_pBox2DWorld->CreateBody(&groundBodyDef);
    
    b2BodyDef chassisDef;
    chassisDef.type = b2_dynamicBody;
    chassisDef.position.Set(CAR_START_X, CAR_START_Y);
    
    b2PolygonShape chassisShape;
    b2Vec2 vertices[6];

    if (g_isBoatLevel) {
        // Boat V-hull: low flat bottom, pointed bow (right), blunt stern (left)
        // Convex CCW polygon — wider at waterline, narrow at keel
        vertices[0].Set(-2.0f, -0.15f); // stern keel
        vertices[1].Set( 1.5f, -0.40f); // bow keel (lower — V shape)
        vertices[2].Set( 2.1f,  0.15f); // bow tip
        vertices[3].Set( 1.2f,  0.55f); // bow deck
        vertices[4].Set(-1.2f,  0.55f); // stern deck
        vertices[5].Set(-2.0f,  0.20f); // stern side
    } else {
        // Standard car chassis shape
        vertices[0].Set(-1.5f, -0.2f);
        vertices[1].Set( 1.2f, -0.2f);
        vertices[2].Set( 1.2f,  0.0f);
        vertices[3].Set( 0.0f,  0.5f);
        vertices[4].Set(-1.15f, 0.5f);
        vertices[5].Set(-1.5f,  0.2f);
    }

    chassisShape.Set(vertices, 6);
    
    g_carChassis = g_pBox2DWorld->CreateBody(&chassisDef);
    g_carChassis->CreateFixture(&chassisShape, 1.0f);
    
    b2CircleShape wheelShape;
    wheelShape.m_radius = g_visualWheelRadius;
    
    b2FixtureDef wheelFixture;
    wheelFixture.shape       = &wheelShape;
    wheelFixture.density     = 1.0f;
    wheelFixture.friction    = g_isAirplane ? 0.0f : 1.9f;
    wheelFixture.restitution = g_isAirplane ? 0.0f : 0.2f;
    wheelFixture.isSensor    = g_isAirplane;
    
    b2BodyDef rearWheelDef;
    rearWheelDef.type = b2_dynamicBody;
    rearWheelDef.position.Set(CAR_START_X + g_wheelRearOffsetX, g_wheelRearStartY);
    
    g_wheelRear = g_pBox2DWorld->CreateBody(&rearWheelDef);
    g_wheelRear->CreateFixture(&wheelFixture);
    g_wheelRear->SetBullet(true);
    
    b2BodyDef frontWheelDef;
    frontWheelDef.type = b2_dynamicBody;
    frontWheelDef.position.Set(CAR_START_X + g_wheelFrontOffsetX, g_wheelFrontStartY);
    
    g_wheelFront = g_pBox2DWorld->CreateBody(&frontWheelDef);
    g_wheelFront->CreateFixture(&wheelFixture);
    g_wheelFront->SetBullet(true);
    
    b2WheelJointDef rearJointDef;
    b2Vec2 axis(0.0f, 1.0f);
    
    rearJointDef.Initialize(g_carChassis, g_wheelRear, g_wheelRear->GetPosition(), axis);
    rearJointDef.motorSpeed = 0.0f;
    rearJointDef.maxMotorTorque = g_motorTorqueRear;
    rearJointDef.enableMotor = true;
    rearJointDef.frequencyHz = g_wheelJointFrequency;
    rearJointDef.dampingRatio = g_wheelJointDamping;
    
    g_rearSuspension = (b2WheelJoint*)g_pBox2DWorld->CreateJoint(&rearJointDef);
    
    b2WheelJointDef frontJointDef;
    frontJointDef.Initialize(g_carChassis, g_wheelFront, g_wheelFront->GetPosition(), axis);
    frontJointDef.motorSpeed = 0.0f;
    frontJointDef.maxMotorTorque = g_motorTorqueFront;
    frontJointDef.enableMotor = false;
    frontJointDef.frequencyHz = g_wheelJointFrequency;
    frontJointDef.dampingRatio = g_wheelJointDamping;
    
    g_frontSuspension = (b2WheelJoint*)g_pBox2DWorld->CreateJoint(&frontJointDef);
}

//============================================================================================

//============================================================================================
// LEVEL MANAGEMENT
//============================================================================================

void LoadLevel(Level level) {
    g_currentLevel = level;

    // ✅ Always clean up any existing bridge before rebuilding the world
    DestroyBridge();

    if (g_groundBody != NULL) {
        g_pBox2DWorld->DestroyBody(g_groundBody);
    }
    
    b2BodyDef groundBodyDef;
    groundBodyDef.type = b2_staticBody;
    groundBodyDef.position.Set(0.0f, 0.0f);
    g_groundBody = g_pBox2DWorld->CreateBody(&groundBodyDef);
    
    // ── Water + terrain generation order ────────────────────────────────────
    // 1. SetLevelWater first — registers pool positions read by terrain carve
    // 2. SetLevelJumps next  — registers jump positions
    // 3. GenerateWrappingTerrain — carves both water valleys and jump features
    // 4. InitWaterSurface — reads carved terrain to set water surface Y
    // 5. CreateWaterRegions — creates Box2D sensor bodies
    // Moon gravity for Level 6 — ~1/6 of Earth gravity
    if (level == LEVEL_6)
        g_pBox2DWorld->SetGravity(b2Vec2(0.0f, GRAVITY / 6.0f));
    else
        g_pBox2DWorld->SetGravity(b2Vec2(0.0f, GRAVITY));

    // Airplane flag — set by caller (main.cpp) via g_selectedCar check
    // Reset here so LoadLevel is clean; main.cpp sets it after this call.

    SetLevelWater(level);
    SetLevelJumps(level);
    GenerateWrappingTerrain();
    InitWaterSurface();
    CreateWaterRegions();

    // ✅ Spawn bridge after terrain so GetTerrainHeightAtX() has valid data
    if (level == LEVEL_2) {
        CreateBridge(BRIDGE_L2_START_X);
    }
    else if (level == LEVEL_4) {
        CreateBridge(BRIDGE_L4_START_X);
    }
    else if ((int)level >= LEVEL_CUSTOM_1) {
        int ci = (int)level - (int)LEVEL_CUSTOM_1;
        if (ci >= 0 && ci < g_numCustomLevels && g_customLevels[ci].loaded
            && g_customLevels[ci].hasBridge > 0.0f) {
            CreateBridge(g_customLevels[ci].bridgeStartX);
        }
    }
    
    // Level 7: spawn boat over water (past the water start at X=15)
    float spawnX = (level == LEVEL_7) ? 45.0f : CAR_START_X;
    // Spawn Y is slightly above rest so boat settles onto water surface
    float spawnY = (level == LEVEL_7) ? CAR_START_Y + 1.5f : CAR_START_Y;

    if (g_carChassis != NULL) {
        g_carChassis->SetTransform(b2Vec2(spawnX, spawnY), 0.0f);
        g_carChassis->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
        g_carChassis->SetAngularVelocity(0.0f);
    }

    if (g_wheelRear != NULL) {
        g_wheelRear->SetTransform(b2Vec2(spawnX + g_wheelRearOffsetX, g_wheelRearStartY), 0.0f);
        g_wheelRear->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
        g_wheelRear->SetAngularVelocity(0.0f);
    }

    if (g_wheelFront != NULL) {
        g_wheelFront->SetTransform(b2Vec2(spawnX + g_wheelFrontOffsetX, g_wheelFrontStartY), 0.0f);
        g_wheelFront->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
        g_wheelFront->SetAngularVelocity(0.0f);
    }

    g_fScrollPos = spawnX;
    g_motorSpeed = 0.0f;
    g_passedFinishLine = false;

    // Water setup handled at start of LoadLevel above
}

//============================================================================================
// WATER PHYSICS
// Buoyancy + drag using sensor fixtures + contact listener.
// Based on the displaced-mass principle from Erin Catto / iforce2d tutorial:
//   F_buoy = fluid_density * |gravity| * submerged_area (upward)
//   F_drag = -drag_coeff * velocity * submerged_fraction
//============================================================================================

// ── Water region pool ────────────────────────────────────────────────────
WaterRegion g_waterRegions[MAX_WATER_REGIONS];
int         g_numWaterRegions = 0;

bool        g_isBoatLevel    = false;
bool        g_isAirplane     = false;
float       g_propSpin       = 0.0f;

// ── User data tag written onto every water sensor fixture ─────────────────
// We store a pointer to the WaterRegion so BeginContact can identify it.
// Box2D fixture user data is void* on Xbox 360 Box2D 2.x.
struct WaterFixtureTag {
    WaterRegion* region;
};
static WaterFixtureTag s_waterTags[MAX_WATER_REGIONS];

// ── Contact listener ─────────────────────────────────────────────────────
// Tracks which dynamic bodies are currently inside a water sensor.
// We keep a flat list of (body, region) pairs — up to 16 simultaneous contacts.
#define MAX_WATER_CONTACTS 16
struct WaterContact {
    b2Body*      body;
    WaterRegion* region;
    bool         active;
};
static WaterContact s_waterContacts[MAX_WATER_CONTACTS];

// Forward declaration — SplashWater is defined after the contact listener
static void SplashWaterForward(float worldX, float splashMag);

class WaterContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) {
        b2Fixture* fA = contact->GetFixtureA();
        b2Fixture* fB = contact->GetFixtureB();

        b2Fixture* sensor  = NULL;
        b2Fixture* other   = NULL;
        if (fA->IsSensor()) { sensor = fA; other = fB; }
        else if (fB->IsSensor()) { sensor = fB; other = fA; }
        else return;

        // Check this is a water sensor (userData set during CreateWaterRegions)
        if (!sensor->GetUserData()) return;
        WaterFixtureTag* tag = (WaterFixtureTag*)sensor->GetUserData();
        if (!tag->region) return;

        b2Body* body = other->GetBody();
        if (body->GetType() != b2_dynamicBody) return;

        // Splash on entry — scale with both vertical AND horizontal speed
        // (car driving in at speed creates a wider entry ripple than dropping)
        b2Vec2 vel = body->GetLinearVelocity();
        float impactSpd = fabsf(vel.y) + fabsf(vel.x) * 0.4f;
        if (impactSpd > 0.8f) {
            float splashMag = Xbox360Min(impactSpd * 0.22f, 2.0f);
            SplashWaterForward(body->GetPosition().x, splashMag);
        }

        // Add to contact list
        for (int i = 0; i < MAX_WATER_CONTACTS; i++) {
            if (!s_waterContacts[i].active) {
                s_waterContacts[i].body   = body;
                s_waterContacts[i].region = tag->region;
                s_waterContacts[i].active = true;
                return;
            }
        }
    }

    void EndContact(b2Contact* contact) {
        b2Fixture* fA = contact->GetFixtureA();
        b2Fixture* fB = contact->GetFixtureB();

        b2Fixture* sensor = NULL;
        b2Fixture* other  = NULL;
        if (fA->IsSensor()) { sensor = fA; other = fB; }
        else if (fB->IsSensor()) { sensor = fB; other = fA; }
        else return;

        if (!sensor->GetUserData()) return;
        WaterFixtureTag* tag = (WaterFixtureTag*)sensor->GetUserData();
        if (!tag->region) return;

        b2Body* body = other->GetBody();
        for (int i = 0; i < MAX_WATER_CONTACTS; i++) {
            if (s_waterContacts[i].active
                && s_waterContacts[i].body   == body
                && s_waterContacts[i].region == tag->region) {
                s_waterContacts[i].active = false;
                return;
            }
        }
    }
};

static WaterContactListener s_waterListener;

// ── Create / destroy water regions ───────────────────────────────────────
void CreateWaterRegions() {
    g_pBox2DWorld->SetContactListener(&s_waterListener);

    for (int i = 0; i < g_numWaterRegions; i++) {
        WaterRegion& wr = g_waterRegions[i];
        if (!wr.active) continue;

        // Static sensor body: spans from water surface (restY) down to valley floor.
        // Floor = restY - WATER_VALLEY_DEPTH * 0.80f (inverse of how restY was set).
        // Centre the box between surface and floor, sized to match exactly.
        // Use wr.depth (per-region value) — Level 7 has depth 4.5, not the default 2.0
        float waterFloorY  = wr.restY - wr.depth * 0.80f;
        float waterHeight  = wr.restY - waterFloorY;
        float sensorCentreY = waterFloorY + waterHeight * 0.5f;

        b2BodyDef bd;
        bd.type     = b2_staticBody;
        bd.position.Set(wr.x + wr.width * 0.5f, sensorCentreY);
        b2Body* body = g_pBox2DWorld->CreateBody(&bd);
        wr.sensorBody = body;  // cache pointer so ApplyWaterForces can find it directly

        b2PolygonShape shape;
        shape.SetAsBox(wr.width * 0.5f, waterHeight * 0.5f);

        b2FixtureDef fd;
        fd.shape                = &shape;
        fd.isSensor  = true;
        s_waterTags[i].region = &wr;
        fd.userData  = &s_waterTags[i];
        body->CreateFixture(&fd);
    }

    // Clear contact list
    for (int i = 0; i < MAX_WATER_CONTACTS; i++)
        s_waterContacts[i].active = false;
}

void DestroyWaterRegions() {
    g_numWaterRegions = 0;
    for (int i = 0; i < MAX_WATER_CONTACTS; i++)
        s_waterContacts[i].active = false;
    // Water sensor bodies are cleaned up when g_groundBody is destroyed in LoadLevel
}

// ── Water heightfield simulation ─────────────────────────────────────────
// Classic spring-column wave propagation:
//   each column acts like a spring (restoring force toward h=0)
//   neighbouring columns exchange velocity (wave propagation)
// Called every frame from main Update loop.
// Forward shim — calls the real SplashWater defined below
static void SplashWaterForward(float worldX, float splashMag) {
    // Inline the splash directly here to avoid forward-declare complications
    for (int ri = 0; ri < g_numWaterRegions; ri++) {
        WaterRegion& wr = g_waterRegions[ri];
        if (!wr.active) continue;
        if (worldX < wr.x || worldX > wr.right()) continue;
        float colW = wr.width / (float)wr.numCols;
        int   ci   = (int)((worldX - wr.x) / colW);
        for (int dc = -2; dc <= 2; dc++) {
            int cc = ci + dc;
            if (cc < 0 || cc >= wr.numCols) continue;
            float falloff = 1.0f - fabsf((float)dc) * 0.3f;
            wr.cols[cc].vel -= splashMag * falloff * 10.0f; // vel-only
        }
    }
}

// Deterministic pseudo-random using integer arithmetic — no stdlib needed
static unsigned int s_waveRand = 12345;
static unsigned int WaveRandNext() {
    s_waveRand = s_waveRand * 1664525u + 1013904223u;
    return s_waveRand;
}

void UpdateWaterHeightfield(float dt) {
    // Wave physics constants:
    //   SPRING_K  — lower = slower oscillation, longer wavelength, travels further
    //   DAMPING   — closer to 1.0 = waves persist longer before dying
    //   SPREAD    — higher = faster lateral propagation across pool
    //   SPREAD_PASSES — extra propagation passes per frame for faster travel speed
    const float SPRING_K     = 2.5f;   // very gentle — long slow ocean-like waves
    const float DAMPING      = 0.9995f; // near-lossless — big splashes last 8-10s
    const float SPREAD       = 0.45f;  // strong coupling — waves travel quickly
    const int   SPREAD_PASSES = 3;     // multiple passes per frame = faster travel

    // Ambient wave driver: every ~0.5s nudge a few random columns
    // This keeps the water alive and creates the gentle constant swell
    // that can occasionally launch the boat
    static float s_ambientTimer = 0.0f;
    s_ambientTimer += dt;

    for (int ri = 0; ri < g_numWaterRegions; ri++) {
        WaterRegion& wr = g_waterRegions[ri];
        if (!wr.active || wr.numCols < 2) continue;

        // Ambient impulse — stronger on boat level for more dramatic waves
        float ambientMag = g_isBoatLevel ? 0.06f : 0.015f;
        if (s_ambientTimer >= 0.18f) {
            // Nudge 2-3 columns at random positions
            int col1 = (int)(WaveRandNext() % (unsigned int)wr.numCols);
            int col2 = (int)(WaveRandNext() % (unsigned int)wr.numCols);
            float sign1 = (WaveRandNext() & 1) ? 1.0f : -1.0f;
            float sign2 = (WaveRandNext() & 1) ? 1.0f : -1.0f;
            wr.cols[col1].vel += sign1 * ambientMag;
            wr.cols[col2].vel += sign2 * ambientMag;
        }

        // Spring + damping
        for (int c = 0; c < wr.numCols; c++) {
            float force    = -SPRING_K * wr.cols[c].h;
            wr.cols[c].vel = (wr.cols[c].vel + force * dt) * DAMPING;
            wr.cols[c].h  += wr.cols[c].vel * dt;
        }

        // Soft floor clamp: columns can't go below -0.35 but vel is preserved
        // so the wave energy continues propagating outward rather than dying.
        for (int c = 0; c < wr.numCols; c++) {
            if (wr.cols[c].h < -0.35f) {
                wr.cols[c].h = -0.35f;
                // Reflect rather than kill — negative vel becomes slightly positive
                // creating the upward surge after a deep trough (realistic rebound)
                if (wr.cols[c].vel < 0.0f)
                    wr.cols[c].vel = -wr.cols[c].vel * 0.3f;
            }
            // Soft ceiling: prevents runaway positive displacement
            if (wr.cols[c].h > 0.5f) {
                wr.cols[c].h = 0.5f;
                if (wr.cols[c].vel > 0.0f)
                    wr.cols[c].vel = -wr.cols[c].vel * 0.3f;
            }
        }

        // Wave propagation — multiple passes per frame for faster travel
        for (int pass = 0; pass < SPREAD_PASSES; pass++) {
            for (int c = 1; c < wr.numCols; c++) {
                float diff = SPREAD * (wr.cols[c].h - wr.cols[c-1].h);
                wr.cols[c-1].vel += diff;
                wr.cols[c  ].vel -= diff;
            }
            for (int c = wr.numCols - 2; c >= 0; c--) {
                float diff = SPREAD * (wr.cols[c].h - wr.cols[c+1].h);
                wr.cols[c+1].vel += diff;
                wr.cols[c  ].vel -= diff;
            }
        }
    }

    // Reset ambient timer after nudge
    if (s_ambientTimer >= 0.18f)
        s_ambientTimer = 0.0f;
}

// Splash: displace columns under worldX by splashHeight.
// Call when a body enters water to create an impact ripple.
void SplashWater(float worldX, float splashHeight) {
    for (int ri = 0; ri < g_numWaterRegions; ri++) {
        WaterRegion& wr = g_waterRegions[ri];
        if (!wr.active) continue;
        if (worldX < wr.x || worldX > wr.right()) continue;

        float colW  = wr.width / wr.numCols;
        int   ci    = (int)((worldX - wr.x) / colW);
        // Disturb columns proportional to splash magnitude:
        //   small splashes: ±3 columns (car wheel)
        //   big splashes:   ±6 columns (boat landing)
        int radius = (splashHeight > 0.5f) ? 8 : 4;
        for (int dc = -radius; dc <= radius; dc++) {
            int cc = ci + dc;
            if (cc < 0 || cc >= wr.numCols) continue;
            float falloff = 1.0f - fabsf((float)dc) / (float)(radius + 1);
            if (falloff < 0.0f) falloff = 0.0f;
            // Inject ONLY into velocity — not height.
            // Setting h directly kills the wave; energy in vel propagates naturally.
            wr.cols[cc].vel -= splashHeight * falloff * 12.0f;
        }
    }
}

//============================================================================================
// iforce2d Buoyancy — ported from Chris Campbell (www.iforce2d.net)
// Uses Sutherland-Hodgman polygon clipping to find exact submerged area,
// applies force at true centroid, plus per-edge drag and lift.
// Fixed arrays used throughout (Xbox 360 / C++03 safe — no STL heap).
// Max polygon vertices = 8 (Box2D limit), max intersection points = 16.
//============================================================================================

#define BUOY_MAX_VERTS 16  // max vertices in intersection polygon

static bool BuoyInside(b2Vec2 cp1, b2Vec2 cp2, b2Vec2 p) {
    return (cp2.x - cp1.x) * (p.y - cp1.y) > (cp2.y - cp1.y) * (p.x - cp1.x);
}

static b2Vec2 BuoyIntersect(b2Vec2 cp1, b2Vec2 cp2, b2Vec2 s, b2Vec2 e) {
    b2Vec2 dc(cp1.x - cp2.x, cp1.y - cp2.y);
    b2Vec2 dp(s.x - e.x,     s.y - e.y);
    float n1 = cp1.x * cp2.y - cp1.y * cp2.x;
    float n2 = s.x   * e.y   - s.y   * e.x;
    float n3 = 1.0f  / (dc.x * dp.y - dc.y * dp.x);
    return b2Vec2((n1 * dp.x - n2 * dc.x) * n3,
                  (n1 * dp.y - n2 * dc.y) * n3);
}

// Sutherland-Hodgman clipping — finds intersection polygon of two Box2D fixtures.
// Both must be polygon shapes. Returns vertex count (0 = no intersection).
static int FindIntersectionOfFixtures(b2Fixture* fA, b2Fixture* fB,
                                      b2Vec2* out, int outMax) {
    if (fA->GetShape()->GetType() != b2Shape::e_polygon) return 0;
    if (fB->GetShape()->GetType() != b2Shape::e_polygon) return 0;

    b2PolygonShape* polyA = (b2PolygonShape*)fA->GetShape();
    b2PolygonShape* polyB = (b2PolygonShape*)fB->GetShape();

    // Subject polygon = fluid fixture (fA) world vertices
    int outCount = polyA->GetVertexCount();
    if (outCount > outMax) return 0;
    for (int i = 0; i < outCount; i++)
        out[i] = fA->GetBody()->GetWorldPoint(polyA->GetVertex(i));

    // Clip against each edge of fB
    b2Vec2 input[BUOY_MAX_VERTS];
    int clipCount = polyB->GetVertexCount();
    b2Vec2 cp1 = fB->GetBody()->GetWorldPoint(polyB->GetVertex(clipCount - 1));

    for (int j = 0; j < clipCount && outCount > 0; j++) {
        b2Vec2 cp2 = fB->GetBody()->GetWorldPoint(polyB->GetVertex(j));

        // Copy output to input
        int inCount = outCount;
        for (int k = 0; k < inCount; k++) input[k] = out[k];
        outCount = 0;

        b2Vec2 s = input[inCount - 1];
        for (int i = 0; i < inCount; i++) {
            b2Vec2 e = input[i];
            if (BuoyInside(cp1, cp2, e)) {
                if (!BuoyInside(cp1, cp2, s) && outCount < outMax)
                    out[outCount++] = BuoyIntersect(cp1, cp2, s, e);
                if (outCount < outMax)
                    out[outCount++] = e;
            } else if (BuoyInside(cp1, cp2, s) && outCount < outMax) {
                out[outCount++] = BuoyIntersect(cp1, cp2, s, e);
            }
            s = e;
        }
        cp1 = cp2;
    }
    return outCount;
}

// Area-weighted centroid of polygon (identical to iforce2d ComputeCentroid).
static b2Vec2 BuoyCentroid(b2Vec2* vs, int count, float& area) {
    b2Vec2 c(0.0f, 0.0f);
    area = 0.0f;
    if (count < 3) return c;
    const float inv3 = 1.0f / 3.0f;
    for (int i = 0; i < count; i++) {
        b2Vec2 p1(0.0f, 0.0f);
        b2Vec2 p2 = vs[i];
        b2Vec2 p3 = vs[(i + 1) % count];
        b2Vec2 e1 = p2 - p1, e2 = p3 - p1;
        float D = b2Cross(e1, e2);
        float triArea = 0.5f * D;
        area += triArea;
        c += triArea * inv3 * (p1 + p2 + p3);
    }
    if (area > b2_epsilon) c *= 1.0f / area;
    else area = 0.0f;
    return c;
}

// ── Per-step buoyancy application ────────────────────────────────────────
// Called once per frame after Box2D Step().
// Applies upward buoyancy force and velocity drag to each submerged body.
void ApplyWaterForces() {
    // HCR-style water: car drives through with resistance, doesn't float or flip.
    //
    // Key insight: Y drag is the main cause of flipping. When the front wheel
    // enters and gets a sudden upward deceleration, it nose-dives. Setting Y drag
    // to zero lets gravity do the work naturally — the car just settles onto the
    // terrain floor at the bottom of the pool.
    //
    // Angular drag only counteracts spin that ALREADY EXISTS — it never adds new
    // torque, so it can't cause a flip.
    const float WATER_DENSITY  = 0.06f;   // very light — car sinks but can drive out easily
    const float LINEAR_DRAG_X  = 0.8f;   // low drag — wheels maintain traction to exit
    const float LINEAR_DRAG_Y  = 0.0f;   // no vertical drag — no flip on entry
    const float ANGULAR_DRAG   = 5.0f;   // damps tipping
    const float GRAVITY_MAG    = -GRAVITY;

    // ── Boat level: proper buoyancy + wave physics ──────────────────────────
    // Approach: direct surface-height test against hull, no sensor drift.
    // The sensor body is kept at a FIXED absolute position — never drifted.
    // Buoyancy is computed by how much of the hull sits below restY + waveH.
    if (g_isBoatLevel) {
        if (!g_carChassis) return;

        // Find the active water region
        WaterRegion* region = NULL;
        for (int ri = 0; ri < g_numWaterRegions; ri++) {
            if (g_waterRegions[ri].active) { region = &g_waterRegions[ri]; break; }
        }
        if (!region) return;

        // Ensure sensor body is at the CORRECT absolute position (never drift)
        // Sensor centre Y = restY - depth*0.80*0.5 (half-height below restY)
        if (region->sensorBody) {
            float sensorHalfH = region->depth * 0.80f * 0.5f;
            float sensorCY    = region->restY - sensorHalfH;
            float sensorCX    = region->x + region->width * 0.5f;
            region->sensorBody->SetTransform(b2Vec2(sensorCX, sensorCY), 0.0f);
        }

        b2Vec2 chassisPos = g_carChassis->GetPosition();
        float  mass       = g_carChassis->GetMass();

        // ── Get wave height at boat position ──────────────────────────────────
        float colWW = region->width / (float)region->numCols;
        int   colIdx = (int)((chassisPos.x - region->x) / colWW);
        colIdx = colIdx < 0 ? 0 : (colIdx >= region->numCols ? region->numCols-1 : colIdx);
        float waveH  = region->cols[colIdx].h;
        // Live water surface at boat X
        float surfaceY = region->restY + waveH;

        // ── Buoyancy — proportional to how deep hull keel is below surface ────
        // Get hull AABB to estimate submersion depth
        b2AABB hullAABB;
        bool first = true;
        for (b2Fixture* f = g_carChassis->GetFixtureList(); f; f = f->GetNext()) {
            b2AABB fa;
            f->GetShape()->ComputeAABB(&fa, g_carChassis->GetTransform(), 0);
            if (first) { hullAABB = fa; first = false; }
            else {
                hullAABB.lowerBound.x = Xbox360Min(hullAABB.lowerBound.x, fa.lowerBound.x);
                hullAABB.lowerBound.y = Xbox360Min(hullAABB.lowerBound.y, fa.lowerBound.y);
                hullAABB.upperBound.x = Xbox360Max(hullAABB.upperBound.x, fa.upperBound.x);
                hullAABB.upperBound.y = Xbox360Max(hullAABB.upperBound.y, fa.upperBound.y);
            }
        }
        if (first) return;

        float hullH    = hullAABB.upperBound.y - hullAABB.lowerBound.y;
        float subDepth = surfaceY - hullAABB.lowerBound.y; // how deep keel is below surface
        if (subDepth < 0.0f) subDepth = 0.0f;              // hull fully above water
        if (subDepth > hullH) subDepth = hullH;            // hull fully submerged
        float subFrac  = subDepth / hullH;                  // 0=dry, 1=fully submerged

        // Hull area (approximated from AABB)
        float hullArea = (hullAABB.upperBound.x - hullAABB.lowerBound.x) * hullH;

        // ── WATER_DENSITY sized so boat floats at ~45% submersion ─────────────
        // Neutral float: DENSITY * hullArea * 0.45 * gravity = mass * gravity
        // So DENSITY = mass / (hullArea * 0.45)
        // Using mass from Box2D ensures it always matches regardless of shape changes.
        const float GRAVITY_MAG   = -GRAVITY;
        const float TARGET_SUBMERSION = 0.45f;
        float WATER_DENSITY = mass / (hullArea * TARGET_SUBMERSION);

        float subArea      = hullArea * subFrac;
        float displacedMass = WATER_DENSITY * subArea;
        b2Vec2 buoyPoint(chassisPos.x, hullAABB.lowerBound.y + subDepth * 0.5f);
        g_carChassis->ApplyForce(b2Vec2(0.0f, displacedMass * GRAVITY_MAG), buoyPoint, true);

        // ── Y velocity damping — stops oscillation, feels like water resistance ─
        b2Vec2 vel = g_carChassis->GetLinearVelocity();
        float yDamp = -vel.y * mass * 3.5f * subFrac;
        g_carChassis->ApplyForce(b2Vec2(0.0f, yDamp), chassisPos, true);

        // ── X drag — slows forward motion in water ────────────────────────────
        float xDrag = -vel.x * 1.2f * subFrac;
        g_carChassis->ApplyForce(b2Vec2(xDrag, 0.0f), chassisPos, true);

        // ── Angular damping — damps rotation but allows tilt ──────────────────
        float angVel = g_carChassis->GetAngularVelocity();
        g_carChassis->ApplyTorque(-angVel * mass * 2.0f * subFrac, true);

        // ── Wave coupling — boat rises/falls with wave surface ────────────────
        // Apply vertical impulse proportional to wave height × mass
        g_carChassis->ApplyForce(b2Vec2(0.0f, waveH * mass * 5.0f), buoyPoint, true);
        // Rocking from wave slope between adjacent columns
        int colL = colIdx > 0 ? colIdx-1 : 0;
        int colR = colIdx < region->numCols-1 ? colIdx+1 : region->numCols-1;
        float slope = region->cols[colR].h - region->cols[colL].h;
        g_carChassis->ApplyTorque(slope * mass * 2.0f, true);

        return;
    }

    for (int ci = 0; ci < MAX_WATER_CONTACTS; ci++) {
        if (!s_waterContacts[ci].active) continue;

        b2Body*      body   = s_waterContacts[ci].body;
        WaterRegion* region = s_waterContacts[ci].region;

        if (!body || !region) continue;

        b2Vec2 pos     = body->GetPosition();
        b2AABB bodyAABB;

        // Build approximate body AABB from fixtures
        bool first = true;
        for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()) {
            b2AABB fa;
            f->GetShape()->ComputeAABB(&fa, body->GetTransform(), 0);
            if (first) { bodyAABB = fa; first = false; }
            else {
                bodyAABB.lowerBound.x = Xbox360Min(bodyAABB.lowerBound.x, fa.lowerBound.x);
                bodyAABB.lowerBound.y = Xbox360Min(bodyAABB.lowerBound.y, fa.lowerBound.y);
                bodyAABB.upperBound.x = Xbox360Max(bodyAABB.upperBound.x, fa.upperBound.x);
                bodyAABB.upperBound.y = Xbox360Max(bodyAABB.upperBound.y, fa.upperBound.y);
            }
        }
        if (first) continue; // no fixtures

        // Clamp AABB to water region
        float subTop    = Xbox360Min(bodyAABB.upperBound.y, region->restY);
        float subBottom = Xbox360Max(bodyAABB.lowerBound.y, region->bottom());
        float subLeft   = Xbox360Max(bodyAABB.lowerBound.x, region->x);
        float subRight  = Xbox360Min(bodyAABB.upperBound.x, region->right());

        float subW = subRight - subLeft;
        float subH = subTop   - subBottom;
        if (subW <= 0.0f || subH <= 0.0f) continue;

        float submergedArea = subW * subH;
        float totalArea     = (bodyAABB.upperBound.x - bodyAABB.lowerBound.x)
                            * (bodyAABB.upperBound.y - bodyAABB.lowerBound.y);
        if (totalArea <= 0.0f) continue;
        float fraction = submergedArea / totalArea;

        // ── Buoyancy: F = rho * g * V (upward) ──────────────────────────
        // submergedArea is our 2D "volume proxy"
        float buoyancyMag = WATER_DENSITY * GRAVITY_MAG * submergedArea;
        b2Vec2 buoyancyForce(0.0f, buoyancyMag);

        // Apply at centroid of submerged region
        b2Vec2 subCentroid((subLeft + subRight) * 0.5f, (subBottom + subTop) * 0.5f);
        body->ApplyForce(buoyancyForce, subCentroid, true);

        // ── Drag: wheels must spin freely so they can push car out ─────
        // Only apply X drag to chassis — wheel bodies get no drag so motor
        // torque remains effective. Angular drag on wheels kills traction.
        b2Vec2 vel = body->GetLinearVelocity();
        bool isWheel = (body == g_wheelRear || body == g_wheelFront);
        if (!isWheel) {
            // Chassis only: light horizontal drag, no vertical, no angular
            b2Vec2 dragForce(-vel.x * LINEAR_DRAG_X * fraction, 0.0f);
            body->ApplyForce(dragForce, pos, true);
        }
        // Angular drag only on chassis to prevent tipping — NOT on wheels
        if (!isWheel) {
            float angVel = body->GetAngularVelocity();
            body->ApplyTorque(-angVel * ANGULAR_DRAG * fraction, true);
        }
    }
}


// ── Level water setup ─────────────────────────────────────────────────────
// Initialise a water region at a fixed X position.
// The terrain valley is carved by terrain.cpp using the same region data,
// so restY is set AFTER GenerateWrappingTerrain() has been called.
// We call this function twice: once before terrain gen (to register positions
// so terrain carves valleys), and InitWaterSurface() after (to set restY).
static void RegisterWaterRegion(int idx, float startX, float width, float depth) {
    WaterRegion& wr = g_waterRegions[idx];
    wr.x      = startX;
    wr.width  = width;
    wr.depth  = depth;
    wr.restY  = 0.0f;   // set later by InitWaterSurface
    wr.active     = true;
    wr.sensorBody = NULL;

    int nc = (int)(width / WATER_COL_WIDTH);
    if (nc > WATER_COLS_MAX) nc = WATER_COLS_MAX;
    if (nc < 2) nc = 2;
    wr.numCols = nc;
    for (int c = 0; c < nc; c++) {
        wr.cols[c].h   = 0.0f;
        wr.cols[c].vel = 0.0f;
    }
}

// Called AFTER GenerateWrappingTerrain() so terrain heights are valid.
// Finds the lowest point of the carved valley and sets restY just above it.
static void InitWaterSurface() {
    for (int wi = 0; wi < g_numWaterRegions; wi++) {
        WaterRegion& wr = g_waterRegions[wi];
        if (!wr.active) continue;
        // Sample the flat floor section (skip walls: inner 70% of span)
        float wallW  = wr.width * WATER_VALLEY_WALL;
        float floorL = wr.x + wallW;
        float floorR = wr.x + wr.width - wallW;
        float lowest = GetTerrainHeightAtX(floorL);
        for (float sx = floorL; sx <= floorR; sx += SEGMENT_LENGTH * 2.0f) {
            float h = GetTerrainHeightAtX(sx);
            if (h < lowest) lowest = h;
        }
        // Set waterline to fill ~80% of the carved valley depth.
        // 'lowest' = valley floor, terrain at the valley rim = lowest + WATER_VALLEY_DEPTH.
        // Waterline at 80% fills the bowl visually while leaving a little wall above water.
        // Use wr.depth (per-region depth) not the global constant —
        // Level 7 has depth 4.5 which is different from the default 2.0
        wr.restY = lowest + wr.depth * 0.80f;
    }
}

void SetLevelWater(Level level) {
    // Clear all regions first
    g_numWaterRegions = 0;
    g_isBoatLevel   = (level == LEVEL_7);
    for (int i = 0; i < MAX_WATER_REGIONS; i++) {
        g_waterRegions[i].active     = false;
        g_waterRegions[i].sensorBody = NULL;  // prevent dangling pointer after world rebuild
    }

    // Register water pool positions.
    // These same positions are read by terrain.cpp to carve the valleys.
    // Positions chosen to be away from the bridge sections on each level.
    if (level == LEVEL_1) {
        // Level 1: one puddle — second one removed (was broken)
        RegisterWaterRegion(0, 75.0f, WATER_SPAN, WATER_VALLEY_DEPTH);
        g_numWaterRegions = 1;
    }
    // Levels 2-6: no water — dry terrain only
    else if (level == LEVEL_7) {
        // Water level — pool starts well ahead of spawn so boat launches onto water.
        // CAR_START_X = 12, boat half-width ~2.25 → safe to start at X=8.
        // Water starts at X=20 so boat is already floating from the first frame.
        // Depth=4.5 with 80% fill = surface is high enough to be clearly visible.
        RegisterWaterRegion(0, 15.0f, 420.0f, 4.5f);  // ends at X=435, before level cutoff
        g_numWaterRegions = 1;
    }
    // Note: InitWaterSurface() and CreateWaterRegions() are called from
    // LoadLevel AFTER GenerateWrappingTerrain() so heights are valid.
}


