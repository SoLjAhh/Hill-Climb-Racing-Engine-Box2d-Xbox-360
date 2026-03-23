#pragma once
#include "stdafx.h"
#include "globals.h"

//============================================================================================
// CAR PHYSICS PARAMETER CONSTANTS (all 22 built-in cars)
// Y1 = 5.0f (defined in main.cpp)
//============================================================================================

//============================================================================================

// jeep Car
const float JEEP_CAR_WIDTH                = 3.1f; // visual setting for car body png scale
const float JEEP_CAR_HEIGHT               = 1.25f; // visual setting for car body png scale
const float JEEP_WHEEL_RADIUS             = 0.35f; // visual setting for car wheel png scale
const float JEEP_WHEEL_JOINT_FREQUENCY    = 4.0f;
const float JEEP_WHEEL_JOINT_DAMPING      = 0.8f;
const float JEEP_MOTOR_MAX_SPEED          = 60.0f;
const float JEEP_MOTOR_TORQUE_REAR        = 7.0f;
const float JEEP_MOTOR_TORQUE_FRONT       = 5.0f;
const float JEEP_RIDE_HEIGHT_OFFSET       = 0.0f; // visual setting for car body png height
const float JEEP_WHEEL_REAR_OFFSET_X      = -0.96f; // visual (+) moves wheel forward (-) moves wheel backward
const float JEEP_WHEEL_REAR_START_Y       = 1.5f*Y1+0.35f;
const float JEEP_WHEEL_FRONT_OFFSET_X     = 1.0f; // visual (+) moves wheel forward (-) moves wheel backward
const float JEEP_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.4f;

// Rally Car
const float RALLY_CAR_WIDTH              = 3.2f;
const float RALLY_CAR_HEIGHT             = 1.2f;
const float RALLY_WHEEL_RADIUS           = 0.32f;
const float RALLY_WHEEL_JOINT_FREQUENCY  = 4.5f;
const float RALLY_WHEEL_JOINT_DAMPING    = 0.9f;
const float RALLY_MOTOR_MAX_SPEED        = 70.0f;
const float RALLY_MOTOR_TORQUE_REAR      = 6.0f;
const float RALLY_MOTOR_TORQUE_FRONT     = 4.5f;
const float RALLY_RIDE_HEIGHT_OFFSET     = -0.2f;
const float RALLY_WHEEL_REAR_OFFSET_X    = -0.98f;
const float RALLY_WHEEL_REAR_START_Y     = 1.5f*Y1+0.35f;
const float RALLY_WHEEL_FRONT_OFFSET_X   = 1.0f;
const float RALLY_WHEEL_FRONT_START_Y    = 1.5f*Y1+0.4f;

// Monster Truck
const float MONSTER_CAR_WIDTH            = 3.1f;
const float MONSTER_CAR_HEIGHT           = 1.2f;
const float MONSTER_WHEEL_RADIUS         = 0.5f;
const float MONSTER_WHEEL_JOINT_FREQUENCY = 3.0f;
const float MONSTER_WHEEL_JOINT_DAMPING  = 0.7f;
const float MONSTER_MOTOR_MAX_SPEED      = 50.0f;
const float MONSTER_MOTOR_TORQUE_REAR    = 10.0f;
const float MONSTER_MOTOR_TORQUE_FRONT   = 7.0f;
const float MONSTER_RIDE_HEIGHT_OFFSET   = 0.0f;
const float MONSTER_WHEEL_REAR_OFFSET_X  = -1.0f;
const float MONSTER_WHEEL_REAR_START_Y   = 1.5f*Y1+0.3f;
const float MONSTER_WHEEL_FRONT_OFFSET_X = 1.1f;
const float MONSTER_WHEEL_FRONT_START_Y  = 1.5f*Y1+0.35f;

