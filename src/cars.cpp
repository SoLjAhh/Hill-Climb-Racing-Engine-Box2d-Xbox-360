#include "stdafx.h"
#include "cars.h"
#include "config.h"
#include "globals.h"

//============================================================================================
// ✅ GET TIRE TEXTURE BY CAR TYPE
//============================================================================================

LPDIRECT3DTEXTURE9 GetTireTexture(CarType car) {
    if (car == CAR_BOAT)     return NULL;  // boat: no wheel sprite
    if (car == CAR_AIRPLANE) return NULL;  // airplane: sensor wheels, no sprite
    switch (car) {
        case CAR_JEEP:
        case CAR_RALLY:
        case CAR_QUAD_BIKE:
        case CAR_RACE_CAR:
        case CAR_USEDCAR: // NEW CAR
            return g_pTexWheel;

        case CAR_FAMILY:
        case CAR_POLICE:
        case CAR_OLD_SCHOOL:
        case CAR_CHEVELLE: // NEW CAR
        case CAR_CAPRICE: //NEW CAR
            return g_pTexWheelClassic;
            
        case CAR_MONSTER:
        case CAR_SUPER_JEEP:
        case CAR_BUGGY:
            return g_pTexWheelMonster;

        case CAR_BLAZER: // NEW CAR
        case CAR_KNIGHTRIDER: // NEW CAR
            return g_pTexWheelBlazer; // NEW WHEEL
            
        case CAR_SUPER_DIESEL:
        case CAR_CYBERTRUCK: // NEW CAR
            return g_pTexWheelDiesel;
            
        case CAR_PARAMEDIC:
        case CAR_SCHOOLBUS: // NEW CAR
            return g_pTexWheelParamedic;
            
        case CAR_STUNT_RIDER:
            return g_pTexWheelStunt;
            
        case CAR_TROPHY_TRUCK:
            return g_pTexWheelTrophy;
            
        case CAR_SPRINTER:
            return g_pTexWheelTuner;
            
        default: {
            // ✅ Custom car: return its custom wheel texture if loaded
            int customIdx = (int)car - CAR_CUSTOM_BASE;
            if (customIdx >= 0 && customIdx < g_numCustomCars
                && g_customCars[customIdx].loaded
                && g_customCars[customIdx].pTexWheel != NULL) {
                return g_customCars[customIdx].pTexWheel;
            }
            return g_pTexWheel;
        }
    }
}

//============================================================================================
// ✅ GET CAR SELECTION ICON BY INDEX
//============================================================================================

LPDIRECT3DTEXTURE9 GetCarSelectionIcon(int index) {
    switch (index) {
        case 0: return g_pTexCarSelectIcon_jeep;
        case 1: return g_pTexCarSelectIcon_Rally;
        case 2: return g_pTexCarSelectIcon_Monster;
        case 3: return g_pTexCarSelectIcon_QuadBike;
        case 4: return g_pTexCarSelectIcon_RaceCar;
        case 5: return g_pTexCarSelectIcon_Family;
        case 6: return g_pTexCarSelectIcon_OldSchool;
        case 7: return g_pTexCarSelectIcon_Paramedic;
        case 8: return g_pTexCarSelectIcon_Buggy;
        case 9: return g_pTexCarSelectIcon_Sprinter;
        case 10: return g_pTexCarSelectIcon_Police;
        case 11: return g_pTexCarSelectIcon_StuntRider;
        case 12: return g_pTexCarSelectIcon_SuperDiesel;
        case 13: return g_pTexCarSelectIcon_SuperJeep;
        case 14: return g_pTexCarSelectIcon_Trophy;
        case 15: return g_pTexCarSelectIcon_Chevelle;
        // ✅ NEW CARS
        case 16: return g_pTexCarSelectIcon_Blazer;
        case 17: return g_pTexCarSelectIcon_Caprice;
        case 18: return g_pTexCarSelectIcon_Cybertruck;
        case 19: return g_pTexCarSelectIcon_KnightRider;
        case 20: return g_pTexCarSelectIcon_SchoolBus;
        case 21: return g_pTexCarSelectIcon_UsedCar;
        case 22: return g_pTexCarSelectIcon_Boat;
        case 23: return g_pTexCarSelectIcon_Airplane;
        default: {
            // ✅ Custom car slots (22+)
            int customIdx = index - CAR_CUSTOM_BASE;
            if (customIdx >= 0 && customIdx < g_numCustomCars
                && g_customCars[customIdx].loaded) {
                return g_customCars[customIdx].pTexIcon;
            }
            return NULL;
        }
    }
}

