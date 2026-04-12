#include "stdafx.h"
#include "bridge.h"
#include "terrain.h"

//============================================================================================
// ✅ BRIDGE PHYSICS - CREATE / DESTROY
//============================================================================================

void DestroyBridge() {
    if (!g_bridgeActive || g_pBox2DWorld == NULL) return;

    // Destroying a body in Box2D automatically destroys all its joints - clear pointers only
    for (int i = 0; i <= BRIDGE_PLANK_COUNT; i++) {
        g_bridgeJoints[i] = NULL;
    }
    for (int i = 0; i < BRIDGE_PLANK_COUNT; i++) {
        if (g_bridgePlanks[i] != NULL) {
            g_pBox2DWorld->DestroyBody(g_bridgePlanks[i]);
            g_bridgePlanks[i] = NULL;
        }
    }
    if (g_bridgeAnchorL != NULL) {
        g_pBox2DWorld->DestroyBody(g_bridgeAnchorL);
        g_bridgeAnchorL = NULL;
    }
    if (g_bridgeAnchorR != NULL) {
        g_pBox2DWorld->DestroyBody(g_bridgeAnchorR);
        g_bridgeAnchorR = NULL;
    }

    g_bridgeActive = false;
}

void CreateBridge(float startX) {
    if (g_pBox2DWorld == NULL) return;

    // ✅ Anchors sit at the CLIFF LIPS (exactly startX and startX + BRIDGE_SPAN)
    //    This matches HCR: anchors are at the highest point of each cliff edge,
    //    and the bridge hangs/sags below from gravity.
    //    BRIDGE_ANCHOR_HEIGHT lifts the attachment point above the cliff surface.
    float anchorLX = startX;
    float anchorRX = startX + BRIDGE_SPAN;
    float terrainL = GetTerrainHeightAtX(anchorLX);
    float terrainR = GetTerrainHeightAtX(anchorRX);
    float anchorLY = terrainL + BRIDGE_ANCHOR_HEIGHT;
    float anchorRY = terrainR + BRIDGE_ANCHOR_HEIGHT;

    // ── Static anchor posts at the cliff lips ────────────────────────────────
    b2BodyDef anchorLDef;
    anchorLDef.type = b2_staticBody;
    anchorLDef.position.Set(anchorLX, anchorLY);
    g_bridgeAnchorL = g_pBox2DWorld->CreateBody(&anchorLDef);

    b2BodyDef anchorRDef;
    anchorRDef.type = b2_staticBody;
    anchorRDef.position.Set(anchorRX, anchorRY);
    g_bridgeAnchorR = g_pBox2DWorld->CreateBody(&anchorRDef);

    // ── Plank shape: slightly wider than slot to overlap neighbours ─────────
    // Overlap factor > 1.0 means each plank is wider than its slot, so there
    // is no physical gap for the car wheel to fall into between planks.
    float plankW    = BRIDGE_SPAN / (float)BRIDGE_PLANK_COUNT;
    float plankWPhys = plankW * 1.03f;  // 3% overlap — with 20 planks and tight limits, minimal gap risk
    b2PolygonShape plankShape;
    plankShape.SetAsBox(plankWPhys * 0.5f, BRIDGE_PLANK_HEIGHT * 0.5f);

    b2FixtureDef plankFix;
    plankFix.shape             = &plankShape;
    plankFix.density           = BRIDGE_PLANK_DENSITY;
    plankFix.friction          = BRIDGE_PLANK_FRICTION;
    plankFix.restitution       = BRIDGE_PLANK_RESTITUTION;
    plankFix.filter.groupIndex = -1;  // planks don't collide with each other

    // ── Create planks with angular damping for stability under heavy loads ───
    // linearDamping:  resists translational oscillation (prevents bounce)
    // angularDamping: resists rotation — key to stopping planks separating
    //                 under a heavy car. Higher = stiffer feeling bridge.
    for (int i = 0; i < BRIDGE_PLANK_COUNT; i++) {
        float t      = ((float)i + 0.5f) / (float)BRIDGE_PLANK_COUNT;
        float plankX = anchorLX + t * BRIDGE_SPAN;
        float plankY = anchorLY + t * (anchorRY - anchorLY);

        b2BodyDef plankDef;
        plankDef.type           = b2_dynamicBody;
        plankDef.position.Set(plankX, plankY);
        plankDef.linearDamping  = 2.0f;   // strong vertical damping — no bounce
        plankDef.angularDamping = 8.0f;   // very high — bridge holds rigid under load

        g_bridgePlanks[i] = g_pBox2DWorld->CreateBody(&plankDef);
        g_bridgePlanks[i]->CreateFixture(&plankFix);
    }

    // ── Pure revolute chain — HCR-style rope bridge ──────────────────────────
    //
    //  Every joint is a free-pivoting hinge (revolute). With 20 short planks
    //  the chain forms a smooth catenary curve under gravity — no visible steps.
    //  This is exactly how HCR implements its bridge physics.
    //
    //  Rotation limits: ±20° per joint. Tight enough to prevent V-gaps under
    //  a car wheel but loose enough to let the chain curve naturally.
    //  The heavy plank damping (set above) stops oscillation under load.

    // Joint 0: left anchor <-> plank[0]
    {
        b2RevoluteJointDef rjd;
        rjd.Initialize(g_bridgeAnchorL, g_bridgePlanks[0],
                       b2Vec2(anchorLX, anchorLY));
        rjd.collideConnected = false;
        rjd.enableLimit      = true;
        rjd.lowerAngle       = -0.35f;  // -20 degrees
        rjd.upperAngle       =  0.35f;
        g_bridgeJoints[0] = g_pBox2DWorld->CreateJoint(&rjd);
    }

    // Joints 1..N-1: plank[i-1] <-> plank[i] — pivot at shared edge
    for (int i = 1; i < BRIDGE_PLANK_COUNT; i++) {
        float jt     = (float)i / (float)BRIDGE_PLANK_COUNT;
        float jointX = anchorLX + jt * BRIDGE_SPAN;
        float jointY = anchorLY + jt * (anchorRY - anchorLY);

        b2RevoluteJointDef rjd;
        rjd.Initialize(g_bridgePlanks[i - 1], g_bridgePlanks[i],
                       b2Vec2(jointX, jointY));
        rjd.collideConnected = false;
        rjd.enableLimit      = true;
        rjd.lowerAngle       = -0.35f;
        rjd.upperAngle       =  0.35f;
        g_bridgeJoints[i] = g_pBox2DWorld->CreateJoint(&rjd);
    }

    // Joint N: plank[N-1] <-> right anchor
    {
        b2RevoluteJointDef rjd;
        rjd.Initialize(g_bridgePlanks[BRIDGE_PLANK_COUNT - 1], g_bridgeAnchorR,
                       b2Vec2(anchorRX, anchorRY));
        rjd.collideConnected = false;
        rjd.enableLimit      = true;
        rjd.lowerAngle       = -0.35f;
        rjd.upperAngle       =  0.35f;
        g_bridgeJoints[BRIDGE_PLANK_COUNT] = g_pBox2DWorld->CreateJoint(&rjd);
    }

    g_bridgeActive = true;
}