// ✅ Quad Bike (lightweight, agile, fast)
const float QUAD_BIKE_CAR_WIDTH              = 3.1f;
const float QUAD_BIKE_CAR_HEIGHT             = 1.5f;
const float QUAD_BIKE_WHEEL_RADIUS           = 0.35f;
const float QUAD_BIKE_WHEEL_JOINT_FREQUENCY  = 5.0f;
const float QUAD_BIKE_WHEEL_JOINT_DAMPING    = 0.7f;
const float QUAD_BIKE_MOTOR_MAX_SPEED        = 75.0f;
const float QUAD_BIKE_MOTOR_TORQUE_REAR      = 5.5f;
const float QUAD_BIKE_MOTOR_TORQUE_FRONT     = 4.0f;
const float QUAD_BIKE_RIDE_HEIGHT_OFFSET     = -0.1f;
const float QUAD_BIKE_WHEEL_REAR_OFFSET_X    = -0.9f;
const float QUAD_BIKE_WHEEL_REAR_START_Y     = 1.5f*Y1+0.35f;
const float QUAD_BIKE_WHEEL_FRONT_OFFSET_X   = 1.0f;
const float QUAD_BIKE_WHEEL_FRONT_START_Y    = 1.5f*Y1+0.4f;

// ✅ Race Car (sleek, very fast, high performance)
const float RACE_CAR_WIDTH                    = 3.4f;
const float RACE_CAR_HEIGHT                   = 1.3f;
const float RACE_CAR_WHEEL_RADIUS             = 0.32f;
const float RACE_CAR_WHEEL_JOINT_FREQUENCY    = 4.8f;
const float RACE_CAR_WHEEL_JOINT_DAMPING      = 0.85f;
const float RACE_CAR_MOTOR_MAX_SPEED          = 72.0f;
const float RACE_CAR_MOTOR_TORQUE_REAR        = 6.5f;
const float RACE_CAR_MOTOR_TORQUE_FRONT       = 4.8f;
const float RACE_CAR_RIDE_HEIGHT_OFFSET       = -0.1f;
const float RACE_CAR_WHEEL_REAR_OFFSET_X      = -0.9f;
const float RACE_CAR_WHEEL_REAR_START_Y       = 1.5f*Y1+0.35f;
const float RACE_CAR_WHEEL_FRONT_OFFSET_X     = 1.15f;
const float RACE_CAR_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.4f;

// Family Car - Balanced all-arounder
const float FAMILY_CAR_WIDTH                  = 3.9f;
const float FAMILY_CAR_HEIGHT                 = 2.0f;
const float FAMILY_CAR_WHEEL_RADIUS           = 0.32f;
const float FAMILY_CAR_WHEEL_JOINT_FREQUENCY  = 4.2f;
const float FAMILY_CAR_WHEEL_JOINT_DAMPING    = 0.8f;
const float FAMILY_CAR_MOTOR_MAX_SPEED        = 55.0f;
const float FAMILY_CAR_MOTOR_TORQUE_REAR      = 6.5f;
const float FAMILY_CAR_MOTOR_TORQUE_FRONT     = 4.8f;
const float FAMILY_CAR_RIDE_HEIGHT_OFFSET     = 0.1f;
const float FAMILY_CAR_WHEEL_REAR_OFFSET_X    = -0.9f;
const float FAMILY_CAR_WHEEL_REAR_START_Y     = 1.5f*Y1+0.35f;
const float FAMILY_CAR_WHEEL_FRONT_OFFSET_X   = 1.08f;
const float FAMILY_CAR_WHEEL_FRONT_START_Y    = 1.5f*Y1+0.4f;

// Old School - Slow but sturdy
const float OLD_SCHOOL_CAR_WIDTH              = 3.5f;
const float OLD_SCHOOL_CAR_HEIGHT             = 1.5f;
const float OLD_SCHOOL_CAR_WHEEL_RADIUS       = 0.4f;
const float OLD_SCHOOL_CAR_WHEEL_JOINT_FREQUENCY = 3.5f;
const float OLD_SCHOOL_CAR_WHEEL_JOINT_DAMPING = 0.6f;
const float OLD_SCHOOL_CAR_MOTOR_MAX_SPEED    = 45.0f;
const float OLD_SCHOOL_CAR_MOTOR_TORQUE_REAR  = 8.0f;
const float OLD_SCHOOL_CAR_MOTOR_TORQUE_FRONT = 6.0f;
const float OLD_SCHOOL_CAR_RIDE_HEIGHT_OFFSET = 0.0f;
const float OLD_SCHOOL_WHEEL_REAR_OFFSET_X    = -0.9f;
const float OLD_SCHOOL_WHEEL_REAR_START_Y     = 1.5f*Y1+0.35f;
const float OLD_SCHOOL_WHEEL_FRONT_OFFSET_X   = 1.02f;
const float OLD_SCHOOL_WHEEL_FRONT_START_Y    = 1.5f*Y1+0.4f;