//============================================================================================
// ✅ SET CAR PARAMETERS (NOW SUPPORTS ALL 16 CARS!)
//============================================================================================

void SetCarParameters(CarType car) {
    if (car == CAR_JEEP) {
        g_visualCarWidth = JEEP_CAR_WIDTH;
        g_visualCarHeight = JEEP_CAR_HEIGHT;
        g_visualWheelRadius = JEEP_WHEEL_RADIUS;
        g_wheelJointFrequency = JEEP_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = JEEP_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = JEEP_MOTOR_MAX_SPEED;
        g_motorTorqueRear = JEEP_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = JEEP_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = JEEP_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = JEEP_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = JEEP_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = JEEP_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = JEEP_WHEEL_FRONT_START_Y;
        LoadCarCfg("jeep");  // override with cfg if present
    }
    else if (car == CAR_RALLY) {
        g_visualCarWidth = RALLY_CAR_WIDTH;
        g_visualCarHeight = RALLY_CAR_HEIGHT;
        g_visualWheelRadius = RALLY_WHEEL_RADIUS;
        g_wheelJointFrequency = RALLY_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = RALLY_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = RALLY_MOTOR_MAX_SPEED;
        g_motorTorqueRear = RALLY_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = RALLY_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = RALLY_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = RALLY_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = RALLY_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = RALLY_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = RALLY_WHEEL_FRONT_START_Y;
        LoadCarCfg("rally");  // override with cfg if present
    }
    else if (car == CAR_MONSTER) {
        g_visualCarWidth = MONSTER_CAR_WIDTH;
        g_visualCarHeight = MONSTER_CAR_HEIGHT;
        g_visualWheelRadius = MONSTER_WHEEL_RADIUS;
        g_wheelJointFrequency = MONSTER_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = MONSTER_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = MONSTER_MOTOR_MAX_SPEED;
        g_motorTorqueRear = MONSTER_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = MONSTER_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = MONSTER_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = MONSTER_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = MONSTER_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = MONSTER_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = MONSTER_WHEEL_FRONT_START_Y;
        LoadCarCfg("monster");  // override with cfg if present
    }
    else if (car == CAR_QUAD_BIKE) {
        g_visualCarWidth = QUAD_BIKE_CAR_WIDTH;
        g_visualCarHeight = QUAD_BIKE_CAR_HEIGHT;
        g_visualWheelRadius = QUAD_BIKE_WHEEL_RADIUS;
        g_wheelJointFrequency = QUAD_BIKE_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = QUAD_BIKE_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = QUAD_BIKE_MOTOR_MAX_SPEED;
        g_motorTorqueRear = QUAD_BIKE_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = QUAD_BIKE_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = QUAD_BIKE_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = QUAD_BIKE_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = QUAD_BIKE_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = QUAD_BIKE_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = QUAD_BIKE_WHEEL_FRONT_START_Y;
        LoadCarCfg("quad_bike");  // override with cfg if present
    }
    else if (car == CAR_RACE_CAR) {
        g_visualCarWidth = RACE_CAR_WIDTH;
        g_visualCarHeight = RACE_CAR_HEIGHT;
        g_visualWheelRadius = RACE_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = RACE_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = RACE_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = RACE_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = RACE_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = RACE_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = RACE_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = RACE_CAR_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = RACE_CAR_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = RACE_CAR_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = RACE_CAR_WHEEL_FRONT_START_Y;
        LoadCarCfg("race_car");  // override with cfg if present
    }
    else if (car == CAR_FAMILY) {
        g_visualCarWidth = FAMILY_CAR_WIDTH;
        g_visualCarHeight = FAMILY_CAR_HEIGHT;
        g_visualWheelRadius = FAMILY_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = FAMILY_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = FAMILY_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = FAMILY_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = FAMILY_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = FAMILY_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = FAMILY_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = FAMILY_CAR_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = FAMILY_CAR_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = FAMILY_CAR_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = FAMILY_CAR_WHEEL_FRONT_START_Y;
        LoadCarCfg("family");  // override with cfg if present
    }
    else if (car == CAR_OLD_SCHOOL) {
        g_visualCarWidth = OLD_SCHOOL_CAR_WIDTH;
        g_visualCarHeight = OLD_SCHOOL_CAR_HEIGHT;
        g_visualWheelRadius = OLD_SCHOOL_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = OLD_SCHOOL_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = OLD_SCHOOL_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = OLD_SCHOOL_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = OLD_SCHOOL_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = OLD_SCHOOL_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = OLD_SCHOOL_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = OLD_SCHOOL_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = OLD_SCHOOL_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = OLD_SCHOOL_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = OLD_SCHOOL_WHEEL_FRONT_START_Y;
        LoadCarCfg("old_school");  // override with cfg if present
    }
    else if (car == CAR_PARAMEDIC) {
        g_visualCarWidth = PARAMEDIC_CAR_WIDTH;
        g_visualCarHeight = PARAMEDIC_CAR_HEIGHT;
        g_visualWheelRadius = PARAMEDIC_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = PARAMEDIC_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = PARAMEDIC_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = PARAMEDIC_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = PARAMEDIC_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = PARAMEDIC_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = PARAMEDIC_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = PARAMEDIC_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = PARAMEDIC_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = PARAMEDIC_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = PARAMEDIC_WHEEL_FRONT_START_Y;
        LoadCarCfg("paramedic");  // override with cfg if present
    }
    else if (car == CAR_BUGGY) {
        g_visualCarWidth = BUGGY_CAR_WIDTH;
        g_visualCarHeight = BUGGY_CAR_HEIGHT;
        g_visualWheelRadius = BUGGY_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = BUGGY_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = BUGGY_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = BUGGY_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = BUGGY_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = BUGGY_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = BUGGY_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = BUGGY_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = BUGGY_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = BUGGY_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = BUGGY_WHEEL_FRONT_START_Y;
        LoadCarCfg("buggy");  // override with cfg if present
    }
    else if (car == CAR_SPRINTER) {
        g_visualCarWidth = SPRINTER_CAR_WIDTH;
        g_visualCarHeight = SPRINTER_CAR_HEIGHT;
        g_visualWheelRadius = SPRINTER_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = SPRINTER_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = SPRINTER_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = SPRINTER_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = SPRINTER_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = SPRINTER_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = SPRINTER_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = SPRINTER_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = SPRINTER_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = SPRINTER_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = SPRINTER_WHEEL_FRONT_START_Y;
        LoadCarCfg("sprinter");  // override with cfg if present
    }
    else if (car == CAR_POLICE) {
        g_visualCarWidth = POLICE_CAR_WIDTH;
        g_visualCarHeight = POLICE_CAR_HEIGHT;
        g_visualWheelRadius = POLICE_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = POLICE_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = POLICE_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = POLICE_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = POLICE_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = POLICE_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = POLICE_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = POLICE_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = POLICE_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = POLICE_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = POLICE_WHEEL_FRONT_START_Y;
        LoadCarCfg("police");  // override with cfg if present
    }
    else if (car == CAR_STUNT_RIDER) {
        g_visualCarWidth = STUNT_RIDER_CAR_WIDTH;
        g_visualCarHeight = STUNT_RIDER_CAR_HEIGHT;
        g_visualWheelRadius = STUNT_RIDER_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = STUNT_RIDER_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = STUNT_RIDER_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = STUNT_RIDER_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = STUNT_RIDER_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = STUNT_RIDER_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = STUNT_RIDER_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = STUNT_RIDER_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = STUNT_RIDER_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = STUNT_RIDER_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = STUNT_RIDER_WHEEL_FRONT_START_Y;
        LoadCarCfg("stunt_rider");  // override with cfg if present
    }
    else if (car == CAR_SUPER_DIESEL) {
        g_visualCarWidth = SUPER_DIESEL_CAR_WIDTH;
        g_visualCarHeight = SUPER_DIESEL_CAR_HEIGHT;
        g_visualWheelRadius = SUPER_DIESEL_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = SUPER_DIESEL_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = SUPER_DIESEL_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = SUPER_DIESEL_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = SUPER_DIESEL_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = SUPER_DIESEL_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = SUPER_DIESEL_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = SUPER_DIESEL_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = SUPER_DIESEL_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = SUPER_DIESEL_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = SUPER_DIESEL_WHEEL_FRONT_START_Y;
        LoadCarCfg("super_diesel");  // override with cfg if present
    }
    else if (car == CAR_SUPER_JEEP) {
        g_visualCarWidth = SUPER_JEEP_CAR_WIDTH;
        g_visualCarHeight = SUPER_JEEP_CAR_HEIGHT;
        g_visualWheelRadius = SUPER_JEEP_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = SUPER_JEEP_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = SUPER_JEEP_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = SUPER_JEEP_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = SUPER_JEEP_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = SUPER_JEEP_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = SUPER_JEEP_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = SUPER_JEEP_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = SUPER_JEEP_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = SUPER_JEEP_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = SUPER_JEEP_WHEEL_FRONT_START_Y;
        LoadCarCfg("super_jeep");  // override with cfg if present
    }
    else if (car == CAR_TROPHY_TRUCK) {
        g_visualCarWidth = TROPHY_TRUCK_CAR_WIDTH;
        g_visualCarHeight = TROPHY_TRUCK_CAR_HEIGHT;
        g_visualWheelRadius = TROPHY_TRUCK_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = TROPHY_TRUCK_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = TROPHY_TRUCK_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = TROPHY_TRUCK_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = TROPHY_TRUCK_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = TROPHY_TRUCK_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = TROPHY_TRUCK_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = TROPHY_TRUCK_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = TROPHY_TRUCK_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = TROPHY_TRUCK_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = TROPHY_TRUCK_WHEEL_FRONT_START_Y;
        LoadCarCfg("trophy_truck");  // override with cfg if present
    }
    else if (car == CAR_CHEVELLE) { // NEW CAR
        g_visualCarWidth = CHEVELLE_CAR_WIDTH;
        g_visualCarHeight = CHEVELLE_CAR_HEIGHT;
        g_visualWheelRadius = CHEVELLE_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = CHEVELLE_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = CHEVELLE_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = CHEVELLE_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = CHEVELLE_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = CHEVELLE_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = CHEVELLE_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = CHEVELLE_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = CHEVELLE_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = CHEVELLE_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = CHEVELLE_WHEEL_FRONT_START_Y;
        LoadCarCfg("chevelle");  // override with cfg if present
    }
    // ✅ NEW CARS - EXTENDED COLLECTION
    else if (car == CAR_BLAZER) {
        g_visualCarWidth = BLAZER_CAR_WIDTH;
        g_visualCarHeight = BLAZER_CAR_HEIGHT;
        g_visualWheelRadius = BLAZER_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = BLAZER_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = BLAZER_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = BLAZER_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = BLAZER_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = BLAZER_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = BLAZER_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = BLAZER_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = BLAZER_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = BLAZER_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = BLAZER_WHEEL_FRONT_START_Y;
        LoadCarCfg("blazer");  // override with cfg if present
    }
    else if (car == CAR_CAPRICE) {
        g_visualCarWidth = CAPRICE_CAR_WIDTH;
        g_visualCarHeight = CAPRICE_CAR_HEIGHT;
        g_visualWheelRadius = CAPRICE_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = CAPRICE_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = CAPRICE_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = CAPRICE_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = CAPRICE_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = CAPRICE_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = CAPRICE_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = CAPRICE_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = CAPRICE_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = CAPRICE_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = CAPRICE_WHEEL_FRONT_START_Y;
        LoadCarCfg("caprice");  // override with cfg if present
    }
    else if (car == CAR_CYBERTRUCK) {
        g_visualCarWidth = CYBERTRUCK_CAR_WIDTH;
        g_visualCarHeight = CYBERTRUCK_CAR_HEIGHT;
        g_visualWheelRadius = CYBERTRUCK_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = CYBERTRUCK_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = CYBERTRUCK_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = CYBERTRUCK_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = CYBERTRUCK_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = CYBERTRUCK_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = CYBERTRUCK_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = CYBERTRUCK_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = CYBERTRUCK_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = CYBERTRUCK_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = CYBERTRUCK_WHEEL_FRONT_START_Y;
        LoadCarCfg("cybertruck");  // override with cfg if present
    }
    else if (car == CAR_KNIGHTRIDER) {
        g_visualCarWidth = KNIGHTRIDER_CAR_WIDTH;
        g_visualCarHeight = KNIGHTRIDER_CAR_HEIGHT;
        g_visualWheelRadius = KNIGHTRIDER_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = KNIGHTRIDER_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = KNIGHTRIDER_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = KNIGHTRIDER_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = KNIGHTRIDER_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = KNIGHTRIDER_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = KNIGHTRIDER_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = KNIGHTRIDER_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = KNIGHTRIDER_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = KNIGHTRIDER_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = KNIGHTRIDER_WHEEL_FRONT_START_Y;
        LoadCarCfg("knightrider");  // override with cfg if present
    }
    else if (car == CAR_SCHOOLBUS) {
        g_visualCarWidth = SCHOOLBUS_CAR_WIDTH;
        g_visualCarHeight = SCHOOLBUS_CAR_HEIGHT;
        g_visualWheelRadius = SCHOOLBUS_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = SCHOOLBUS_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = SCHOOLBUS_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = SCHOOLBUS_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = SCHOOLBUS_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = SCHOOLBUS_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = SCHOOLBUS_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = SCHOOLBUS_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = SCHOOLBUS_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = SCHOOLBUS_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = SCHOOLBUS_WHEEL_FRONT_START_Y;
        LoadCarCfg("schoolbus");  // override with cfg if present
    }
    else if (car == CAR_USEDCAR) {
        g_visualCarWidth = USEDCAR_CAR_WIDTH;
        g_visualCarHeight = USEDCAR_CAR_HEIGHT;
        g_visualWheelRadius = USEDCAR_CAR_WHEEL_RADIUS;
        g_wheelJointFrequency = USEDCAR_CAR_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping = USEDCAR_CAR_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed = USEDCAR_CAR_MOTOR_MAX_SPEED;
        g_motorTorqueRear = USEDCAR_CAR_MOTOR_TORQUE_REAR;
        g_motorTorqueFront = USEDCAR_CAR_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset = USEDCAR_CAR_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX = USEDCAR_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY = USEDCAR_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = USEDCAR_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY = USEDCAR_WHEEL_FRONT_START_Y;
        LoadCarCfg("used_car");  // override with cfg if present
    }
    else if (car == CAR_BOAT) {
        g_visualCarWidth    = BOAT_CAR_WIDTH;
        g_visualCarHeight   = BOAT_CAR_HEIGHT;
        g_visualWheelRadius = BOAT_WHEEL_RADIUS;
        g_wheelJointFrequency = BOAT_WHEEL_JOINT_FREQUENCY;
        g_wheelJointDamping   = BOAT_WHEEL_JOINT_DAMPING;
        g_motorMaxSpeed     = BOAT_MOTOR_MAX_SPEED;
        g_motorTorqueRear   = BOAT_MOTOR_TORQUE_REAR;
        g_motorTorqueFront  = BOAT_MOTOR_TORQUE_FRONT;
        g_rideHeightOffset  = BOAT_RIDE_HEIGHT_OFFSET;
        g_wheelRearOffsetX  = BOAT_WHEEL_REAR_OFFSET_X;
        g_wheelRearStartY   = BOAT_WHEEL_REAR_START_Y;
        g_wheelFrontOffsetX = BOAT_WHEEL_FRONT_OFFSET_X;
        g_wheelFrontStartY  = BOAT_WHEEL_FRONT_START_Y;
        LoadCarCfg("boat");
    }
    else if (car == CAR_AIRPLANE) {
        g_visualCarWidth    = AIRPLANE_WIDTH;
        g_visualCarHeight   = AIRPLANE_HEIGHT;
        g_visualWheelRadius = 0.28f;          // small landing gear wheels
        g_wheelJointFrequency = 4.0f;
        g_wheelJointDamping   = 0.8f;
        g_motorMaxSpeed     = AIRPLANE_MOTOR_MAX_SPEED;
        g_motorTorqueRear   = 0.0f;           // thrust via ApplyForce, not motor
        g_motorTorqueFront  = 0.0f;
        g_rideHeightOffset  = 0.0f;
        g_wheelRearOffsetX  = -1.8f;
        g_wheelRearStartY   = CAR_START_Y - 0.9f;
        g_wheelFrontOffsetX =  1.8f;
        g_wheelFrontStartY  = CAR_START_Y - 0.9f;
        LoadCarCfg("airplane");
    }
    else {
        // ✅ CUSTOM CAR: index into g_customCars array
        int customIdx = (int)car - CAR_CUSTOM_BASE;
        if (customIdx >= 0 && customIdx < g_numCustomCars && g_customCars[customIdx].loaded) {
            const CustomCarDef& cc = g_customCars[customIdx];
            g_visualCarWidth      = cc.carWidth;
            g_visualCarHeight     = cc.carHeight;
            g_visualWheelRadius   = cc.wheelRadius;
            g_wheelJointFrequency = cc.wheelJointFreq;
            g_wheelJointDamping   = cc.wheelJointDamp;
            g_motorMaxSpeed       = cc.motorMaxSpeed;
            g_motorTorqueRear     = cc.motorTorqueRear;
            g_motorTorqueFront    = cc.motorTorqueFront;
            g_rideHeightOffset    = cc.rideHeightOffset;
            g_wheelRearOffsetX    = cc.wheelRearOffsetX;
            g_wheelFrontOffsetX   = cc.wheelFrontOffsetX;
            // Vertical start positions use the same formula as built-in cars
            g_wheelRearStartY     = 1.5f * Y1 + 0.35f;
            g_wheelFrontStartY    = 1.5f * Y1 + 0.4f;
        }
    }
}

