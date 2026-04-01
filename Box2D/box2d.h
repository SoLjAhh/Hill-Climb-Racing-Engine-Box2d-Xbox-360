// MIT License

// Copyright (c) 2019 Erin Catto

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef BOX2D_H
#define BOX2D_H

// These include files constitute the main Box2D API

#include "common/b2settings.h"
#include "common/b2draw.h"
#include "common/b2timer.h"

#include "collision/shapes/b2chainshape.h"
#include "collision/shapes/b2circleshape.h"
#include "collision/shapes/b2edgeshape.h"
#include "collision/shapes/b2polygonshape.h"

#include "collision/b2broadphase.h"
#include "collision/b2dynamictree.h"

#include "dynamics/b2body.h"
#include "contacts/b2contact.h"
#include "dynamics/b2fixture.h"
#include "dynamics/b2timestep.h"
#include "dynamics/b2world.h"
#include "dynamics/b2worldcallbacks.h"

#include "dynamics/joints/b2distancejoint.h"
#include "dynamics/joints/b2frictionjoint.h"
#include "dynamics/joints/b2gearjoint.h"
#include "dynamics/joints/b2motorjoint.h"
#include "dynamics/joints/b2mousejoint.h"
#include "dynamics/joints/b2prismaticjoint.h"
#include "dynamics/joints/b2pulleyjoint.h"
#include "dynamics/joints/b2revolutejoint.h"
#include "dynamics/joints/b2ropejoint.h"
#include "dynamics/joints/b2weldjoint.h"
#include "dynamics/joints/b2wheeljoint.h"

#endif