// Paramedic - Ambulance, balanced speed
const float PARAMEDIC_CAR_WIDTH               = 4.5f;
const float PARAMEDIC_CAR_HEIGHT              = 2.3f;
const float PARAMEDIC_CAR_WHEEL_RADIUS        = 0.38f;
const float PARAMEDIC_CAR_WHEEL_JOINT_FREQUENCY = 5.0f;
const float PARAMEDIC_CAR_WHEEL_JOINT_DAMPING = 0.9f;
const float PARAMEDIC_CAR_MOTOR_MAX_SPEED     = 65.0f;
const float PARAMEDIC_CAR_MOTOR_TORQUE_REAR   = 7.5f;
const float PARAMEDIC_CAR_MOTOR_TORQUE_FRONT  = 5.5f;
const float PARAMEDIC_CAR_RIDE_HEIGHT_OFFSET  = 0.1f;   // reduced: was 0.5, caused wild flip offset
const float PARAMEDIC_WHEEL_REAR_OFFSET_X     = -0.9f;
const float PARAMEDIC_WHEEL_REAR_START_Y      = 1.5f*Y1+0.35f;
const float PARAMEDIC_WHEEL_FRONT_OFFSET_X    = 1.05f;  // reduced: was 1.72, too far outside chassis
const float PARAMEDIC_WHEEL_FRONT_START_Y     = 1.5f*Y1+0.4f;

// Buggy - Light and bouncy
const float BUGGY_CAR_WIDTH                   = 2.9f;
const float BUGGY_CAR_HEIGHT                  = 1.1f;
const float BUGGY_CAR_WHEEL_RADIUS            = 0.32f;
const float BUGGY_CAR_WHEEL_JOINT_FREQUENCY   = 5.5f;
const float BUGGY_CAR_WHEEL_JOINT_DAMPING     = 0.8f;
const float BUGGY_CAR_MOTOR_MAX_SPEED         = 78.0f;
const float BUGGY_CAR_MOTOR_TORQUE_REAR       = 5.0f;
const float BUGGY_CAR_MOTOR_TORQUE_FRONT      = 3.8f;
const float BUGGY_CAR_RIDE_HEIGHT_OFFSET      = -0.2f;
const float BUGGY_WHEEL_REAR_OFFSET_X         = -0.93f;
const float BUGGY_WHEEL_REAR_START_Y          = 1.5f*Y1+0.35f;
const float BUGGY_WHEEL_FRONT_OFFSET_X        = 0.9f;
const float BUGGY_WHEEL_FRONT_START_Y         = 1.5f*Y1+0.4f;

// Sprinter - Super fast
const float SPRINTER_CAR_WIDTH                = 4.0f;
const float SPRINTER_CAR_HEIGHT               = 1.2f;
const float SPRINTER_CAR_WHEEL_RADIUS         = 0.3f;
const float SPRINTER_CAR_WHEEL_JOINT_FREQUENCY = 5.0f;
const float SPRINTER_CAR_WHEEL_JOINT_DAMPING = 0.9f;
const float SPRINTER_CAR_MOTOR_MAX_SPEED      = 85.0f;
const float SPRINTER_CAR_MOTOR_TORQUE_REAR    = 6.0f;
const float SPRINTER_CAR_MOTOR_TORQUE_FRONT   = 4.5f;
const float SPRINTER_CAR_RIDE_HEIGHT_OFFSET   = -0.05f;
const float SPRINTER_WHEEL_REAR_OFFSET_X      = -1.2f; 
const float SPRINTER_WHEEL_REAR_START_Y       = 1.5f*Y1+0.35f;
const float SPRINTER_WHEEL_FRONT_OFFSET_X     = 0.8f; 
const float SPRINTER_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.4f;