//============================================================================================
// ✅ BRIDGE RENDERING
//============================================================================================

void RenderBridge() {
    if (!g_bridgeActive) return;
    if (g_pTexBridgePlank == NULL) return;

    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetTexture(0, g_pTexBridgePlank);

    for (int i = 0; i < BRIDGE_PLANK_COUNT; i++) {
        if (g_bridgePlanks[i] == NULL) continue;

        b2Vec2 pos    = g_bridgePlanks[i]->GetPosition();
        float  angle  = g_bridgePlanks[i]->GetAngle();
        float  screenX = pos.x - g_fScrollPos;

        D3DXMATRIX matRot, matTrans, matScale, matResult;
        float visualPlankW = (BRIDGE_SPAN / (float)BRIDGE_PLANK_COUNT) * BRIDGE_VISUAL_WIDTH_SCALE;
        // Z = -0.08f: drawn in front of terrain (z=0) but behind car (z=-0.1f)
        D3DXMatrixScaling(&matScale,      visualPlankW, BRIDGE_VISUAL_HEIGHT_SCALE, 1.0f);
        D3DXMatrixRotationZ(&matRot,      angle);
        D3DXMatrixTranslation(&matTrans,  screenX, pos.y + BRIDGE_VISUAL_Y_OFFSET, -0.08f);
        matResult = matScale * matRot * matTrans * g_matView * g_matProj;
        g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matResult, 4);

        TEXVERTEX plankQuad[4];
        plankQuad[0].x = -1.0f; plankQuad[0].y =  1.0f; plankQuad[0].z = 0; plankQuad[0].u = 0; plankQuad[0].v = 0;
        plankQuad[1].x =  1.0f; plankQuad[1].y =  1.0f; plankQuad[1].z = 0; plankQuad[1].u = 1; plankQuad[1].v = 0;
        plankQuad[2].x = -1.0f; plankQuad[2].y = -1.0f; plankQuad[2].z = 0; plankQuad[2].u = 0; plankQuad[2].v = 1;
        plankQuad[3].x =  1.0f; plankQuad[3].y = -1.0f; plankQuad[3].z = 0; plankQuad[3].u = 1; plankQuad[3].v = 1;
        g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, plankQuad, sizeof(TEXVERTEX));
    }
}

//============================================================================================
