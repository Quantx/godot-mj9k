#ifndef MJ9K_H
#define MJ9K_H

#include <gdextension_interface.h>
#include <godot_cpp/godot.hpp>

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

#include <godot_cpp/classes/engine.hpp>

namespace godot {

#define MJ9K_SHIFTER_OFFSET 7
#define MJ9K_LIGHT_COUNT 40
#define MJ9K_BUTTON_COUNT 48

#define MJ9K_TIMEOUT 10

#define MJ9K_VID 0x0A7B
#define MJ9K_PID 0xD000

#define MJ9K_OUT (LIBUSB_ENDPOINT_OUT | 1)
#define MJ9K_IN  (LIBUSB_ENDPOINT_IN  | 2)

#define XBOX_VID 0x045E
#define XBOX_PID 0x0289

#define XBOX_OUT (LIBUSB_ENDPOINT_OUT | 1)
#define XBOX_IN  (LIBUSB_ENDPOINT_IN  | 1)

typedef struct __attribute__((packed)) {
	uint8_t zero;
	uint8_t bLength;
	uint16_t buttons[MJ9K_BUTTON_COUNT / 16];
	uint16_t aimingX;	//0 to 0xFFFF left to right
	uint16_t aimingY;	//0 to 0xFFFF top to bottom
	int16_t turningLever;
	int16_t sightChangeX;
	int16_t sightChangeY;
	uint16_t slidePedal;	//Sidestep, 0x0000 to 0xFF00
	uint16_t brakePedal;	//Brake, 0x0000 to 0xFF00
	uint16_t accelPedal;	//Acceleration, 0x0000 to oxFF00
	uint8_t tuner;          //0-15 is from 9oclock, around clockwise
	int8_t shifter;         //-2 = R, -1 = N, 0 = Error, 1 = 1st, 2 = 2nd, 3 = 3rnd, 4 = 4th, 5 = 5th
} mj9k_in_report;

typedef struct __attribute__((packed)) {
	uint8_t zero;
	uint8_t bLength;
	uint8_t lights[MJ9K_LIGHT_COUNT / 2];
/*
	uint8_t CockpitHatch_EmergencyEject;
	uint8_t Start_Ignition;
	uint8_t MapZoomInOut_OpenClose;
	uint8_t SubMonitorModeSelect_ModeSelect;
	uint8_t MainMonitorZoomOut_MainMonitorZoomIn;
	uint8_t Manipulator_ForecastShootingSystem;
	uint8_t Washing_LineColorChange;
	uint8_t Chaff_Extinguisher;
	uint8_t Override_TankDetach;
	uint8_t F1_NightScope;
	uint8_t F3_F2;
	uint8_t SubWeaponControl_MainWeaponControl;
	uint8_t Comm1_MagazineChange;
	uint8_t Comm3_Comm2;
	uint8_t Comm5_Comm4;
	uint8_t GearR_;
	uint8_t Gear1_GearN;
	uint8_t Gear3_Gear2;
	uint8_t Gear5_Gear4;
	uint8_t dummy;
*/
} mj9k_out_report;

class mj9k : public Object {
	GDCLASS(mj9k, Object)

public:
	enum Error {
		ERROR_OKAY,
		ERROR_NO_DEVICE,
		ERROR_BAD_RX,
		ERROR_BAD_TX,
		ERROR_LIBUSB_INIT,
		ERROR_LIBUSB
	};

	enum Pedal {
		PEDAL_ACCEL,
		PEDAL_BRAKE,
		PEDAL_SLIDE
	};

	enum Button {
		BUTTON_FIRE_MAIN,
		BUTTON_FIRE_SUB,
		BUTTON_LOCK_ON,
		BUTTON_EJECT,
		BUTTON_HATCH,
		BUTTON_IGNITION,
		BUTTON_START,
		BUTTON_MULTI_TOGGLE,
		BUTTON_MULTI_ZOOM,
		BUTTON_MULTI_MODE,
		BUTTON_SUB_MODE,
		BUTTON_ZOOM_IN,
		BUTTON_ZOOM_OUT,
		BUTTON_FSS,
		BUTTON_MANIPULATOR,
		BUTTON_LINE_COLOR_CHANGE,
		BUTTON_WASHING,
		BUTTON_EXTINGUISHER,
		BUTTON_CHAFF,
		BUTTON_TANK_DETACH,
		BUTTON_OVERRIDE,
		BUTTON_NIGHT_SCOPE,
		BUTTON_F1,
		BUTTON_F2,
		BUTTON_F3,
		BUTTON_CYCLE_MAIN,
		BUTTON_CYCLE_SUB,
		BUTTON_MAG_CHANGE,
		BUTTON_COMM_1,
		BUTTON_COMM_2,
		BUTTON_COMM_3,
		BUTTON_COMM_4,
		BUTTON_COMM_5,
		BUTTON_SIGHT_CHANGE,
		// Toggle Switches
		SWITCH_FILTER,
		SWITCH_OXYGEN,
		SWITCH_FUEL,
		SWITCH_BUFFER,
		SWITCH_LOCATION
	};

	enum Light {
		LIGHT_EJECT,
		LIGHT_HATCH,
		LIGHT_IGNITION,
		LIGHT_START,
		LIGHT_MULTI_TOGGLE,
		LIGHT_MULTI_ZOOM,
		LIGHT_MULTI_MODE,
		LIGHT_SUB_MODE,
		LIGHT_ZOOM_IN,
		LIGHT_ZOOM_OUT,
		LIGHT_FSS,
		LIGHT_MANIPULATOR,
		LIGHT_LINE_COLOR_CHANGE,
		LIGHT_WASHING,
		LIGHT_EXTINGUISHER,
		LIGHT_CHAFF,
		LIGHT_TANK_DETACH,
		LIGHT_OVERRIDE,
		LIGHT_NIGHT_SCOPE,
		LIGHT_F1,
		LIGHT_F2,
		LIGHT_F3,
		LIGHT_CYCLE_MAIN,
		LIGHT_CYCLE_SUB,
		LIGHT_MAG_CHANGE,
		LIGHT_COMM_1,
		LIGHT_COMM_2,
		LIGHT_COMM_3,
		LIGHT_COMM_4,
		LIGHT_COMM_5,
		LIGHT_GEAR_R = LIGHT_COMM_5 + 2,
		LIGHT_GEAR_N,
		LIGHT_GEAR_1,
		LIGHT_GEAR_2,
		LIGHT_GEAR_3,
		LIGHT_GEAR_4,
		LIGHT_GEAR_5
	};

	// USB Handling (returns false on error)
	bool is_connected();
	bool send_lights();
	bool recieve_input();

	// Error handling
	String get_error() const;
	String get_libusb_error() const;

	// Setting lights
	void set_light(mj9k::Light p_light, int32_t p_brightness);

	// Reading input
	bool get_button(mj9k::Button p_button) const;
	Vector2 get_aiming() const;
	double get_turning() const;
	Vector2 get_sight() const;
	double get_pedal(mj9k::Pedal p_pedal) const;
	int get_tuner() const;
	int get_shifter() const;

protected:
	static void _bind_methods();
};

} // namespace godot

VARIANT_ENUM_CAST(mj9k::Error);
VARIANT_ENUM_CAST(mj9k::Pedal);
VARIANT_ENUM_CAST(mj9k::Button);
VARIANT_ENUM_CAST(mj9k::Light);

#endif
