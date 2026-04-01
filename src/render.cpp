#include "stdafx.h"
#include "render.h"
#include "terrain.h"
#include "cars.h"
#include "bridge.h"
#include "audio.h"
#include "physics.h"

//============================================================================================
// RENDERING
//============================================================================================

void RenderWheel(float parentX, float parentY, float localX, float localY, float carRot, float spin) {
    if (g_isBoatLevel) return;  // boat has no wheels
    if (g_isAirplane)  return;  // airplane has no driven wheels
    D3DXMATRIX matSpin, matLocalPos, matBodyRot, matWorldPos, matResult;
    D3DXMatrixRotationZ(&matSpin, spin);
    D3DXMatrixTranslation(&matLocalPos, localX, localY, -0.05f);
    D3DXMatrixRotationZ(&matBodyRot, carRot);
    D3DXMatrixTranslation(&matWorldPos, parentX, parentY, -0.1f);

    matResult = matSpin * matLocalPos * matBodyRot * matWorldPos * g_matView * g_matProj;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matResult, 4);

    TEXVERTEX v[34];
    v[0].x = 0.0f; v[0].y = 0.0f; v[0].z = 0.0f; v[0].u = 0.5f; v[0].v = 0.5f;
    for(int i = 0; i <= CIRCLE_SEGMENTS; i++) {
        float t = (2.0f * M_PI * i) / (float)CIRCLE_SEGMENTS;
        v[i+1].x = g_visualWheelRadius * cosf(t);
        v[i+1].y = g_visualWheelRadius * sinf(t);
        v[i+1].z = 0.0f;
        v[i+1].u = 0.5f + 0.5f * cosf(t);
        v[i+1].v = 0.5f + 0.5f * sinf(t);
    }
    
    LPDIRECT3DTEXTURE9 wheelTex = GetTireTexture(g_selectedCar);
    g_pd3dDevice->SetTexture(0, wheelTex);
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, CIRCLE_SEGMENTS, v, sizeof(TEXVERTEX));
}

//============================================================================================
// RenderProp — draws a spinning propeller at the nose of the airplane.
// Two thin quads crossed at the prop centre, rotated by g_propSpin.
// At high RPM the blades blur into a disc — rendered with alpha blending.
//============================================================================================
void RenderProp(float noseX, float noseY, float planeAngle)
{
    if (!g_pSolidColourShader) return;

    // Nose sits at ~95% of texture width = 0.44 * fuselage half-width from centre.
    // Prop disc radius matches fuselage height (~1.6 units) so PROP_R = 0.40.
    //
    // Viewed from the side the prop spins around the nose axis (into screen).
    // Each blade appears to shorten and lengthen as it rotates:
    //   visible half-length = PROP_R * |cos(propSpin)|
    //   full length at 0 / 180 deg, point at 90 deg.
    // Two blades 180 deg apart share the same |cos| value so they are drawn
    // as one centred quad from -bladeH to +bladeH.
    // Blur disc is always present and grows more opaque with airspeed.

    const float PROP_R    = 0.44f;   // prop disc radius
    const float BLADE_W   = 0.019f;  // blade chord — very thin
    const float BLUR_W    = 0.0f;  // blur oval left at 0
    const float HUB_SIZE  = 0.0f;  // hub cap left at 0.

    // Single WVP: nose position, pitched by planeAngle — no propSpin in matrix.
    // propSpin drives geometry only (blade height and blur opacity).
    D3DXMATRIX matRot, matTrans, matWVP;
    D3DXMatrixRotationZ(&matRot, planeAngle);
    D3DXMatrixTranslation(&matTrans, noseX, noseY, -0.15f);
    matWVP = matRot * matTrans * g_matView * g_matProj;

    g_pd3dDevice->SetPixelShader(g_pSolidColourShader);
    g_pd3dDevice->SetTexture(0, NULL);

    // ── Layer 1: Blur disc ────────────────────────────────────────────────────
    // Thin vertical oval always present. Opacity grows with airspeed so at
    // cruise speed the blades smear into a solid disc.
    float blurAlpha = 0.20f + (g_hudSpeedometer / AIRPLANE_MOTOR_MAX_SPEED) * 0.45f;
    if (blurAlpha > 0.65f) blurAlpha = 0.65f;

    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);
    float blurCol[4] = { 0.60f, 0.60f, 0.60f, blurAlpha };
    g_pd3dDevice->SetPixelShaderConstantF(0, blurCol, 1);

    TEXVERTEX ov[4];
    ov[0].x = -BLUR_W; ov[0].y =  PROP_R; ov[0].z = 0; ov[0].u=0; ov[0].v=0;
    ov[1].x =  BLUR_W; ov[1].y =  PROP_R; ov[1].z = 0; ov[1].u=1; ov[1].v=0;
    ov[2].x = -BLUR_W; ov[2].y = -PROP_R; ov[2].z = 0; ov[2].u=0; ov[2].v=1;
    ov[3].x =  BLUR_W; ov[3].y = -PROP_R; ov[3].z = 0; ov[3].u=1; ov[3].v=1;
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, ov, sizeof(TEXVERTEX));

    // ── Layer 2: Two blades (drawn as one centred quad) ───────────────────────
    // Blade half-height = PROP_R * |cos(propSpin)|.
    // Blade width is constant and thin — the height oscillation is what creates
    // the spinning illusion as blades shorten toward viewer and lengthen away.
    float bladeH = PROP_R * fabsf(cosf(g_propSpin));
    if (bladeH < 0.008f) bladeH = 0.008f;  // minimum sliver so hub stays visible

    float bladeCol[4] = { 0.08f, 0.08f, 0.08f, 0.92f };
    g_pd3dDevice->SetPixelShaderConstantF(0, bladeCol, 1);

    TEXVERTEX b[4];
    b[0].x = -BLADE_W; b[0].y =  bladeH; b[0].z = 0; b[0].u=0; b[0].v=0;
    b[1].x =  BLADE_W; b[1].y =  bladeH; b[1].z = 0; b[1].u=1; b[1].v=0;
    b[2].x = -BLADE_W; b[2].y = -bladeH; b[2].z = 0; b[2].u=0; b[2].v=1;
    b[3].x =  BLADE_W; b[3].y = -bladeH; b[3].z = 0; b[3].u=1; b[3].v=1;
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, b, sizeof(TEXVERTEX));

    // ── Layer 3: Hub cap ──────────────────────────────────────────────────────
    float hubCol[4] = { 0.15f, 0.15f, 0.15f, 1.0f };
    g_pd3dDevice->SetPixelShaderConstantF(0, hubCol, 1);

    TEXVERTEX h[4];
    h[0].x = -HUB_SIZE; h[0].y =  HUB_SIZE; h[0].z = 0; h[0].u=0; h[0].v=0;
    h[1].x =  HUB_SIZE; h[1].y =  HUB_SIZE; h[1].z = 0; h[1].u=1; h[1].v=0;
    h[2].x = -HUB_SIZE; h[2].y = -HUB_SIZE; h[2].z = 0; h[2].u=0; h[2].v=1;
    h[3].x =  HUB_SIZE; h[3].y = -HUB_SIZE; h[3].z = 0; h[3].u=1; h[3].v=1;
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, h, sizeof(TEXVERTEX));

    g_pd3dDevice->SetPixelShader(g_pPixelShader);
}