// Police Car - Fast and responsive
const float POLICE_CAR_WIDTH                  = 4.0f;
const float POLICE_CAR_HEIGHT                 = 1.2f;
const float POLICE_CAR_WHEEL_RADIUS           = 0.34f;
const float POLICE_CAR_WHEEL_JOINT_FREQUENCY  = 4.6f;
const float POLICE_CAR_WHEEL_JOINT_DAMPING    = 0.85f;
const float POLICE_CAR_MOTOR_MAX_SPEED        = 72.0f;
const float POLICE_CAR_MOTOR_TORQUE_REAR      = 6.8f;
const float POLICE_CAR_MOTOR_TORQUE_FRONT     = 5.0f;
const float POLICE_CAR_RIDE_HEIGHT_OFFSET     = -0.05f;
const float POLICE_WHEEL_REAR_OFFSET_X        = -0.9f;
const float POLICE_WHEEL_REAR_START_Y         = 1.5f*Y1+0.35f;
const float POLICE_WHEEL_FRONT_OFFSET_X       = 1.05f;
const float POLICE_WHEEL_FRONT_START_Y        = 1.5f*Y1+0.4f;

// Stunt Rider - Motorcycle, ultra-light and bouncy
const float STUNT_RIDER_CAR_WIDTH             = 2.4f;
const float STUNT_RIDER_CAR_HEIGHT            = 1.1f;
const float STUNT_RIDER_CAR_WHEEL_RADIUS      = 0.28f;
const float STUNT_RIDER_CAR_WHEEL_JOINT_FREQUENCY = 6.0f;
const float STUNT_RIDER_CAR_WHEEL_JOINT_DAMPING = 0.6f;
const float STUNT_RIDER_CAR_MOTOR_MAX_SPEED   = 80.0f;
const float STUNT_RIDER_CAR_MOTOR_TORQUE_REAR = 4.5f;
const float STUNT_RIDER_CAR_MOTOR_TORQUE_FRONT = 3.2f;
const float STUNT_RIDER_CAR_RIDE_HEIGHT_OFFSET = -0.2f;
const float STUNT_RIDER_WHEEL_REAR_OFFSET_X   = -0.75f;
const float STUNT_RIDER_WHEEL_REAR_START_Y    = 1.5f*Y1+0.35f;
const float STUNT_RIDER_WHEEL_FRONT_OFFSET_X  = 1.0f;
const float STUNT_RIDER_WHEEL_FRONT_START_Y   = 1.5f*Y1+0.4f;

// Super Diesel - Heavy truck
const float SUPER_DIESEL_CAR_WIDTH            = 3.4f;
const float SUPER_DIESEL_CAR_HEIGHT           = 1.25f;
const float SUPER_DIESEL_CAR_WHEEL_RADIUS     = 0.48f;
const float SUPER_DIESEL_CAR_WHEEL_JOINT_FREQUENCY = 2.8f;
const float SUPER_DIESEL_CAR_WHEEL_JOINT_DAMPING = 0.65f;
const float SUPER_DIESEL_CAR_MOTOR_MAX_SPEED  = 48.0f;
const float SUPER_DIESEL_CAR_MOTOR_TORQUE_REAR = 11.0f;
const float SUPER_DIESEL_CAR_MOTOR_TORQUE_FRONT = 8.0f;
const float SUPER_DIESEL_CAR_RIDE_HEIGHT_OFFSET = 0.2f;
const float SUPER_DIESEL_WHEEL_REAR_OFFSET_X  = -1.12f;
const float SUPER_DIESEL_WHEEL_REAR_START_Y   = 1.5f*Y1+0.3f;
const float SUPER_DIESEL_WHEEL_FRONT_OFFSET_X = 1.15f;
const float SUPER_DIESEL_WHEEL_FRONT_START_Y  = 1.5f*Y1+0.35f;

// Super Jeep - Off-road capable
const float SUPER_JEEP_CAR_WIDTH              = 3.15f;
const float SUPER_JEEP_CAR_HEIGHT             = 1.1f;
const float SUPER_JEEP_CAR_WHEEL_RADIUS       = 0.45f;
const float SUPER_JEEP_CAR_WHEEL_JOINT_FREQUENCY = 3.8f;
const float SUPER_JEEP_CAR_WHEEL_JOINT_DAMPING = 0.7f;
const float SUPER_JEEP_CAR_MOTOR_MAX_SPEED    = 58.0f;
const float SUPER_JEEP_CAR_MOTOR_TORQUE_REAR  = 9.0f;
const float SUPER_JEEP_CAR_MOTOR_TORQUE_FRONT = 6.5f;
const float SUPER_JEEP_CAR_RIDE_HEIGHT_OFFSET = 0.12f;
const float SUPER_JEEP_WHEEL_REAR_OFFSET_X    = -0.9f;
const float SUPER_JEEP_WHEEL_REAR_START_Y     = 1.5f*Y1+0.3f;
const float SUPER_JEEP_WHEEL_FRONT_OFFSET_X   = 1.0f;
const float SUPER_JEEP_WHEEL_FRONT_START_Y    = 1.5f*Y1+0.35f;

