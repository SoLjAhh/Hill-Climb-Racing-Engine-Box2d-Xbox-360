#include "stdafx.h"
#include "terrain.h"
#include "bridge.h"

//============================================================================================
// TERRAIN HEIGHT LOOKUP
//============================================================================================

float GetTerrainHeightAtX(float worldX) {
    if (g_terrainSegments.size() == 0) return 0.0f;
    
    float wrappedX = worldX;
    while (wrappedX < 0.0f) wrappedX += TERRAIN_WRAP_LENGTH;
    while (wrappedX >= TERRAIN_WRAP_LENGTH) wrappedX -= TERRAIN_WRAP_LENGTH;
    
    int segIndex = (int)(wrappedX / SEGMENT_LENGTH);
    if (segIndex < 0) segIndex = 0;
    if (segIndex >= (int)g_terrainSegments.size()) {
        segIndex = (int)g_terrainSegments.size() - 1;
    }
    
    if (segIndex >= 0 && segIndex < (int)g_terrainSegments.size()) {
        const TerrainSegment& seg = g_terrainSegments[segIndex];
        
        float segmentSpan = seg.x2 - seg.x1;
        if (segmentSpan <= 0.0f) segmentSpan = SEGMENT_LENGTH;
        
        float segLocalX = wrappedX - seg.x1;
        float t = segLocalX / segmentSpan;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        
        return seg.y1 + t * (seg.y2 - seg.y1);
    }
    
    return 0.0f;
}

//============================================================================================

float GenerateTerrainHeightProcedural(float worldX) {
    float amplitude = TERRAIN_AMPLITUDE;
    float frequency = TERRAIN_FREQUENCY_BASE;
    
    if (g_currentLevel == LEVEL_7) {
        // Water level — nearly flat terrain, boat glides across
        amplitude = 0.3f;
        frequency = 0.05f;
    }
    else if (g_currentLevel == LEVEL_2) {
        amplitude = TERRAIN_AMPLITUDE_L2;
        frequency = TERRAIN_FREQUENCY_BASE_L2;   
    }
    else if (g_currentLevel == LEVEL_3) {
        amplitude = TERRAIN_AMPLITUDE_L3;
        frequency = TERRAIN_FREQUENCY_BASE_L3;
    }
    else if (g_currentLevel == LEVEL_4) {
        amplitude = TERRAIN_AMPLITUDE_L4;
        frequency = TERRAIN_FREQUENCY_BASE_L4;
    }
    else if (g_currentLevel == LEVEL_5) {
        amplitude = TERRAIN_AMPLITUDE_L5;
        frequency = TERRAIN_FREQUENCY_BASE_L5;
    }
    else if (g_currentLevel == LEVEL_6) {
        amplitude = TERRAIN_AMPLITUDE_L6;
        frequency = TERRAIN_FREQUENCY_BASE_L6;
    }
    
    // ✅ Smooth HCR-style hills: two low-frequency base waves + tiny texture detail
    //    wave1 + wave2 create broad rolling hills, wave3 adds subtle surface variation
    //    High-frequency wave4 removed - it was causing jagged, artificial-looking terrain
    float wave1 = sinf(worldX * frequency)             * amplitude * 1.0f;
    float wave2 = sinf(worldX * (frequency * 0.43f))   * amplitude * 0.55f;
    float wave3 = cosf(worldX * (frequency * 1.3f))    * amplitude * 0.12f;
    float wave4 = sinf(worldX * (frequency * 0.21f))   * amplitude * 0.35f;

    // ✅ Custom level override: use its amplitude and frequency
    if (g_currentLevel >= LEVEL_CUSTOM_1) {
        int ci = g_currentLevel - (int)LEVEL_CUSTOM_1;
        if (ci >= 0 && ci < g_numCustomLevels && g_customLevels[ci].loaded) {
            float ca = g_customLevels[ci].amplitude;
            float cf = g_customLevels[ci].frequencyBase;
            float cw1 = sinf(worldX * cf)         * ca * 1.2f;
            float cw2 = sinf(worldX * (cf * 0.5f))* ca * 0.8f;
            float cw3 = sinf(worldX * (cf * 1.67f))* ca * 0.4f;
            float cw4 = sinf(worldX * (cf * 2.67f))* ca * 0.2f;
            amplitude = ca; frequency = cf;
            wave1 = cw1; wave2 = cw2; wave3 = cw3; wave4 = cw4;
        }
    }

    float height = wave1 + wave2 + wave3 + wave4;

    // ✅ Carve a HCR-style valley: steep cliff walls + flat valley floor
    //    The smoothstep function creates near-vertical walls at each edge
    //    while keeping the valley floor flat - matching the reference image exactly.
    //
    //    Shape:  ____              ____
    //           |    |            |    |   ← cliff lips (= anchor points)
    //           |    |____________|    |   ← flat valley floor
    //
    //    'wall' = fraction of span used for the cliff wall (each side)
    //    Outside the span: height is unchanged (cliff tops at full terrain height)
    {
        float bStartX = -1.0f;
        if (g_currentLevel == LEVEL_2 || g_currentLevel == LEVEL_4)
            bStartX = (g_currentLevel == LEVEL_2) ? BRIDGE_L2_START_X : BRIDGE_L4_START_X;
        else if (g_currentLevel >= LEVEL_CUSTOM_1) {
            int ci = g_currentLevel - (int)LEVEL_CUSTOM_1;
            if (ci >= 0 && ci < g_numCustomLevels && g_customLevels[ci].loaded
                && g_customLevels[ci].hasBridge > 0.0f)
                bStartX = g_customLevels[ci].bridgeStartX;
        }

        if (bStartX >= 0.0f) {
            float bEndX = bStartX + BRIDGE_SPAN;
            if (worldX >= bStartX && worldX <= bEndX) {
                float t    = (worldX - bStartX) / BRIDGE_SPAN;
                float wall = 0.14f;  // 14% of span per cliff wall (~1.7 world units each)
                float dip;
                if (t < wall) {
                    // Left cliff wall: smoothstep 0 → 1
                    float s = t / wall;
                    dip = s * s * (3.0f - 2.0f * s);
                } else if (t > 1.0f - wall) {
                    // Right cliff wall: smoothstep 1 → 0
                    float s = (1.0f - t) / wall;
                    dip = s * s * (3.0f - 2.0f * s);
                } else {
                    dip = 1.0f;  // flat valley floor
                }
                height -= dip * BRIDGE_VALLEY_DEPTH;
            }
        }
    }

    // ── Jump features ─────────────────────────────────────────────────────
    // Each jump is a Gaussian bell curve added to the terrain height.
    // Positive height = hump (car launches), negative = dip (valley/pit).
    // Multiple overlapping jumps sum together naturally.
    for (int ji = 0; ji < g_numLevelJumps; ji++) {
        const JumpDef& jd = g_levelJumps[ji];
        float dx = worldX - jd.x;
        // Gaussian: e^(-(dx/width)^2)  — peak at centre, tapers to 0 at edges
        float t  = (dx / jd.width);
        float bell = expf(-(t * t));
        height += jd.height * bell;
    }

    // ── Water valley carve ───────────────────────────────────────────────────
    // Same smoothstep technique as bridge valleys but shallower.
    // Each active water region defines a carved pool section.
    for (int wi = 0; wi < g_numWaterRegions; wi++) {
        const WaterRegion& wr = g_waterRegions[wi];
        if (!wr.active) continue;
        float wEnd = wr.x + wr.width;
        if (worldX >= wr.x && worldX <= wEnd) {
            float t = (worldX - wr.x) / wr.width;
            float dip;
            if (t < WATER_VALLEY_WALL) {
                float s = t / WATER_VALLEY_WALL;
                dip = s * s * (3.0f - 2.0f * s);        // smoothstep slope in
            } else if (t > 1.0f - WATER_VALLEY_WALL) {
                float s = (1.0f - t) / WATER_VALLEY_WALL;
                dip = s * s * (3.0f - 2.0f * s);        // smoothstep slope out
            } else {
                dip = 1.0f;                              // flat pool floor
            }
            height -= dip * WATER_VALLEY_DEPTH;
        }
    }

    return height;
}