void RenderBody(float x, float y, float w, float h, float rot) {
    D3DXMATRIX matRot, matTrans, matResult;
    D3DXMatrixRotationZ(&matRot, rot);
    D3DXMatrixTranslation(&matTrans, x, y + g_rideHeightOffset, -0.1f);
    matResult = matRot * matTrans * g_matView * g_matProj;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matResult, 4);

    TEXVERTEX box[4];
    box[0].x = -w/2; box[0].y =  h/2; box[0].z = 0; box[0].u = 0; box[0].v = 0;
    box[1].x =  w/2; box[1].y =  h/2; box[1].z = 0; box[1].u = 1; box[1].v = 0;
    box[2].x = -w/2; box[2].y = -h/2; box[2].z = 0; box[2].u = 0; box[2].v = 1;
    box[3].x =  w/2; box[3].y = -h/2; box[3].z = 0; box[3].u = 1; box[3].v = 1;

    LPDIRECT3DTEXTURE9 carTex = g_pTexCar;
    if (g_selectedCar == CAR_JEEP) carTex = g_pTexCar;
    else if (g_selectedCar == CAR_RALLY) carTex = g_pTexCarRally;
    else if (g_selectedCar == CAR_MONSTER) carTex = g_pTexCarMonster;
    else if (g_selectedCar == CAR_QUAD_BIKE) carTex = g_pTexCarQuadBike;
    else if (g_selectedCar == CAR_RACE_CAR) carTex = g_pTexCarRacecar;
    else if (g_selectedCar == CAR_FAMILY) carTex = g_pTexCarFamily;
    else if (g_selectedCar == CAR_OLD_SCHOOL) carTex = g_pTexCarOldSchool;
    else if (g_selectedCar == CAR_PARAMEDIC) carTex = g_pTexCarParamedic;
    else if (g_selectedCar == CAR_BUGGY) carTex = g_pTexCarBuggy;
    else if (g_selectedCar == CAR_SPRINTER) carTex = g_pTexCarSprinter;
    else if (g_selectedCar == CAR_POLICE) carTex = g_pTexCarPolice;
    else if (g_selectedCar == CAR_STUNT_RIDER) carTex = g_pTexCarStuntRider;
    else if (g_selectedCar == CAR_SUPER_DIESEL) carTex = g_pTexCarSuperDiesel;
    else if (g_selectedCar == CAR_SUPER_JEEP) carTex = g_pTexCarSuperJeep;
    else if (g_selectedCar == CAR_TROPHY_TRUCK) carTex = g_pTexCarTrophy;
    else if (g_selectedCar == CAR_CHEVELLE) carTex = g_pTexCarChevelle; // New Car
    else if (g_selectedCar == CAR_BLAZER) carTex = g_pTexCar_Blazer; // New Car
    else if (g_selectedCar == CAR_CAPRICE) carTex = g_pTexCar_Caprice; // New Car
    else if (g_selectedCar == CAR_CYBERTRUCK) carTex = g_pTexCar_Cybertruck; // New Car
    else if (g_selectedCar == CAR_KNIGHTRIDER) carTex = g_pTexCar_KnightRider; // New Car
    else if (g_selectedCar == CAR_SCHOOLBUS) carTex = g_pTexCar_SchoolBus; // New Car
    else if (g_selectedCar == CAR_USEDCAR) carTex = g_pTexCar_UsedCar; // New Car
    else if (g_selectedCar == CAR_BOAT)     carTex = g_pTexBoat;
    else if (g_selectedCar == CAR_AIRPLANE) carTex = g_pTexAirplane;
    else {
        // ✅ Custom car body texture
        int customIdx = (int)g_selectedCar - CAR_CUSTOM_BASE;
        if (customIdx >= 0 && customIdx < g_numCustomCars
            && g_customCars[customIdx].loaded
            && g_customCars[customIdx].pTexBody != NULL) {
            carTex = g_customCars[customIdx].pTexBody;
        }
    }

    g_pd3dDevice->SetTexture(0, carTex);
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, box, sizeof(TEXVERTEX));
}

//============================================================================================
// ✅ ICON-BASED CAR SELECT SCREEN (WITH ALL 16 CARS!)
//============================================================================================

void RenderCarSelectScreen() {
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    
    g_pd3dDevice->SetVertexDeclaration(g_pVertexDecl);
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
    
    // ✅ Fixed menu camera: never affected by dynamic game camera position/FOV.
    D3DXMATRIX matMenuView, matMenuProj, matMenuVP;
    D3DXVECTOR3 mEye(0.0f, 1.5f, -15.0f);
    D3DXVECTOR3 mAt (0.0f, -0.5f,  0.0f);
    D3DXVECTOR3 mUp (0.0f,  1.0f,  0.0f);
    D3DXMatrixLookAtLH(&matMenuView, &mEye, &mAt, &mUp);
    D3DXMatrixPerspectiveFovLH(&matMenuProj, D3DX_PI / 4, 16.0f / 9.0f, 1.0f, 500.0f);
    matMenuVP = matMenuView * matMenuProj;

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);

    D3DXMATRIX matBGScale;
    D3DXMatrixScaling(&matBGScale, 12.8f, 8.0f, 2.0f);
    D3DXMATRIX matWVP = matBGScale * matIdentity * matMenuVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

    g_pd3dDevice->SetTexture(0, g_pTexCarSelectBG);
    
    TEXVERTEX bgQuad[4];
    bgQuad[0].x = -1.0f; bgQuad[0].y =  1.0f; bgQuad[0].z = 0.5f; bgQuad[0].u = 0.0f; bgQuad[0].v = 0.0f;
    bgQuad[1].x =  1.0f; bgQuad[1].y =  1.0f; bgQuad[1].z = 0.5f; bgQuad[1].u = 1.0f; bgQuad[1].v = 0.0f;
    bgQuad[2].x = -1.0f; bgQuad[2].y = -1.0f; bgQuad[2].z = 0.5f; bgQuad[2].u = 0.0f; bgQuad[2].v = 1.0f;
    bgQuad[3].x =  1.0f; bgQuad[3].y = -1.0f; bgQuad[3].z = 0.5f; bgQuad[3].u = 1.0f; bgQuad[3].v = 1.0f;
    
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, bgQuad, sizeof(TEXVERTEX));
    
    LPDIRECT3DTEXTURE9 previewTex = GetCarSelectionIcon(g_carSelectIndex);
    float previewScale = 1.8f;
    
    D3DXMATRIX matPreview;
    D3DXMatrixScaling(&matPreview, previewScale, previewScale, 1.0f);
    D3DXMATRIX matPreviewTrans;
    D3DXMatrixTranslation(&matPreviewTrans, 0.0f, 0.0f, -0.2f);
    D3DXMATRIX matPreviewWVP = matPreview * matPreviewTrans * matIdentity * matMenuVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matPreviewWVP, 4);
    
    g_pd3dDevice->SetTexture(0, previewTex);
    
    TEXVERTEX previewQuad[4];
    previewQuad[0].x = -0.7f; previewQuad[0].y =  0.9f; previewQuad[0].z = 0; previewQuad[0].u = 0; previewQuad[0].v = 0;
    previewQuad[1].x =  0.7f; previewQuad[1].y =  0.9f; previewQuad[1].z = 0; previewQuad[1].u = 1; previewQuad[1].v = 0;
    previewQuad[2].x = -0.7f; previewQuad[2].y = -0.9f; previewQuad[2].z = 0; previewQuad[2].u = 0; previewQuad[2].v = 1;
    previewQuad[3].x =  0.7f; previewQuad[3].y = -0.9f; previewQuad[3].z = 0; previewQuad[3].u = 1; previewQuad[3].v = 1;
    
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, previewQuad, sizeof(TEXVERTEX));
    
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

//============================================================================================
// RENDER TITLE SCREEN
//============================================================================================

void RenderTitleScreen() {
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    D3DXMatrixTranspose(&matIdentity, &matIdentity);
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matIdentity, 4);
    
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetPixelShader(g_pMenuPixelShader);
    g_pd3dDevice->SetTexture(0, g_pTexTitle);
    g_pd3dDevice->SetStreamSource(0, g_pSplashVB_Title, 0, sizeof(TEXVERTEX));
    g_pd3dDevice->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
    g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

//============================================================================================
// RENDER BACKGROUNDS
//============================================================================================