// Trophy Truck - Powerful heavyweight
const float TROPHY_TRUCK_CAR_WIDTH            = 4.0f;
const float TROPHY_TRUCK_CAR_HEIGHT           = 1.3f;
const float TROPHY_TRUCK_CAR_WHEEL_RADIUS     = 0.50f;
const float TROPHY_TRUCK_CAR_WHEEL_JOINT_FREQUENCY = 3.2f;
const float TROPHY_TRUCK_CAR_WHEEL_JOINT_DAMPING = 0.68f;
const float TROPHY_TRUCK_CAR_MOTOR_MAX_SPEED  = 52.0f;
const float TROPHY_TRUCK_CAR_MOTOR_TORQUE_REAR = 12.0f;
const float TROPHY_TRUCK_CAR_MOTOR_TORQUE_FRONT = 8.5f;
const float TROPHY_TRUCK_CAR_RIDE_HEIGHT_OFFSET = 0.0f;
const float TROPHY_TRUCK_WHEEL_REAR_OFFSET_X  = -1.1f;
const float TROPHY_TRUCK_WHEEL_REAR_START_Y   = 1.5f*Y1+0.3f;
const float TROPHY_TRUCK_WHEEL_FRONT_OFFSET_X = 1.2f;
const float TROPHY_TRUCK_WHEEL_FRONT_START_Y  = 1.5f*Y1+0.35f;

// Chevelle - powerful muscle car
const float CHEVELLE_CAR_WIDTH                = 4.0f;
const float CHEVELLE_CAR_HEIGHT               = 1.2f;
const float CHEVELLE_CAR_WHEEL_RADIUS         = 0.3f;
const float CHEVELLE_CAR_WHEEL_JOINT_FREQUENCY = 5.0f;
const float CHEVELLE_CAR_WHEEL_JOINT_DAMPING = 0.9f;
const float CHEVELLE_CAR_MOTOR_MAX_SPEED      = 85.0f;
const float CHEVELLE_CAR_MOTOR_TORQUE_REAR    = 6.0f;
const float CHEVELLE_CAR_MOTOR_TORQUE_FRONT   = 4.5f;
const float CHEVELLE_CAR_RIDE_HEIGHT_OFFSET   = -0.1f;
const float CHEVELLE_WHEEL_REAR_OFFSET_X      = -1.0f; 
const float CHEVELLE_WHEEL_REAR_START_Y       = 1.5f*Y1+0.35f;
const float CHEVELLE_WHEEL_FRONT_OFFSET_X     = 1.3f; 
const float CHEVELLE_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.4f;

//============================================================================================
// ✅ NEW CARS - EXTENDED COLLECTION (22 Cars Total)
//============================================================================================

// Blazer - SUV with good balance
const float BLAZER_CAR_WIDTH                = 3.6f;
const float BLAZER_CAR_HEIGHT               = 1.6f;
const float BLAZER_CAR_WHEEL_RADIUS         = 0.36f;
const float BLAZER_CAR_WHEEL_JOINT_FREQUENCY = 4.2f;
const float BLAZER_CAR_WHEEL_JOINT_DAMPING = 0.85f;
const float BLAZER_CAR_MOTOR_MAX_SPEED      = 65.0f;
const float BLAZER_CAR_MOTOR_TORQUE_REAR    = 7.5f;
const float BLAZER_CAR_MOTOR_TORQUE_FRONT   = 5.5f;
const float BLAZER_CAR_RIDE_HEIGHT_OFFSET   = 0.05f;
const float BLAZER_WHEEL_REAR_OFFSET_X      = -0.95f;
const float BLAZER_WHEEL_REAR_START_Y       = 1.5f*Y1+0.36f;
const float BLAZER_WHEEL_FRONT_OFFSET_X     = 1.1f;
const float BLAZER_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.4f;

