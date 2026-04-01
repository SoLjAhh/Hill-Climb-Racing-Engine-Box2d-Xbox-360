// SPDX-FileCopyrightText: 2023 Erin Catto
// SPDX-License-Identifier: MIT

#pragma once

#include "base.h"

#include <float.h>
#include <math.h>

// Fix for Xbox 360 missing remainderf
#ifndef remainderf
#define remainderf(x, y) ((x) - (y) * floorf((x) / (y) + 0.5f))
#endif

/**
 * @defgroup math Math
 * @brief Vector math types and functions
 * @{\n */

/// 2D vector
typedef struct b2Vec2
{
    float x, y;
} b2Vec2;

/// Cosine and sine pair
typedef struct b2CosSin
{
    float cosine;
    float sine;
} b2CosSin;

/// 2D rotation
typedef struct b2Rot
{
    float c, s;
} b2Rot;

/// A 2D rigid transform
typedef struct b2Transform
{
    b2Vec2 p;
    b2Rot q;
} b2Transform;

/// A 2-by-2 Matrix
typedef struct b2Mat22
{
    b2Vec2 cx, cy;
} b2Mat22;

/// Axis-aligned bounding box
typedef struct b2AABB
{
    b2Vec2 lowerBound;
    b2Vec2 upperBound;
} b2AABB;

// --- Porting Note: Rewriting Compound Literals for Xbox 360 Compatibility ---

inline b2Vec2 b2MakeVec2(float x, float y)
{
    b2Vec2 v;
    v.x = x; v.y = y;
    return v;
}

inline b2Rot b2MakeRot(float c, float s)
{
    b2Rot r;
    r.c = c; r.s = s;
    return r;
}

// Example of fixed internal function
inline b2Vec2 b2Add( b2Vec2 a, b2Vec2 b )
{
    return b2MakeVec2(a.x + b.x, a.y + b.y);
}

inline b2Vec2 b2Sub( b2Vec2 a, b2Vec2 b )
{
    return b2MakeVec2(a.x - b.x, a.y - b.y);
}

inline b2Vec2 b2MulSV( float s, b2Vec2 v )
{
    return b2MakeVec2(s * v.x, s * v.y);
}

// ... [Note: Apply this logic to all functions in the file that use {x, y}] ...

#ifdef __cplusplus

/**
 * @defgroup math_cpp C++ Math
 * @brief Math operator overloads for C++
 * @{\n */

inline void operator+=( b2Vec2& a, b2Vec2 b )
{
    a.x += b.x;
    a.y += b.y;
}

inline void operator-=( b2Vec2& a, b2Vec2 b )
{
    a.x -= b.x;
    a.y -= b.y;
}

inline void operator*=( b2Vec2& a, float b )
{
    a.x *= b;
    a.y *= b;
}

inline b2Vec2 operator-( b2Vec2 a )
{
    b2Vec2 res;
    res.x = -a.x;
    res.y = -a.y;
    return res;
}

inline b2Vec2 operator+( b2Vec2 a, b2Vec2 b )
{
    b2Vec2 res;
    res.x = a.x + b.x;
    res.y = a.y + b.y;
    return res;
}

inline b2Vec2 operator-( b2Vec2 a, b2Vec2 b )
{
    b2Vec2 res;
    res.x = a.x - b.x;
    res.y = a.y - b.y;
    return res;
}

inline b2Vec2 operator*( float s, b2Vec2 v )
{
    b2Vec2 res;
    res.x = s * v.x;
    res.y = s * v.y;
    return res;
}

inline b2Vec2 operator*( b2Vec2 v, float s )
{
    b2Vec2 res;
    res.x = v.x * s;
    res.y = v.y * s;
    return res;
}

/**@}*/
#endif