void RenderBackground() {
    // ✅ Backgrounds rendered as screen-filling NDC quads with UV-scroll parallax.
    //    WVP = identity → clip-space positions = vertex positions exactly.
    //    (-1,-1) to (1,1) always covers the full screen regardless of camera Y or FOV.
    //    Parallax is achieved by shifting U coordinates — no world-space geometry needed.
    //    This matches how HCR renders its background layers.
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matIdentity, 4);

    // ── Select background texture ─────────────────────────────────────────────
    LPDIRECT3DTEXTURE9 bgTex = g_pTexBackground;
    if (g_currentLevel == LEVEL_2)      bgTex = g_pTexBackground_L2;
    else if (g_currentLevel == LEVEL_3) bgTex = g_pTexBackground_L3;
    else if (g_currentLevel == LEVEL_4) bgTex = g_pTexBackground_L4;
    else if (g_currentLevel == LEVEL_5) bgTex = g_pTexBackground_L5;
    else if (g_currentLevel == LEVEL_6) bgTex = g_pTexBackground_L6;
    else if (g_currentLevel == LEVEL_7) bgTex = g_pTexBackground_L7;
    else if (g_currentLevel >= LEVEL_CUSTOM_1) {
        int ci = g_currentLevel - (int)LEVEL_CUSTOM_1;
        if (ci >= 0 && ci < g_numCustomLevels && g_customLevels[ci].loaded
            && g_customLevels[ci].pTexBg != NULL)
            bgTex = g_customLevels[ci].pTexBg;
    }

    // ── Layer 1: Far background — slowest parallax ────────────────────────────
    // uOffset scrolls the texture horizontally. Texture wrap mode handles tiling.
    float uBG = (g_fScrollPos * BACKGROUND_PARALLAX_FACTOR) / BACKGROUND_TILE_SIZE;

    g_pd3dDevice->SetTexture(0, bgTex);
    TEXVERTEX bgQuad[4];
    bgQuad[0].x = -1.0f; bgQuad[0].y =  1.0f; bgQuad[0].z = 0.99f; bgQuad[0].u = uBG;       bgQuad[0].v = 0.0f;
    bgQuad[1].x =  1.0f; bgQuad[1].y =  1.0f; bgQuad[1].z = 0.99f; bgQuad[1].u = uBG + 1.0f; bgQuad[1].v = 0.0f;
    bgQuad[2].x = -1.0f; bgQuad[2].y = -1.0f; bgQuad[2].z = 0.99f; bgQuad[2].u = uBG;       bgQuad[2].v = 1.0f;
    bgQuad[3].x =  1.0f; bgQuad[3].y = -1.0f; bgQuad[3].z = 0.99f; bgQuad[3].u = uBG + 1.0f; bgQuad[3].v = 1.0f;
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, bgQuad, sizeof(TEXVERTEX));

    // ── Layer 2: Midground — medium parallax ──────────────────────────────────
    LPDIRECT3DTEXTURE9 mgTex = g_pTexBackground2;
    if (g_currentLevel == LEVEL_2)      mgTex = g_pTexBackground2_L2;
    else if (g_currentLevel == LEVEL_3) mgTex = g_pTexBackground2_L3;
    else if (g_currentLevel == LEVEL_4) mgTex = g_pTexBackground2_L4;
    else if (g_currentLevel == LEVEL_5) mgTex = g_pTexBackground2_L5;
    else if (g_currentLevel == LEVEL_6) mgTex = g_pTexBackground2_L6;
    else if (g_currentLevel == LEVEL_7) mgTex = g_pTexBackground2_L7;
    else if (g_currentLevel >= LEVEL_CUSTOM_1) {
        int ci = g_currentLevel - (int)LEVEL_CUSTOM_1;
        if (ci >= 0 && ci < g_numCustomLevels && g_customLevels[ci].loaded)
            mgTex = g_customLevels[ci].pTexBg2;
    }

    if (mgTex != NULL) {
        float uMG = (g_fScrollPos * BACKGROUND2_PARALLAX_FACTOR) / BACKGROUND_TILE_SIZE;

        g_pd3dDevice->SetTexture(0, mgTex);
        TEXVERTEX mgQuad[4];
        mgQuad[0].x = -1.0f; mgQuad[0].y =  1.0f; mgQuad[0].z = 0.98f; mgQuad[0].u = uMG;       mgQuad[0].v = 0.0f;
        mgQuad[1].x =  1.0f; mgQuad[1].y =  1.0f; mgQuad[1].z = 0.98f; mgQuad[1].u = uMG + 1.0f; mgQuad[1].v = 0.0f;
        mgQuad[2].x = -1.0f; mgQuad[2].y = -1.0f; mgQuad[2].z = 0.98f; mgQuad[2].u = uMG;       mgQuad[2].v = 1.0f;
        mgQuad[3].x =  1.0f; mgQuad[3].y = -1.0f; mgQuad[3].z = 0.98f; mgQuad[3].u = uMG + 1.0f; mgQuad[3].v = 1.0f;
        g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, mgQuad, sizeof(TEXVERTEX));
    }

    // ── Layer 3: Near foreground — fastest parallax ───────────────────────────
    // Scrolls at 0.72x speed. Use a PNG with transparent sky region so layers 1
    // and 2 show through the top. Only the lower portion of the image should have
    // content (distant rocks, bushes, silhouettes). Rendered as full NDC quad
    // but the texture's own alpha transparency creates the layered look.
    LPDIRECT3DTEXTURE9 fgTex = g_pTexBackground3;
    if (g_currentLevel == LEVEL_2)      fgTex = g_pTexBackground3_L2;
    else if (g_currentLevel == LEVEL_3) fgTex = g_pTexBackground3_L3;
    else if (g_currentLevel == LEVEL_4) fgTex = g_pTexBackground3_L4;
    else if (g_currentLevel == LEVEL_5) fgTex = g_pTexBackground3_L5;
    else if (g_currentLevel == LEVEL_6) fgTex = g_pTexBackground3_L6;
    else if (g_currentLevel == LEVEL_7) fgTex = g_pTexBackground3_L7;
    else if (g_currentLevel >= LEVEL_CUSTOM_1) {
        int ci = g_currentLevel - (int)LEVEL_CUSTOM_1;
        if (ci >= 0 && ci < g_numCustomLevels && g_customLevels[ci].loaded)
            fgTex = g_customLevels[ci].pTexBg3;
    }

    if (fgTex != NULL) {
        float uFG = (g_fScrollPos * BACKGROUND3_PARALLAX_FACTOR) / BACKGROUND_TILE_SIZE;

        g_pd3dDevice->SetTexture(0, fgTex);
        TEXVERTEX fgQuad[4];
        fgQuad[0].x = -1.0f; fgQuad[0].y =  1.0f; fgQuad[0].z = 0.97f; fgQuad[0].u = uFG;       fgQuad[0].v = 0.0f;
        fgQuad[1].x =  1.0f; fgQuad[1].y =  1.0f; fgQuad[1].z = 0.97f; fgQuad[1].u = uFG + 1.0f; fgQuad[1].v = 0.0f;
        fgQuad[2].x = -1.0f; fgQuad[2].y = -1.0f; fgQuad[2].z = 0.97f; fgQuad[2].u = uFG;       fgQuad[2].v = 1.0f;
        fgQuad[3].x =  1.0f; fgQuad[3].y = -1.0f; fgQuad[3].z = 0.97f; fgQuad[3].u = uFG + 1.0f; fgQuad[3].v = 1.0f;
        g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, fgQuad, sizeof(TEXVERTEX));
    }
}

//============================================================================================
// ✅ RENDER PAUSE MENU
//============================================================================================