// Caprice - Heavy sedan, stable and slow
const float CAPRICE_CAR_WIDTH                = 3.8f;
const float CAPRICE_CAR_HEIGHT               = 1.8f;
const float CAPRICE_CAR_WHEEL_RADIUS         = 0.34f;
const float CAPRICE_CAR_WHEEL_JOINT_FREQUENCY = 3.8f;
const float CAPRICE_CAR_WHEEL_JOINT_DAMPING = 0.92f;
const float CAPRICE_CAR_MOTOR_MAX_SPEED      = 50.0f;
const float CAPRICE_CAR_MOTOR_TORQUE_REAR    = 8.5f;
const float CAPRICE_CAR_MOTOR_TORQUE_FRONT   = 6.0f;
const float CAPRICE_CAR_RIDE_HEIGHT_OFFSET   = 0.0f;
const float CAPRICE_WHEEL_REAR_OFFSET_X      = -0.8f;
const float CAPRICE_WHEEL_REAR_START_Y       = 1.5f*Y1+0.34f;
const float CAPRICE_WHEEL_FRONT_OFFSET_X     = 1.2f;
const float CAPRICE_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.38f;

// Cybertruck - Modern efficient truck
const float CYBERTRUCK_CAR_WIDTH                = 3.9f;
const float CYBERTRUCK_CAR_HEIGHT               = 1.7f;
const float CYBERTRUCK_CAR_WHEEL_RADIUS         = 0.38f;
const float CYBERTRUCK_CAR_WHEEL_JOINT_FREQUENCY = 4.3f;
const float CYBERTRUCK_CAR_WHEEL_JOINT_DAMPING = 0.87f;
const float CYBERTRUCK_CAR_MOTOR_MAX_SPEED      = 75.0f;
const float CYBERTRUCK_CAR_MOTOR_TORQUE_REAR    = 8.0f;
const float CYBERTRUCK_CAR_MOTOR_TORQUE_FRONT   = 5.8f;
const float CYBERTRUCK_CAR_RIDE_HEIGHT_OFFSET   = 0.0f;
const float CYBERTRUCK_WHEEL_REAR_OFFSET_X      = -1.15f;
const float CYBERTRUCK_WHEEL_REAR_START_Y       = 1.5f*Y1+0.38f;
const float CYBERTRUCK_WHEEL_FRONT_OFFSET_X     = 1.2f;
const float CYBERTRUCK_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.42f;

// Knight Rider (KITT) - Sports car, responsive and fast
const float KNIGHTRIDER_CAR_WIDTH                = 3.5f;
const float KNIGHTRIDER_CAR_HEIGHT               = 1.5f;
const float KNIGHTRIDER_CAR_WHEEL_RADIUS         = 0.33f;
const float KNIGHTRIDER_CAR_WHEEL_JOINT_FREQUENCY = 4.7f;
const float KNIGHTRIDER_CAR_WHEEL_JOINT_DAMPING = 0.88f;
const float KNIGHTRIDER_CAR_MOTOR_MAX_SPEED      = 88.0f;
const float KNIGHTRIDER_CAR_MOTOR_TORQUE_REAR    = 5.5f;
const float KNIGHTRIDER_CAR_MOTOR_TORQUE_FRONT   = 4.2f;
const float KNIGHTRIDER_CAR_RIDE_HEIGHT_OFFSET   = -0.2f;
const float KNIGHTRIDER_WHEEL_REAR_OFFSET_X      = -0.926f;
const float KNIGHTRIDER_WHEEL_REAR_START_Y       = 1.5f*Y1+0.33f;
const float KNIGHTRIDER_WHEEL_FRONT_OFFSET_X     = 0.915f;
const float KNIGHTRIDER_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.38f;

// School Bus - Heavy, powerful, slow
const float SCHOOLBUS_CAR_WIDTH                = 4.2f;
const float SCHOOLBUS_CAR_HEIGHT               = 1.8f;
const float SCHOOLBUS_CAR_WHEEL_RADIUS         = 0.4f;
const float SCHOOLBUS_CAR_WHEEL_JOINT_FREQUENCY = 3.9f;
const float SCHOOLBUS_CAR_WHEEL_JOINT_DAMPING = 0.95f;
const float SCHOOLBUS_CAR_MOTOR_MAX_SPEED      = 45.0f;
const float SCHOOLBUS_CAR_MOTOR_TORQUE_REAR    = 9.5f;
const float SCHOOLBUS_CAR_MOTOR_TORQUE_FRONT   = 7.0f;
const float SCHOOLBUS_CAR_RIDE_HEIGHT_OFFSET   = 0.15f;
const float SCHOOLBUS_WHEEL_REAR_OFFSET_X      = -1.05f;
const float SCHOOLBUS_WHEEL_REAR_START_Y       = 1.5f*Y1+0.4f;
const float SCHOOLBUS_WHEEL_FRONT_OFFSET_X     = 1.48f;
const float SCHOOLBUS_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.44f;