void GenerateWrappingTerrain() {
    g_terrainSegments.clear();
    
    float currentX = 0.0f;
    float currentY = GenerateTerrainHeightProcedural(currentX);
    g_terrainStartY = currentY;
    
    while (currentX < TERRAIN_WRAP_LENGTH) {
        float nextX = currentX + SEGMENT_LENGTH;
        float nextY = GenerateTerrainHeightProcedural(nextX);
        
        // ✅ No slope limiter - real HCR connects terrain points directly.
        // The limiter was preventing bridge valley walls from forming because
        // the valley carve needs slopes of ~0.9/unit but SLOPE_LIMIT was 0.2.
        // Without it, GenerateTerrainHeightProcedural() values are used as-is,
        // so bridge cliff lips are sampled at the correct heights by CreateBridge.
        
        TerrainSegment seg;
        seg.x1 = currentX; seg.y1 = currentY;
        seg.x2 = nextX; seg.y2 = nextY;
        
        g_terrainSegments.push_back(seg);
        
        if (g_pBox2DWorld != NULL && g_groundBody != NULL) {
            b2EdgeShape edge;
            edge.Set(b2Vec2(currentX, currentY), b2Vec2(nextX, nextY));
            
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &edge;
            fixtureDef.density = 0.0f;
            fixtureDef.friction = 0.6f;
            fixtureDef.restitution = 0.1f;
            
            g_groundBody->CreateFixture(&fixtureDef);
        }
        
        currentX = nextX;
        currentY = nextY;
    }
    
    g_terrainEndY = currentY;
    
    if (g_terrainSegments.size() > 0) {
        const TerrainSegment& lastSeg = g_terrainSegments.back();
        
        b2EdgeShape wrapEdge;
        wrapEdge.Set(b2Vec2(lastSeg.x2, lastSeg.y2), 
                     b2Vec2(TERRAIN_WRAP_LENGTH + SEGMENT_LENGTH, g_terrainStartY));
        
        b2FixtureDef wrapFixture;
        wrapFixture.shape = &wrapEdge;
        wrapFixture.density = 0.0f;
        wrapFixture.friction = 0.6f;
        wrapFixture.restitution = 0.1f;
        
        g_groundBody->CreateFixture(&wrapFixture);
    }
    
    b2BodyDef barrierBodyDef;
    barrierBodyDef.type = b2_staticBody;
    barrierBodyDef.position.Set(BARRIER_WALL_X, 0.0f);
    b2Body* barrierBody = g_pBox2DWorld->CreateBody(&barrierBodyDef);
    
    b2EdgeShape barrierEdge;
    barrierEdge.Set(b2Vec2(0.0f, -10.0f), b2Vec2(0.0f, 10.0f));
    
    b2FixtureDef barrierFixture;
    barrierFixture.shape = &barrierEdge;
    barrierFixture.density = 0.0f;
    barrierFixture.friction = 0.0f;
    
    barrierBody->CreateFixture(&barrierFixture);
}

//============================================================================================
// RENDERING
//============================================================================================