void RenderPauseMenu() {
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(135,206,235), 1.0f, 0);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    g_pd3dDevice->SetVertexDeclaration(g_pVertexDecl);
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetPixelShader(g_pPixelShader);

    RenderBackground();

    LPDIRECT3DTEXTURE9 terrainTexP = g_pTexTerrain;
    if (g_currentLevel == LEVEL_2)      terrainTexP = g_pTexTerrain_L2;
    else if (g_currentLevel == LEVEL_3) terrainTexP = g_pTexTerrain_L3;
    else if (g_currentLevel == LEVEL_4) terrainTexP = g_pTexTerrain_L4;
    else if (g_currentLevel == LEVEL_5) terrainTexP = g_pTexTerrain_L5;
    else if (g_currentLevel == LEVEL_6) terrainTexP = g_pTexTerrain_L6;
    else if (g_currentLevel == LEVEL_7) terrainTexP = g_pTexTerrain_L7;
    else if (g_currentLevel >= LEVEL_CUSTOM_1) {
        int ci = g_currentLevel - (int)LEVEL_CUSTOM_1;
        if (ci >= 0 && ci < g_numCustomLevels && g_customLevels[ci].loaded
            && g_customLevels[ci].pTexTerrain != NULL)
            terrainTexP = g_customLevels[ci].pTexTerrain;
    }

    g_pd3dDevice->SetTexture(0, terrainTexP);
    D3DXMATRIX matWVP = g_matView * g_matProj;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);
    g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(TEXVERTEX));
    g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, (NUM_DISPLAY_SEGMENTS*2)-2);
    g_pd3dDevice->SetStreamSource(0, NULL, 0, 0);

    if (g_carChassis && g_wheelRear && g_wheelFront) {
        b2Vec2 cPos  = g_carChassis->GetPosition();
        float  cAngle = g_carChassis->GetAngle();
        b2Vec2 rPos  = g_wheelRear->GetPosition();
        b2Vec2 fPos  = g_wheelFront->GetPosition();
        float  wSpin = g_fScrollPos * 4.5f;
        RenderBody(cPos.x - g_fScrollPos, cPos.y, g_visualCarWidth, g_visualCarHeight, cAngle);
        if (!g_isBoatLevel) {
            RenderWheel(rPos.x - g_fScrollPos, rPos.y, 0, 0, cAngle, -wSpin);
            RenderWheel(fPos.x - g_fScrollPos, fPos.y, 0, 0, cAngle, -wSpin);
        }
    }

    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    // ✅ Pause overlay (panel + buttons) uses the same fixed camera as the HUD so
    //    it stays screen-centred regardless of where the dynamic camera is pointing.
    D3DXMATRIX matPauseView, matPauseProj, matPauseVP;
    D3DXVECTOR3 pEye(0.0f, 1.5f, -15.0f);
    D3DXVECTOR3 pAt (0.0f, -0.5f,  0.0f);
    D3DXVECTOR3 pUp (0.0f,  1.0f,  0.0f);
    D3DXMatrixLookAtLH(&matPauseView, &pEye, &pAt, &pUp);
    D3DXMatrixPerspectiveFovLH(&matPauseProj, D3DX_PI / 4, 16.0f / 9.0f, 1.0f, 500.0f);
    matPauseVP = matPauseView * matPauseProj;

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);

    // Pause menu background panel
    D3DXMATRIX matBGScale;
    D3DXMatrixScaling(&matBGScale, 6.4f, 4.0f, 1.0f);
    matWVP = matBGScale * matIdentity * matPauseVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

    g_pd3dDevice->SetTexture(0, g_pTexPauseMenuBG);

    TEXVERTEX bgQuad[4];
    bgQuad[0].x = -1.0f; bgQuad[0].y =  1.0f; bgQuad[0].z = 0.5f; bgQuad[0].u = 0.0f; bgQuad[0].v = 0.0f;
    bgQuad[1].x =  1.0f; bgQuad[1].y =  1.0f; bgQuad[1].z = 0.5f; bgQuad[1].u = 1.0f; bgQuad[1].v = 0.0f;
    bgQuad[2].x = -1.0f; bgQuad[2].y = -1.0f; bgQuad[2].z = 0.5f; bgQuad[2].u = 0.0f; bgQuad[2].v = 1.0f;
    bgQuad[3].x =  1.0f; bgQuad[3].y = -1.0f; bgQuad[3].z = 0.5f; bgQuad[3].u = 1.0f; bgQuad[3].v = 1.0f;

    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, bgQuad, sizeof(TEXVERTEX));

    // Resume button (top option)
    D3DXMATRIX matButton1;
    D3DXMatrixScaling(&matButton1, 2.0f, 0.5f, 1.0f);
    D3DXMATRIX matButton1Trans;
    D3DXMatrixTranslation(&matButton1Trans, 0.0f, 1.0f, -0.2f);
    D3DXMATRIX matButton1WVP = matButton1 * matButton1Trans * matIdentity * matPauseVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matButton1WVP, 4);

    TEXVERTEX buttonQuad[4];
    buttonQuad[0].x = -1.0f; buttonQuad[0].y =  1.0f; buttonQuad[0].z = 0; buttonQuad[0].u = 0; buttonQuad[0].v = 0;
    buttonQuad[1].x =  1.0f; buttonQuad[1].y =  1.0f; buttonQuad[1].z = 0; buttonQuad[1].u = 1; buttonQuad[1].v = 0;
    buttonQuad[2].x = -1.0f; buttonQuad[2].y = -1.0f; buttonQuad[2].z = 0; buttonQuad[2].u = 0; buttonQuad[2].v = 1;
    buttonQuad[3].x =  1.0f; buttonQuad[3].y = -1.0f; buttonQuad[3].z = 0; buttonQuad[3].u = 1; buttonQuad[3].v = 1;

    if (g_pauseMenuIndex == 0)
        g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
    g_pd3dDevice->SetTexture(0, g_pTexPauseResumeButton);
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, buttonQuad, sizeof(TEXVERTEX));
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);

    // Level select button
    D3DXMATRIX matButton2;
    D3DXMatrixScaling(&matButton2, 2.0f, 0.5f, 1.0f);
    D3DXMATRIX matButton2Trans;
    D3DXMatrixTranslation(&matButton2Trans, 0.0f, -1.0f, -0.2f);
    D3DXMATRIX matButton2WVP = matButton2 * matButton2Trans * matIdentity * matPauseVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matButton2WVP, 4);
    if (g_pauseMenuIndex == 2)
        g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
    g_pd3dDevice->SetTexture(0, g_pTexPauseLevelButton);
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, buttonQuad, sizeof(TEXVERTEX));
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);

    // Back to title button (middle option)
    D3DXMATRIX matButton3;
    D3DXMatrixScaling(&matButton3, 2.0f, 0.5f, 1.0f);
    D3DXMATRIX matButton3Trans;
    D3DXMatrixTranslation(&matButton3Trans, 0.0f, 0.0f, -0.2f);
    D3DXMATRIX matButton3WVP = matButton3 * matButton3Trans * matIdentity * matPauseVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matButton3WVP, 4);
    if (g_pauseMenuIndex == 1)
        g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);
    g_pd3dDevice->SetTexture(0, g_pTexPauseRestartButton);
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, buttonQuad, sizeof(TEXVERTEX));
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);

    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

//============================================================================================
// INITIALIZATION
//============================================================================================

//============================================================================================
// ✅ RENDER MAIN MENU
//============================================================================================

void RenderMainMenu() {
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetVertexDeclaration(g_pVertexDecl);
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
    
    // ✅ Fixed menu camera: never affected by dynamic game camera position/FOV.
    D3DXMATRIX matMenuView, matMenuProj, matMenuVP;
    D3DXVECTOR3 mEye(0.0f, 1.5f, -15.0f);
    D3DXVECTOR3 mAt (0.0f, -0.5f,  0.0f);
    D3DXVECTOR3 mUp (0.0f,  1.0f,  0.0f);
    D3DXMatrixLookAtLH(&matMenuView, &mEye, &mAt, &mUp);
    D3DXMatrixPerspectiveFovLH(&matMenuProj, D3DX_PI / 4, 16.0f / 9.0f, 1.0f, 500.0f);
    matMenuVP = matMenuView * matMenuProj;

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    D3DXMATRIX matBGScale;
    D3DXMatrixScaling(&matBGScale, 12.8f, 8.0f, 2.0f);
    D3DXMATRIX matWVP = matBGScale * matIdentity * matMenuVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);
    g_pd3dDevice->SetTexture(0, g_pTexMainMenuBG);
    
    TEXVERTEX bgQuad[4];
    bgQuad[0].x = -1.0f; bgQuad[0].y =  1.0f; bgQuad[0].z = 0.5f; bgQuad[0].u = 0.0f; bgQuad[0].v = 0.0f;
    bgQuad[1].x =  1.0f; bgQuad[1].y =  1.0f; bgQuad[1].z = 0.5f; bgQuad[1].u = 1.0f; bgQuad[1].v = 0.0f;
    bgQuad[2].x = -1.0f; bgQuad[2].y = -1.0f; bgQuad[2].z = 0.5f; bgQuad[2].u = 0.0f; bgQuad[2].v = 1.0f;
    bgQuad[3].x =  1.0f; bgQuad[3].y = -1.0f; bgQuad[3].z = 0.5f; bgQuad[3].u = 1.0f; bgQuad[3].v = 1.0f;
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, bgQuad, sizeof(TEXVERTEX));
    
    // ✅ RENDER MENU BUTTON ICONS (CARS / SETTINGS / ABOUT)
    LPDIRECT3DTEXTURE9 menuButtons[] = {g_pTexMenuButtonCars, g_pTexMenuButtonSettings, g_pTexMenuButtonAbout};
    
    // Button positions (horizontal layout, centered)
    float buttonPositions[] = {-2.0f, 0.0f, 2.0f};  // ✅ INCREASED SPACING: Was -0.65f, 0.0f, 0.65f
    float buttonBaseScale = 0.5f;  // ✅ INCREASED: Was 0.25f - Now 2x larger
    float buttonSelectedScale = 0.65f;  // ✅ INCREASED: Was 0.35f
    
    for (int i = 0; i < 3; i++) {
        if (menuButtons[i] == NULL) continue;
        
        g_pd3dDevice->SetTexture(0, menuButtons[i]);
        
        // Scale: selected button is larger, others are smaller
        float buttonScale = (i == g_mainMenuIndex) ? buttonSelectedScale : buttonBaseScale;
        
        // Create scale and translation matrices
        D3DXMATRIX matButtonScale;
        D3DXMatrixScaling(&matButtonScale, buttonScale, buttonScale, 1.0f);
        
        D3DXMATRIX matButtonTranslate;
        D3DXMatrixTranslation(&matButtonTranslate, buttonPositions[i], 0.3f, 0.0f);
        
        D3DXMATRIX matButtonWVP = matButtonScale * matButtonTranslate * matIdentity * matMenuVP;
        g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matButtonWVP, 4);
        
        // Render button quad
        TEXVERTEX buttonQuad[4];
        buttonQuad[0].x = -1.0f; buttonQuad[0].y =  1.0f; buttonQuad[0].z = 0.0f; buttonQuad[0].u = 0.0f; buttonQuad[0].v = 0.0f;
        buttonQuad[1].x =  1.0f; buttonQuad[1].y =  1.0f; buttonQuad[1].z = 0.0f; buttonQuad[1].u = 1.0f; buttonQuad[1].v = 0.0f;
        buttonQuad[2].x = -1.0f; buttonQuad[2].y = -1.0f; buttonQuad[2].z = 0.0f; buttonQuad[2].u = 0.0f; buttonQuad[2].v = 1.0f;
        buttonQuad[3].x =  1.0f; buttonQuad[3].y = -1.0f; buttonQuad[3].z = 0.0f; buttonQuad[3].u = 1.0f; buttonQuad[3].v = 1.0f;
        
        g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, buttonQuad, sizeof(TEXVERTEX));
    }
    
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

//============================================================================================
// ✅ RENDER SETTINGS SCREEN
//============================================================================================