// Used Car - Light, economical, nimble
const float USEDCAR_CAR_WIDTH                = 3.5f;
const float USEDCAR_CAR_HEIGHT               = 1.2f;
const float USEDCAR_CAR_WHEEL_RADIUS         = 0.31f;
const float USEDCAR_CAR_WHEEL_JOINT_FREQUENCY = 4.5f;
const float USEDCAR_CAR_WHEEL_JOINT_DAMPING = 0.82f;
const float USEDCAR_CAR_MOTOR_MAX_SPEED      = 72.0f;
const float USEDCAR_CAR_MOTOR_TORQUE_REAR    = 5.8f;
const float USEDCAR_CAR_MOTOR_TORQUE_FRONT   = 4.5f;
const float USEDCAR_CAR_RIDE_HEIGHT_OFFSET   = -0.1f;   // reduced: was -0.3, too low
const float USEDCAR_WHEEL_REAR_OFFSET_X      = -0.95f;
const float USEDCAR_WHEEL_REAR_START_Y       = 1.5f*Y1+0.31f;
const float USEDCAR_WHEEL_FRONT_OFFSET_X     = 0.7f;   // increased: was 0.7, wheels too close together
const float USEDCAR_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.36f;

// All screen/terrain constants are defined in globals.h

// ✅ Yacht / Boat — floats on water, Level 7 only
const float BOAT_CAR_WIDTH                = 4.5f;

// ── Airplane constants ────────────────────────────────────────────────────
const float AIRPLANE_WIDTH                = 5.5f;   // fuselage visual width
const float AIRPLANE_HEIGHT               = 1.6f;   // fuselage visual height
const float AIRPLANE_THRUST               = 30.0f;  // prop thrust at full throttle
const float AIRPLANE_LIFT_FACTOR          = 0.08f;  // lift = factor * forwardSpeed^2 (world-up)
const float AIRPLANE_DRAG_FACTOR          = 0.06f;  // drag coefficient
const float AIRPLANE_PITCH_TORQUE         = 5.0f;   // gentle pitch from stick
const float AIRPLANE_ANGULAR_DAMP         = 6.0f;   // strong damp — no oscillation
const float AIRPLANE_LEVEL_TORQUE         = 18.0f;  // auto-level spring strength
const float AIRPLANE_MAX_BANK_ANGLE       = 0.55f;  // ~31 degrees max pitch
const float AIRPLANE_MOTOR_MAX_SPEED      = 85.0f;  // max forward speed
const float BOAT_CAR_HEIGHT               = 1.4f;
const float BOAT_WHEEL_RADIUS             = 0.38f;   // propeller proxies
const float BOAT_WHEEL_JOINT_FREQUENCY    = 1.5f;    // very soft — water compliance
const float BOAT_WHEEL_JOINT_DAMPING      = 0.95f;   // heavy damping
const float BOAT_MOTOR_MAX_SPEED          = 45.0f;   // slower top speed on water
const float BOAT_MOTOR_TORQUE_REAR        = 8.0f;    // rear propeller thrust
const float BOAT_MOTOR_TORQUE_FRONT       = 0.0f;    // no front drive
const float BOAT_RIDE_HEIGHT_OFFSET       = 0.3f;    // sits higher — hull above water
const float BOAT_WHEEL_REAR_OFFSET_X      = -1.2f;
const float BOAT_WHEEL_REAR_START_Y       = 1.5f*Y1+0.4f;
const float BOAT_WHEEL_FRONT_OFFSET_X     = 1.3f;
const float BOAT_WHEEL_FRONT_START_Y      = 1.5f*Y1+0.45f;

//============================================================================================
// FUNCTION DECLARATIONS
//============================================================================================

LPDIRECT3DTEXTURE9 GetTireTexture(CarType car);
LPDIRECT3DTEXTURE9 GetCarSelectionIcon(int index);
void               SetCarParameters(CarType car);
