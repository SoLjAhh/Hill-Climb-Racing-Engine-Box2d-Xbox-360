// SPDX-FileCopyrightText: 2023 Erin Catto
// SPDX-License-Identifier: MIT

#pragma once

#include "base.h"
#include "math_functions.h"

// Forward declarations for the compiler
typedef struct b2SimplexCache b2SimplexCache;
typedef struct b2Hull b2Hull;

/**
 * @defgroup geometry Geometry
 * @brief Geometry types and algorithms
 * @{
 */

/// The maximum number of vertices on a convex polygon.
#define B2_MAX_POLYGON_VERTICES 8

/// Low level ray cast input data
typedef struct b2RayCastInput
{
    b2Vec2 origin;
    b2Vec2 translation;
    float maxFraction;
} b2RayCastInput;

/// A distance proxy is used by the GJK algorithm.
typedef struct b2ShapeProxy
{
    b2Vec2 points[B2_MAX_POLYGON_VERTICES];
    int count;
    float radius;
} b2ShapeProxy;

// --- Porting Note: Define b2Plane early so later structs can use it ---
/// A 2D plane for collision manifolds
typedef struct b2Plane
{
    b2Vec2 normal;
    float center;
} b2Plane;

/// Low level ray cast output data
typedef struct b2RayCastOutput
{
    b2Vec2 normal;
    b2Vec2 point;
    float fraction;
    int iterations;
    bool hit;
} b2RayCastOutput;

/// Result of a distance query between two shapes
typedef struct b2DistanceOutput
{
    b2Vec2 pointA;
    b2Vec2 pointB;
    float distance;
    int iterations;
} b2DistanceOutput;

/// Result returned by b2ShapeDistance
typedef struct b2PlaneResult
{
    b2Plane plane; // Now the compiler knows what this is
    b2Vec2 point;
    float separation;
} b2PlaneResult;

/// These are collision planes that can be fed to b2SolvePlanes.
typedef struct b2CollisionPlane
{
    b2Plane plane;
    float pushLimit;
    float push;
    bool clipVelocity;
} b2CollisionPlane;

/// Result returned by b2SolvePlanes
typedef struct b2PlaneSolverResult
{
    b2Vec2 translation;
    int iterationCount;
} b2PlaneSolverResult;

// Function prototypes
#ifdef __cplusplus
extern "C" {
#endif

B2_API b2PlaneSolverResult b2SolvePlanes( b2Vec2 targetDelta, b2CollisionPlane* planes, int count );
B2_API b2Vec2 b2ClipVector( b2Vec2 vector, const b2CollisionPlane* planes, int count );

#ifdef __cplusplus
}
#endif

/** @} */