void RenderSettingsScreen() {
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetVertexDeclaration(g_pVertexDecl);
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetPixelShader(g_pPixelShader);

    // ✅ Fixed menu camera (same as other menu screens)
    D3DXMATRIX matMenuView, matMenuProj, matMenuVP;
    D3DXVECTOR3 mEye(0.0f, 1.5f, -15.0f);
    D3DXVECTOR3 mAt (0.0f, -0.5f,  0.0f);
    D3DXVECTOR3 mUp (0.0f,  1.0f,  0.0f);
    D3DXMatrixLookAtLH(&matMenuView, &mEye, &mAt, &mUp);
    D3DXMatrixPerspectiveFovLH(&matMenuProj, D3DX_PI / 4, 16.0f / 9.0f, 1.0f, 500.0f);
    matMenuVP = matMenuView * matMenuProj;

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);

    // ── Draw settings_menu.png as full-screen background ────────────────────
    D3DXMATRIX matBGScale;
    D3DXMatrixScaling(&matBGScale, 12.8f, 8.0f, 2.0f);
    D3DXMATRIX matWVP = matBGScale * matIdentity * matMenuVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);
    g_pd3dDevice->SetTexture(0, g_pTexSettingsBG);

    TEXVERTEX bgQuad[4];
    bgQuad[0].x = -1.0f; bgQuad[0].y =  1.0f; bgQuad[0].z = 0.5f; bgQuad[0].u = 0.0f; bgQuad[0].v = 0.0f;
    bgQuad[1].x =  1.0f; bgQuad[1].y =  1.0f; bgQuad[1].z = 0.5f; bgQuad[1].u = 1.0f; bgQuad[1].v = 0.0f;
    bgQuad[2].x = -1.0f; bgQuad[2].y = -1.0f; bgQuad[2].z = 0.5f; bgQuad[2].u = 0.0f; bgQuad[2].v = 1.0f;
    bgQuad[3].x =  1.0f; bgQuad[3].y = -1.0f; bgQuad[3].z = 0.5f; bgQuad[3].u = 1.0f; bgQuad[3].v = 1.0f;
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, bgQuad, sizeof(TEXVERTEX));

    // ── Draw volume slider fills using solid colour shader ──────────────────
    if (g_pSolidColourShader) {
        g_pd3dDevice->SetPixelShader(g_pSolidColourShader);
        g_pd3dDevice->SetTexture(0, NULL);

        // Disable Z-test so fills draw on top of background
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

        // Bar geometry constants (normalised to the background quad coordinate space)
        // These map to the slider bar outlines in settings_menu.png with a small inset
        // Background quad spans (-1,-1) to (1,1) scaled by (12.8, 8.0).
        // Bar positions estimated from the image layout (with border inset):
        float barLeft   = -0.36f;   // left edge of inner fill area
        float barRight  =  0.62f;   // right edge of inner fill area
        float barHeight =  0.045f;  // half-height of bar fill

        // Centre Y positions for each bar (in bg-quad normalised coords)
        float musicBarY =  0.14f;   // music slider centre
        float sfxBarY   = -0.18f;   // SFX slider centre

        float barYPositions[2] = { musicBarY, sfxBarY };
        float volumes[2]       = { g_musicVolume, g_sfxVolume };

        for (int i = 0; i < 2; i++) {
            float vol = volumes[i];
            if (vol <= 0.0f) continue;  // nothing to fill

            // Fill width proportional to volume
            float fillRight = barLeft + (barRight - barLeft) * vol;

            // Colour: selected slider is brighter, unselected is dimmer
            float cFill[4];
            if (i == g_settingsIndex) {
                // Selected — bright teal
                cFill[0] = 0.15f; cFill[1] = 0.85f; cFill[2] = 0.95f; cFill[3] = 0.90f;
            } else {
                // Unselected — dimmer blue-grey
                cFill[0] = 0.30f; cFill[1] = 0.50f; cFill[2] = 0.60f; cFill[3] = 0.70f;
            }
            g_pd3dDevice->SetPixelShaderConstantF(0, cFill, 1);

            // Build the fill quad in background-quad normalised space, then apply same BG scale
            float cy = barYPositions[i];
            TEXVERTEX fillQuad[4];
            fillQuad[0].x = barLeft;   fillQuad[0].y = cy + barHeight; fillQuad[0].z = 0.49f;
            fillQuad[0].u = 0.0f; fillQuad[0].v = 0.0f;
            fillQuad[1].x = fillRight; fillQuad[1].y = cy + barHeight; fillQuad[1].z = 0.49f;
            fillQuad[1].u = 1.0f; fillQuad[1].v = 0.0f;
            fillQuad[2].x = barLeft;   fillQuad[2].y = cy - barHeight; fillQuad[2].z = 0.49f;
            fillQuad[2].u = 0.0f; fillQuad[2].v = 1.0f;
            fillQuad[3].x = fillRight; fillQuad[3].y = cy - barHeight; fillQuad[3].z = 0.49f;
            fillQuad[3].u = 1.0f; fillQuad[3].v = 1.0f;

            // Same WVP as background so fills align perfectly
            g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, fillQuad, sizeof(TEXVERTEX));
        }

        // ── Draw selection arrow indicator ──────────────────────────────────
        // Small bright arrow/marker to the left of the selected slider
        {
            float arrowX  = barLeft - 0.06f;   // just left of bar
            float arrowY  = barYPositions[g_settingsIndex];
            float arrowSz = 0.025f;

            float cArrow[4] = { 1.0f, 1.0f, 1.0f, 0.95f };  // white
            g_pd3dDevice->SetPixelShaderConstantF(0, cArrow, 1);

            TEXVERTEX arrow[4];
            arrow[0].x = arrowX - arrowSz; arrow[0].y = arrowY + arrowSz; arrow[0].z = 0.48f;
            arrow[0].u = 0; arrow[0].v = 0;
            arrow[1].x = arrowX + arrowSz; arrow[1].y = arrowY + arrowSz; arrow[1].z = 0.48f;
            arrow[1].u = 1; arrow[1].v = 0;
            arrow[2].x = arrowX - arrowSz; arrow[2].y = arrowY - arrowSz; arrow[2].z = 0.48f;
            arrow[2].u = 0; arrow[2].v = 1;
            arrow[3].x = arrowX + arrowSz; arrow[3].y = arrowY - arrowSz; arrow[3].z = 0.48f;
            arrow[3].u = 1; arrow[3].v = 1;

            g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, arrow, sizeof(TEXVERTEX));
        }

        // Restore state
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
        g_pd3dDevice->SetPixelShader(g_pPixelShader);
    }

    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

//============================================================================================
// ✅ RENDER ABOUT SCREEN
//============================================================================================

void RenderAboutScreen() {
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetVertexDeclaration(g_pVertexDecl);
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
    
    // ✅ Fixed menu camera: never affected by dynamic game camera position/FOV.
    D3DXMATRIX matMenuView, matMenuProj, matMenuVP;
    D3DXVECTOR3 mEye(0.0f, 1.5f, -15.0f);
    D3DXVECTOR3 mAt (0.0f, -0.5f,  0.0f);
    D3DXVECTOR3 mUp (0.0f,  1.0f,  0.0f);
    D3DXMatrixLookAtLH(&matMenuView, &mEye, &mAt, &mUp);
    D3DXMatrixPerspectiveFovLH(&matMenuProj, D3DX_PI / 4, 16.0f / 9.0f, 1.0f, 500.0f);
    matMenuVP = matMenuView * matMenuProj;

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    D3DXMATRIX matBGScale;
    D3DXMatrixScaling(&matBGScale, 12.8f, 8.0f, 2.0f);
    D3DXMATRIX matWVP = matBGScale * matIdentity * matMenuVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);
    g_pd3dDevice->SetTexture(0, g_pTexAboutBG);
    
    TEXVERTEX bgQuad[4];
    bgQuad[0].x = -1.0f; bgQuad[0].y =  1.0f; bgQuad[0].z = 0.5f; bgQuad[0].u = 0.0f; bgQuad[0].v = 0.0f;
    bgQuad[1].x =  1.0f; bgQuad[1].y =  1.0f; bgQuad[1].z = 0.5f; bgQuad[1].u = 1.0f; bgQuad[1].v = 0.0f;
    bgQuad[2].x = -1.0f; bgQuad[2].y = -1.0f; bgQuad[2].z = 0.5f; bgQuad[2].u = 0.0f; bgQuad[2].v = 1.0f;
    bgQuad[3].x =  1.0f; bgQuad[3].y = -1.0f; bgQuad[3].z = 0.5f; bgQuad[3].u = 1.0f; bgQuad[3].v = 1.0f;
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, bgQuad, sizeof(TEXVERTEX));
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

//============================================================================================
// ✅ RENDER LEVEL SELECT SCREEN
//============================================================================================

void RenderLevelSelectScreen() {
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetVertexDeclaration(g_pVertexDecl);
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
    
    // ✅ Fixed menu camera: never affected by dynamic game camera position/FOV.
    D3DXMATRIX matMenuView, matMenuProj, matMenuVP;
    D3DXVECTOR3 mEye(0.0f, 1.5f, -15.0f);
    D3DXVECTOR3 mAt (0.0f, -0.5f,  0.0f);
    D3DXVECTOR3 mUp (0.0f,  1.0f,  0.0f);
    D3DXMatrixLookAtLH(&matMenuView, &mEye, &mAt, &mUp);
    D3DXMatrixPerspectiveFovLH(&matMenuProj, D3DX_PI / 4, 16.0f / 9.0f, 1.0f, 500.0f);
    matMenuVP = matMenuView * matMenuProj;

    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity(&matIdentity);
    D3DXMATRIX matBGScale;
    D3DXMatrixScaling(&matBGScale, 12.8f, 8.0f, 2.0f);
    D3DXMATRIX matWVP = matBGScale * matIdentity * matMenuVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);
    g_pd3dDevice->SetTexture(0, g_pTexLevelSelectBG);
    
    TEXVERTEX bgQuad[4];
    bgQuad[0].x = -1.0f; bgQuad[0].y =  1.0f; bgQuad[0].z = 0.5f; bgQuad[0].u = 0.0f; bgQuad[0].v = 0.0f;
    bgQuad[1].x =  1.0f; bgQuad[1].y =  1.0f; bgQuad[1].z = 0.5f; bgQuad[1].u = 1.0f; bgQuad[1].v = 0.0f;
    bgQuad[2].x = -1.0f; bgQuad[2].y = -1.0f; bgQuad[2].z = 0.5f; bgQuad[2].u = 0.0f; bgQuad[2].v = 1.0f;
    bgQuad[3].x =  1.0f; bgQuad[3].y = -1.0f; bgQuad[3].z = 0.5f; bgQuad[3].u = 1.0f; bgQuad[3].v = 1.0f;
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, bgQuad, sizeof(TEXVERTEX));
    
    // ✅ SAFE icon lookup - never index out of the 6-element built-in array
    LPDIRECT3DTEXTURE9 levelIconTex = NULL;
    if (g_levelSelectIndex < 7) {
        LPDIRECT3DTEXTURE9 levelIcons[] = {g_pTexLevelIcon_1, g_pTexLevelIcon_2, g_pTexLevelIcon_3,
                                           g_pTexLevelIcon_4, g_pTexLevelIcon_5, g_pTexLevelIcon_6,
                                           g_pTexLevelIcon_7};
        levelIconTex = levelIcons[g_levelSelectIndex];
    } else {
        // Custom level slot
        int ci = g_levelSelectIndex - 7;
        if (ci >= 0 && ci < g_numCustomLevels && g_customLevels[ci].loaded)
            levelIconTex = g_customLevels[ci].pTexIcon; // NULL is safe - draw is skipped below
    }

    if (levelIconTex != NULL) {
        g_pd3dDevice->SetTexture(0, levelIconTex);
        TEXVERTEX levelQuad[4];
        levelQuad[0].x = -0.5f; levelQuad[0].y =  0.7f; levelQuad[0].z = 0; levelQuad[0].u = 0; levelQuad[0].v = 0;
        levelQuad[1].x =  0.5f; levelQuad[1].y =  0.7f; levelQuad[1].z = 0; levelQuad[1].u = 1; levelQuad[1].v = 0;
        levelQuad[2].x = -0.5f; levelQuad[2].y = -0.7f; levelQuad[2].z = 0; levelQuad[2].u = 0; levelQuad[2].v = 1;
        levelQuad[3].x =  0.5f; levelQuad[3].y = -0.7f; levelQuad[3].z = 0; levelQuad[3].u = 1; levelQuad[3].v = 1;
        g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, levelQuad, sizeof(TEXVERTEX));
    }
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

//============================================================================================
// ✅ RENDER HUD GAUGES (SPEEDOMETER & BOOST METER)
//============================================================================================

void RenderHUDGauges() {
    if (!g_showHUD) return;
    if (!g_pTexGaugeRPM || !g_pTexGaugeBoost || !g_pTexGaugeNeedle) return;

    // ✅ HUD uses a fixed view+projection that exactly matches the ORIGINAL
    //    static camera the gauge positions were designed for.
    //    eye=(0,1.5,-15) lookat=(0,-0.5,0) FOV=45deg — never changes with speed
    //    or car height, so gauges stay locked to the screen corners at all times.
    D3DXMATRIX matHUDView, matHUDProj, matHUDVP;
    D3DXVECTOR3 hudEye(0.0f, 1.5f, -15.0f);
    D3DXVECTOR3 hudAt (0.0f, -0.5f,  0.0f);
    D3DXVECTOR3 hudUp (0.0f,  1.0f,  0.0f);
    D3DXMatrixLookAtLH(&matHUDView, &hudEye, &hudAt, &hudUp);
    D3DXMatrixPerspectiveFovLH(&matHUDProj, D3DX_PI / 4, 16.0f / 9.0f, 1.0f, 500.0f);
    matHUDVP = matHUDView * matHUDProj;

    // Disable Z-testing for 2D HUD overlay
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    D3DXMATRIX matIdentity, matScale, matTrans, matRot, matFinal;
    D3DXMatrixIdentity(&matIdentity);

    // ========== RPM GAUGE (Bottom Left) ==========
    D3DXMatrixScaling(&matScale, 2.0f, 2.0f, 1.0f);
    D3DXMatrixTranslation(&matTrans, -8.0f, -5.5f, 0.5f);
    matFinal = matScale * matTrans * matHUDVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matFinal, 4);
    g_pd3dDevice->SetTexture(0, g_pTexGaugeRPM);

    TEXVERTEX gaugeQuad[4];
    gaugeQuad[0].x = -1.0f; gaugeQuad[0].y =  1.0f; gaugeQuad[0].z = 0; gaugeQuad[0].u = 0; gaugeQuad[0].v = 0;
    gaugeQuad[1].x =  1.0f; gaugeQuad[1].y =  1.0f; gaugeQuad[1].z = 0; gaugeQuad[1].u = 1; gaugeQuad[1].v = 0;
    gaugeQuad[2].x = -1.0f; gaugeQuad[2].y = -1.0f; gaugeQuad[2].z = 0; gaugeQuad[2].u = 0; gaugeQuad[2].v = 1;
    gaugeQuad[3].x =  1.0f; gaugeQuad[3].y = -1.0f; gaugeQuad[3].z = 0; gaugeQuad[3].u = 1; gaugeQuad[3].v = 1;
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, gaugeQuad, sizeof(TEXVERTEX));

    // Render RPM needle
    float rpmRotation = (3.14159f * 1.75f) - (g_hudSpeedometer * 3.14159f * 1.5f);
    D3DXMatrixRotationZ(&matRot, rpmRotation);
    D3DXMatrixScaling(&matScale, 0.3f, 1.8f, 1.0f);
    D3DXMatrixTranslation(&matTrans, -8.0f, -5.5f, 0.4f);
    matFinal = matScale * matRot * matTrans * matHUDVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matFinal, 4);
    g_pd3dDevice->SetTexture(0, g_pTexGaugeNeedle);
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, gaugeQuad, sizeof(TEXVERTEX));

    // ========== BOOST GAUGE (Bottom Right) ==========
    D3DXMatrixScaling(&matScale, 2.0f, 2.0f, 1.0f);
    D3DXMatrixTranslation(&matTrans, 8.0f, -5.5f, 0.5f);
    matFinal = matScale * matTrans * matHUDVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matFinal, 4);
    g_pd3dDevice->SetTexture(0, g_pTexGaugeBoost);
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, gaugeQuad, sizeof(TEXVERTEX));

    // Render Boost needle
    float boostRotation = (3.14159f * 1.75f) - (g_hudBoost * 3.14159f * 1.5f);
    D3DXMatrixRotationZ(&matRot, boostRotation);
    D3DXMatrixScaling(&matScale, 0.3f, 1.8f, 1.0f);
    D3DXMatrixTranslation(&matTrans, 8.0f, -5.5f, 0.4f);
    matFinal = matScale * matRot * matTrans * matHUDVP;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matFinal, 4);
    g_pd3dDevice->SetTexture(0, g_pTexGaugeNeedle);
    g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, gaugeQuad, sizeof(TEXVERTEX));

    // Re-enable Z-testing
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
}

//============================================================================================
// RENDERING
//============================================================================================

// ── Water rendering — spring-column heightfield ───────────────────────────
// Each column has an independent surface height from the simulation.
// We render:
//   1. Water body — per-column quads from surface down to terrain floor
//      (translucent deep blue, darker toward bottom)
//   2. Surface highlight — thin bright strip along the wave tops
//   3. Surface line vertices form a smooth wave using per-column heights
void RenderWater() {
    if (g_numWaterRegions == 0) return;

    // Always use standard alpha blend — keeps land puddles correct and lets
    // us control coverage by drawing a solid fill pass first on boat level.
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

    D3DXMATRIX matWVP = g_matView * g_matProj;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

    float uvScrollX = g_fScrollPos * 0.02f;

    if (!g_pSolidColourShader) {
        // Shader not compiled — skip water render to avoid garbage output
        g_pd3dDevice->SetPixelShader(g_pPixelShader);
        return;
    }

    // Use solid colour shader throughout — never samples any texture,
    // so no boat/terrain texture can bleed through regardless of sampler state.
    g_pd3dDevice->SetPixelShader(g_pSolidColourShader);
    g_pd3dDevice->SetTexture(0, NULL);  // belt-and-suspenders

    for (int ri = 0; ri < g_numWaterRegions; ri++) {
        const WaterRegion& wr = g_waterRegions[ri];
        if (!wr.active || wr.numCols < 2) continue;

        float colW = wr.width / (float)wr.numCols;

        // ── Water body: same per-column approach for ALL levels ───────────
        // Colours match the Level 1 puddle exactly. No separate boat-level
        // code path — the same code that makes Level 1 look great is reused.
        // Boat level just has a flat terrain so there are no wall columns to skip.

        // Pass 1: solid water fill (terrain-clipped)
        float cFill[4] = { 0.15f, 0.48f, 0.75f, 0.85f };
        g_pd3dDevice->SetPixelShaderConstantF(0, cFill, 1);
        for (int c = 0; c < wr.numCols - 1; c++) {
            float wx0 = wr.x + c * colW,  wx1 = wr.x + (c+1) * colW;
            float sy0 = wr.restY + wr.cols[c].h;
            float sy1 = wr.restY + wr.cols[c+1].h;
            float fy0 = GetTerrainHeightAtX(wx0);
            float fy1 = GetTerrainHeightAtX(wx1);
            // Wall skip — on boat level terrain is flat so this rarely fires
            if (fy0 >= wr.restY && fy1 >= wr.restY) continue;
            if (sy0 < fy0) sy0 = fy0;
            if (sy1 < fy1) sy1 = fy1;
            if (fy0 > sy0) fy0 = sy0;
            if (fy1 > sy1) fy1 = sy1;
            if (fabsf(sy0 - fy0) < 0.01f && fabsf(sy1 - fy1) < 0.01f) continue;
            float sx0 = wx0 - g_fScrollPos, sx1 = wx1 - g_fScrollPos;
            // Boat level: extend floor down to cover hull keel below terrain
            if (g_isBoatLevel) { fy0 -= 1.5f; fy1 -= 1.5f; }
            // z depth: boat level must be CLOSER than car (z=-0.1) to overlay hull.
            // Land levels must be FARTHER than car so car renders on top of water.
            float wz = g_isBoatLevel ? -0.12f : -0.04f;
            TEXVERTEX v[4];
            v[0].x=sx0; v[0].y=sy0; v[0].z=wz; v[0].u=0; v[0].v=0;
            v[1].x=sx1; v[1].y=sy1; v[1].z=wz; v[1].u=1; v[1].v=0;
            v[2].x=sx0; v[2].y=fy0; v[2].z=wz; v[2].u=0; v[2].v=1;
            v[3].x=sx1; v[3].y=fy1; v[3].z=wz; v[3].u=1; v[3].v=1;
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(TEXVERTEX));
        }

        // Pass 2: surface highlight strip (follows wave heights)
        float cSurf[4] = { 0.60f, 0.90f, 1.0f, 0.75f };
        g_pd3dDevice->SetPixelShaderConstantF(0, cSurf, 1);
        const float SH = 0.07f;
        for (int c = 0; c < wr.numCols - 1; c++) {
            float wx0 = wr.x + c * colW,  wx1 = wr.x + (c+1) * colW;
            float sy0 = wr.restY + wr.cols[c].h;
            float sy1 = wr.restY + wr.cols[c+1].h;
            float hy0 = GetTerrainHeightAtX(wx0);
            float hy1 = GetTerrainHeightAtX(wx1);
            if (hy0 >= wr.restY && hy1 >= wr.restY) continue;
            if (sy0 < hy0) sy0 = hy0;
            if (sy1 < hy1) sy1 = hy1;
            float sx0 = wx0 - g_fScrollPos, sx1 = wx1 - g_fScrollPos;
            TEXVERTEX vs[4];
            float hz = g_isBoatLevel ? -0.11f : -0.035f;
            vs[0].x=sx0; vs[0].y=sy0+SH; vs[0].z=hz; vs[0].u=0; vs[0].v=0;
            vs[1].x=sx1; vs[1].y=sy1+SH; vs[1].z=hz; vs[1].u=1; vs[1].v=0;
            vs[2].x=sx0; vs[2].y=sy0;     vs[2].z=hz; vs[2].u=0; vs[2].v=1;
            vs[3].x=sx1; vs[3].y=sy1;     vs[3].z=hz; vs[3].u=1; vs[3].v=1;
            g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vs, sizeof(TEXVERTEX));
        }
    }
    // Restore game shader
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
    g_pd3dDevice->SetTexture(0, NULL);
    // Restore game shader + clear texture slot
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
    g_pd3dDevice->SetTexture(0, NULL);
    // Restore game shader and pixel shader state
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
    g_pd3dDevice->SetTexture(0, NULL);
}

// ── Particle rendering ───────────────────────────────────────────────────
// Uses the standard game pixel shader (tex2D) with level-aware particle textures.
// Smoke always uses smoke.png. Dirt/dust/landing uses:
//   Level 2 (snow)  -> snow.png
//   Level 5 (cave)  -> rocks.png
//   All others      -> dirt.png
// Alpha is written into the vertex shader WVP trick — we modulate the texture
// alpha by setting a ps constant that the game shader multiplies in.
// Actually: game shader just does tex2D, so alpha comes from the texture's
// alpha channel. We control fade by scaling the quad's alpha via a tint.
//
// Simpler correct approach for Xbox 360: use the EXISTING pixel shader
// (which samples the texture) and rely on the texture's own alpha channel
// for shape. We modulate the overall alpha by using a second ps constant
// the particle shader reads as a multiplier.
//
// We keep g_pParticlePixelShader but now give it a texture + alpha uniform:
//   float4 main(): return tex2D(s0, uv) * float4(1,1,1,alpha)
void RenderParticles() {
    if (g_numParticles == 0) return;

    // ── Select which terrain-particle texture to use this level ──────────
    LPDIRECT3DTEXTURE9 terrainParticleTex = g_pTexParticleDirt;
    if (g_currentLevel == LEVEL_2)
        terrainParticleTex = g_pTexParticleSnow;
    else if (g_currentLevel == LEVEL_5 || g_currentLevel == LEVEL_6)
        terrainParticleTex = g_pTexParticleRocks;  // L5=cave rocks, L6=moon rocks

    // Use the existing game pixel shader — it samples tex2D and outputs colour.
    // We modulate alpha via SetPixelShaderConstantF(0) which the particle shader
    // uses as a multiplier: output = tex2D(...) * float4(1,1,1,alphaConst)
    // Switch to particle shader so we can control alpha fade cleanly.
    if (!g_pParticlePixelShader) return;
    g_pd3dDevice->SetPixelShader(g_pParticlePixelShader);

    // ✅ Additive blending: dst = src*srcAlpha + dst*1
    //    Black pixels (r=0,g=0,b=0) contribute nothing to the frame — no dark boxes.
    //    Bright particle shapes add their colour on top of whatever is behind.
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    D3DXMATRIX matWVP = g_matView * g_matProj;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

    LPDIRECT3DTEXTURE9 lastTex = NULL;  // avoid redundant SetTexture calls

    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle& p = g_particles[i];
        if (p.type == PARTICLE_NONE || p.alpha <= 0.0f) continue;

        // ── Pick texture for this particle ────────────────────────────────
        LPDIRECT3DTEXTURE9 tex;
        if (p.type == PARTICLE_SMOKE)
            tex = g_pTexParticleSmoke;
        else if (p.type == PARTICLE_WATER)
            tex = NULL;   // solid colour — no texture needed for droplets
        else if (p.type == PARTICLE_WAKE)
            tex = NULL;   // wake: solid colour only — avoids texture bleed artefacts
        else
            tex = terrainParticleTex;

        if (tex != lastTex) {
            g_pd3dDevice->SetTexture(0, tex);
            lastTex = tex;
        }

        // ── Set colour / alpha for this particle type ─────────────────────
        float col[4];
        if (p.type == PARTICLE_WATER) {
            col[0] = 0.7f + p.life * 0.3f;
            col[1] = 0.88f;
            col[2] = 1.0f;
            col[3] = p.alpha * 0.85f;
        } else if (p.type == PARTICLE_WAKE) {
            // Wake foam: bright white, semi-transparent
            col[0] = 0.95f; col[1] = 0.98f; col[2] = 1.0f;
            col[3] = p.alpha * 0.70f;
        } else {
            col[0] = 1.0f; col[1] = 1.0f; col[2] = 1.0f;
            col[3] = p.alpha * 0.9f;
        }
        g_pd3dDevice->SetPixelShaderConstantF(0, col, 1);

        float sx = p.x - g_fScrollPos;
        float sy = p.y;
        float hs = p.size * 0.5f;

        TEXVERTEX v[4];
        v[0].x = sx - hs; v[0].y = sy + hs; v[0].z = -0.05f; v[0].u = 0; v[0].v = 0;
        v[1].x = sx + hs; v[1].y = sy + hs; v[1].z = -0.05f; v[1].u = 1; v[1].v = 0;
        v[2].x = sx - hs; v[2].y = sy - hs; v[2].z = -0.05f; v[2].u = 0; v[2].v = 1;
        v[3].x = sx + hs; v[3].y = sy - hs; v[3].z = -0.05f; v[3].u = 1; v[3].v = 1;

        g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(TEXVERTEX));
    }

    // Restore normal alpha blend and game pixel shader
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
}

//============================================================================================
// RENDER SNOWFALL — ambient falling-snow overlay for the snow level (Level 2).
// Renders the snow_flake.png texture as two tiling fullscreen layers that scroll
// downward at different speeds, creating a parallax snowfall effect.
// Uses additive blending so the black background disappears and only the white
// snowflakes are visible on top of the scene.
//============================================================================================
void RenderSnowfall() {
    if (g_currentLevel != LEVEL_2) return;
    if (g_pTexSnowflake == NULL)   return;
    if (!g_pParticlePixelShader)   return;

    // ── Accumulate real time for continuous scroll (independent of car speed) ──
    static float snowTime = 0.0f;
    snowTime += g_Time.fElapsedTime;

    // ── Additive blending: black=invisible, white snowflakes add on top ───────
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    // Disable depth test — this is a fullscreen overlay drawn after the scene
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    // Identity WVP: quad vertices are in NDC (-1..1) directly
    D3DXMATRIX matIdent;
    D3DXMatrixIdentity(&matIdent);
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matIdent, 4);

    g_pd3dDevice->SetTexture(0, g_pTexSnowflake);
    g_pd3dDevice->SetPixelShader(g_pParticlePixelShader);

    // ── Two parallax layers for depth ────────────────────────────────────────
    // Back layer: slower, larger tile, dimmer
    // Front layer: faster, smaller tile, brighter
    float speeds[2]  = { 0.06f, 0.12f };
    float scales[2]  = { 2.5f,  1.8f  };
    float alphas[2]  = { 0.35f, 0.55f };
    float winds[2]   = { 0.015f, 0.025f };

    for (int l = 0; l < 2; l++) {
        // V increases over time → texture shifts up on screen → snow falls down
        // Negative V scroll = pattern moves downward on screen
        float vScroll = -snowTime * speeds[l];
        float uScroll =  snowTime * winds[l];   // slight horizontal wind drift
        float s = scales[l];

        float col[4] = { 1.0f, 1.0f, 1.0f, alphas[l] };
        g_pd3dDevice->SetPixelShaderConstantF(0, col, 1);

        TEXVERTEX sq[4];
        sq[0].x = -1; sq[0].y =  1; sq[0].z = 0; sq[0].u = uScroll;     sq[0].v = vScroll;
        sq[1].x =  1; sq[1].y =  1; sq[1].z = 0; sq[1].u = uScroll + s;  sq[1].v = vScroll;
        sq[2].x = -1; sq[2].y = -1; sq[2].z = 0; sq[2].u = uScroll;      sq[2].v = vScroll + s;
        sq[3].x =  1; sq[3].y = -1; sq[3].z = 0; sq[3].u = uScroll + s;  sq[3].v = vScroll + s;
        g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, sq, sizeof(TEXVERTEX));
    }

    // ── Restore state ────────────────────────────────────────────────────────
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
}

void Render() {

    if (g_gameState == STATE_TITLE_SCREEN) {
        RenderTitleScreen();
        return;
    }
    
    if (g_gameState == STATE_MAIN_MENU) {
        RenderMainMenu();
        return;
    }
    
    if (g_gameState == STATE_CAR_SELECT) {
        RenderCarSelectScreen();
        return;
    }
    
    if (g_gameState == STATE_SETTINGS) {
        RenderSettingsScreen();
        return;
    }
    
    if (g_gameState == STATE_ABOUT) {
        RenderAboutScreen();
        return;
    }
    
    if (g_gameState == STATE_LEVEL_SELECT) {
        RenderLevelSelectScreen();
        return;
    }
    
    if (g_gameState == STATE_PAUSE_MENU) {
        RenderPauseMenu();
        return;
    }
    
    g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(135,206,235), 1.0f, 0);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    g_pd3dDevice->SetVertexDeclaration(g_pVertexDecl);
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetPixelShader(g_pPixelShader);

    RenderBackground();

    LPDIRECT3DTEXTURE9 terrainTex = g_pTexTerrain;
    if (g_currentLevel == LEVEL_2) terrainTex = g_pTexTerrain_L2;
    else if (g_currentLevel == LEVEL_3) terrainTex = g_pTexTerrain_L3;
    else if (g_currentLevel == LEVEL_4) terrainTex = g_pTexTerrain_L4;
    else if (g_currentLevel == LEVEL_5) terrainTex = g_pTexTerrain_L5;
    else if (g_currentLevel == LEVEL_6) terrainTex = g_pTexTerrain_L6;
    else if (g_currentLevel == LEVEL_7) terrainTex = g_pTexTerrain_L7;
    else if (g_currentLevel >= LEVEL_CUSTOM_1) {
        int ci = g_currentLevel - (int)LEVEL_CUSTOM_1;
        if (ci >= 0 && ci < g_numCustomLevels && g_customLevels[ci].loaded
            && g_customLevels[ci].pTexTerrain != NULL)
            terrainTex = g_customLevels[ci].pTexTerrain;
    }
    
    g_pd3dDevice->SetTexture(0, terrainTex);
    D3DXMATRIX matWVP = g_matView * g_matProj;
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);
    g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(TEXVERTEX));
    g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, (NUM_DISPLAY_SEGMENTS*2)-2);

    // ✅ RENDER BRIDGE PLANKS (between terrain and car so car drives on top)
    RenderBridge();

    // ✅ RENDER WATER (before car on land levels; after car on boat level so water overlaps hull)
    if (!g_isBoatLevel) RenderWater();

    // ✅ RENDER PARTICLES before car so they appear behind it.
    RenderParticles();

    if (g_carChassis && g_wheelRear && g_wheelFront) {
        b2Vec2 chassisPos = g_carChassis->GetPosition();
        float chassisAngle = g_carChassis->GetAngle();

        b2Vec2 rearPos = g_wheelRear->GetPosition();
        b2Vec2 frontPos = g_wheelFront->GetPosition();

        float wheelSpin = g_fScrollPos * 4.5f;

        if (g_isBoatLevel) {
            // Disable depth writes for boat body: water (drawn after) must be able
            // to overdraw it. The boat is closer to the camera (z=-0.1) than the
            // water quads (z=-0.035/-0.04), so without this it wins the depth test
            // and blocks water from rendering — leaving ghost images of the hull.
            g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        }

        RenderBody(chassisPos.x - g_fScrollPos, chassisPos.y,
                   g_visualCarWidth, g_visualCarHeight, chassisAngle);

        // Spinning prop on airplane — at the nose (front of fuselage).
        // Cessna texture faces right, nose hub at ~95% of image width,
        // which is 0.45 * fuselage_width from the body centre.
        if (g_isAirplane) {
            float noseX = (chassisPos.x - g_fScrollPos)
                        + cosf(chassisAngle) * g_visualCarWidth * 0.44f;
            float noseY = chassisPos.y + g_rideHeightOffset
                        + sinf(chassisAngle) * g_visualCarWidth * 0.44f;
            RenderProp(noseX, noseY, chassisAngle);
        }

        if (g_isBoatLevel) {
            g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
        } else {
            RenderWheel(rearPos.x  - g_fScrollPos, rearPos.y,  0, 0, chassisAngle, -wheelSpin);
            RenderWheel(frontPos.x - g_fScrollPos, frontPos.y, 0, 0, chassisAngle, -wheelSpin);
        }
    }

    // ✅ On boat level: water renders AFTER boat — hull appears submerged in water
    if (g_isBoatLevel) RenderWater();

    // Restore standard alpha blend after any additive water draw
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    // ✅ RENDER SNOWFALL OVERLAY (Level 2 only — after scene, before HUD)
    RenderSnowfall();

    // ✅ RENDER HUD GAUGES (SPEEDOMETER & BOOST METER)
    RenderHUDGauges();

    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}
