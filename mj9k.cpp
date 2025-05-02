#include "mj9k.hpp"

#include <libusb.h>

using namespace godot;

mj9k * mj9k_singleton = nullptr;

mj9k_in_report  status_in = {};
mj9k_out_report status_out = {.bLength = sizeof(mj9k_out_report)};

mj9k::Error error;

int usb_ret;
bool libusb_initialized = false;

libusb_device_handle * controller = NULL;

bool mj9k::is_connected() {
	if (!libusb_initialized) return false;

	if (controller) {
		// This is the only non-blocking way to detect if the USB device is still connected
		usb_ret = libusb_claim_interface(controller, 0);
		if (usb_ret == LIBUSB_SUCCESS) {
			error = ERROR_OKAY;
			return true;
		}

		// Close the old device, before attempting to open a new one
		libusb_close(controller);
		controller = NULL;
	}

	libusb_device * dev = NULL, ** dev_list;
	ssize_t dev_count = libusb_get_device_list(NULL, &dev_list);
	for (int i = 0; i < dev_count; i++) {
		struct libusb_device_descriptor dev_desc;
	        usb_ret = libusb_get_device_descriptor(dev_list[i], &dev_desc);
		if (usb_ret != LIBUSB_SUCCESS) {
			libusb_free_device_list(dev_list, 1);
			error = ERROR_LIBUSB;
			return false;
		}

		if (dev_desc.idVendor == MJ9K_VID && dev_desc.idProduct == MJ9K_PID) {
			dev = dev_list[i];
			break;
		}
	}

	if (!dev) {
		libusb_free_device_list(dev_list, 1);
		error = ERROR_NO_DEVICE;
		return false;
	}

	usb_ret = libusb_open(dev, &controller);

	libusb_free_device_list(dev_list, 1);

	if (usb_ret != LIBUSB_SUCCESS) {
		controller = NULL;

		error = ERROR_LIBUSB;
		return false;
	}

	libusb_set_auto_detach_kernel_driver(controller, true);

	usb_ret = libusb_claim_interface(controller, 0);
	if (usb_ret != LIBUSB_SUCCESS) {
		libusb_close(controller);
		controller = NULL;

		error = ERROR_LIBUSB;
		return false;
	}

	error = ERROR_OKAY;
	return true;
}

bool mj9k::send_lights() {
	if (!libusb_initialized) return false;

	if (!controller) {
		error = ERROR_NO_DEVICE;
		return false;
	}

	int transfered;
	usb_ret = libusb_interrupt_transfer(controller, MJ9K_OUT,
		(unsigned char *)&status_out, sizeof(mj9k_out_report), &transfered, MJ9K_TIMEOUT);

	if (!usb_ret) {
		error = ERROR_LIBUSB;
		return false;
	}

	if (transfered != sizeof(mj9k_out_report)) {
		error = ERROR_BAD_TX;
		return false;
	}

	error = ERROR_OKAY;
	return true;
}

bool mj9k::recieve_input() {
        if (!libusb_initialized) return false;

        if (!controller) {
            error = ERROR_NO_DEVICE;
            return false;
        }

	int transfered;
	usb_ret = libusb_interrupt_transfer(controller, MJ9K_IN,
		(unsigned char *)&status_in, sizeof(mj9k_in_report), &transfered, MJ9K_TIMEOUT);

	if (!usb_ret) {
		error = ERROR_LIBUSB;
		return false;
	}

        if (transfered != sizeof(mj9k_in_report)) {
		error = ERROR_BAD_RX;
		return false;
	}

	error = ERROR_OKAY;
	return true;
}

void mj9k::set_light(mj9k::Light p_light, int32_t p_brightness) {
        unsigned int light_idx = p_light;
        if (light_idx >= MJ9K_LIGHT_COUNT) return;

        uint8_t * light = status_out.lights + light_idx / 2;
	uint8_t value = p_brightness & 0xF;
	
	if (light_idx % 2) {
	    *light = (value << 4) | (*light & 0xF);
	} else {
	    *light = value | (*light & 0xF0);
	}
}

bool mj9k::get_button(mj9k::Button p_button) const {
        unsigned int button_idx = p_button;
        if (button_idx >= MJ9K_BUTTON_COUNT) return false;

	unsigned int button_offset = button_idx / 16;
	uint16_t button_mask = 1 << (button_idx % 16);

	return status_in.buttons[button_offset] & button_mask;
}

Vector2 mj9k::get_aiming() const {
        return Vector2(
                (double)status_in.aimingX / (double)0xFFFF,
                (double)status_in.aimingY / (double)0xFFFF
        );
}

double mj9k::get_turning() const {
	return (double)status_in.turningLever / (double)0x7FFF;
}

Vector2 mj9k::get_sight() const {
	return Vector2(
		(double)status_in.sightChangeX / (double)0x7FFF,
		(double)status_in.sightChangeY / (double)0x7FFF
	);
}

double mj9k::get_pedal(mj9k::Pedal p_pedal) const {
	double value = 0.0;
	switch (p_pedal) {
	case PEDAL_ACCEL: value = status_in.accelPedal; break;
	case PEDAL_BRAKE: value = status_in.brakePedal; break;
	case PEDAL_SLIDE: value = status_in.slidePedal;	break;
	}

	return value / (double)0xFFFF;
}

int mj9k::get_tuner() const {
	return status_in.tuner;
}

int mj9k::get_shifter() const {
	return status_in.shifter;
}

String mj9k::get_error() const {
	switch (error) {
	case ERROR_NO_DEVICE:
		return "A MJ9K is not connected";
	case ERROR_BAD_RX:
		return "Failed to recieve all bytes from MJ9K";
	case ERROR_BAD_TX:
		return "Failed to send all bytes to MJ9K";
	case ERROR_LIBUSB:
		return libusb_strerror(usb_ret);
	}

	return "";
}

String mj9k::get_libusb_error() const {
	if (error != ERROR_LIBUSB) return "";
	return libusb_error_name(usb_ret);
}

void mj9k::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_connected"), &mj9k::is_connected);
	ClassDB::bind_method(D_METHOD("send_lights"), &mj9k::send_lights);
	ClassDB::bind_method(D_METHOD("recieve_input"), &mj9k::recieve_input);

	ClassDB::bind_method(D_METHOD("get_error"), &mj9k::get_error);
	ClassDB::bind_method(D_METHOD("get_libusb_error"), &mj9k::get_libusb_error);

	ClassDB::bind_method(D_METHOD("set_light", "light", "brightness"), &mj9k::set_light);

	ClassDB::bind_method(D_METHOD("get_button", "button"), &mj9k::get_button);
	ClassDB::bind_method(D_METHOD("get_aiming"), &mj9k::get_aiming);
	ClassDB::bind_method(D_METHOD("get_turning"), &mj9k::get_turning);
	ClassDB::bind_method(D_METHOD("get_sight"), &mj9k::get_sight);
	ClassDB::bind_method(D_METHOD("get_pedal", "pedal"), &mj9k::get_pedal);
	ClassDB::bind_method(D_METHOD("get_tuner"), &mj9k::get_tuner);
	ClassDB::bind_method(D_METHOD("get_shifter"), &mj9k::get_shifter);
	
	BIND_ENUM_CONSTANT(ERROR_OKAY);
	BIND_ENUM_CONSTANT(ERROR_NO_DEVICE);
	BIND_ENUM_CONSTANT(ERROR_BAD_RX);
	BIND_ENUM_CONSTANT(ERROR_BAD_TX);
	BIND_ENUM_CONSTANT(ERROR_LIBUSB);
	
	BIND_ENUM_CONSTANT(PEDAL_ACCEL);
	BIND_ENUM_CONSTANT(PEDAL_BRAKE);
	BIND_ENUM_CONSTANT(PEDAL_SLIDE);
	
	BIND_ENUM_CONSTANT(BUTTON_FIRE_MAIN);
	BIND_ENUM_CONSTANT(BUTTON_FIRE_SUB);
        BIND_ENUM_CONSTANT(BUTTON_LOCK_ON);
        BIND_ENUM_CONSTANT(BUTTON_EJECT);
        BIND_ENUM_CONSTANT(BUTTON_HATCH);
        BIND_ENUM_CONSTANT(BUTTON_IGNITION);
        BIND_ENUM_CONSTANT(BUTTON_START);
	BIND_ENUM_CONSTANT(BUTTON_MULTI_TOGGLE);
	BIND_ENUM_CONSTANT(BUTTON_MULTI_ZOOM);
	BIND_ENUM_CONSTANT(BUTTON_MULTI_MODE);
	BIND_ENUM_CONSTANT(BUTTON_SUB_MODE);
	BIND_ENUM_CONSTANT(BUTTON_ZOOM_IN);
	BIND_ENUM_CONSTANT(BUTTON_ZOOM_OUT);
	BIND_ENUM_CONSTANT(BUTTON_FSS);
	BIND_ENUM_CONSTANT(BUTTON_MANIPULATOR);
	BIND_ENUM_CONSTANT(BUTTON_LINE_COLOR_CHANGE);
	BIND_ENUM_CONSTANT(BUTTON_WASHING);
	BIND_ENUM_CONSTANT(BUTTON_EXTINGUISHER);
	BIND_ENUM_CONSTANT(BUTTON_CHAFF);
	BIND_ENUM_CONSTANT(BUTTON_TANK_DETACH);
	BIND_ENUM_CONSTANT(BUTTON_OVERRIDE);
	BIND_ENUM_CONSTANT(BUTTON_NIGHT_SCOPE);
	BIND_ENUM_CONSTANT(BUTTON_F1);
	BIND_ENUM_CONSTANT(BUTTON_F2);
	BIND_ENUM_CONSTANT(BUTTON_F3);
	BIND_ENUM_CONSTANT(BUTTON_CYCLE_MAIN);
	BIND_ENUM_CONSTANT(BUTTON_CYCLE_SUB);
	BIND_ENUM_CONSTANT(BUTTON_MAG_CHANGE);
	BIND_ENUM_CONSTANT(BUTTON_COMM_1);
	BIND_ENUM_CONSTANT(BUTTON_COMM_2);
	BIND_ENUM_CONSTANT(BUTTON_COMM_3);
	BIND_ENUM_CONSTANT(BUTTON_COMM_4);
	BIND_ENUM_CONSTANT(BUTTON_COMM_5);
	BIND_ENUM_CONSTANT(BUTTON_SIGHT_CHANGE);
	// Toggle Switches
	BIND_ENUM_CONSTANT(SWITCH_FILTER);
	BIND_ENUM_CONSTANT(SWITCH_OXYGEN);
	BIND_ENUM_CONSTANT(SWITCH_FUEL);
	BIND_ENUM_CONSTANT(SWITCH_BUFFER);
	BIND_ENUM_CONSTANT(SWITCH_LOCATION);
	
	BIND_ENUM_CONSTANT(LIGHT_EJECT);
	BIND_ENUM_CONSTANT(LIGHT_HATCH);
	BIND_ENUM_CONSTANT(LIGHT_IGNITION);
	BIND_ENUM_CONSTANT(LIGHT_START);
	BIND_ENUM_CONSTANT(LIGHT_MULTI_TOGGLE);
	BIND_ENUM_CONSTANT(LIGHT_MULTI_ZOOM);
	BIND_ENUM_CONSTANT(LIGHT_MULTI_MODE);
	BIND_ENUM_CONSTANT(LIGHT_SUB_MODE);
	BIND_ENUM_CONSTANT(LIGHT_ZOOM_IN);
	BIND_ENUM_CONSTANT(LIGHT_ZOOM_OUT);
	BIND_ENUM_CONSTANT(LIGHT_FSS);
	BIND_ENUM_CONSTANT(LIGHT_MANIPULATOR);
	BIND_ENUM_CONSTANT(LIGHT_LINE_COLOR_CHANGE);
	BIND_ENUM_CONSTANT(LIGHT_WASHING);
	BIND_ENUM_CONSTANT(LIGHT_EXTINGUISHER);
	BIND_ENUM_CONSTANT(LIGHT_CHAFF);
	BIND_ENUM_CONSTANT(LIGHT_OVERRIDE);
	BIND_ENUM_CONSTANT(LIGHT_TANK_DETACH);
	BIND_ENUM_CONSTANT(LIGHT_NIGHT_SCOPE);
	BIND_ENUM_CONSTANT(LIGHT_F1);
	BIND_ENUM_CONSTANT(LIGHT_F2);
	BIND_ENUM_CONSTANT(LIGHT_F3);
	BIND_ENUM_CONSTANT(LIGHT_CYCLE_MAIN);
	BIND_ENUM_CONSTANT(LIGHT_CYCLE_SUB);
	BIND_ENUM_CONSTANT(LIGHT_MAG_CHANGE);
	BIND_ENUM_CONSTANT(LIGHT_COMM_1);
	BIND_ENUM_CONSTANT(LIGHT_COMM_2);
	BIND_ENUM_CONSTANT(LIGHT_COMM_3);
	BIND_ENUM_CONSTANT(LIGHT_COMM_4);
	BIND_ENUM_CONSTANT(LIGHT_COMM_5);
	BIND_ENUM_CONSTANT(LIGHT_GEAR_R);
	BIND_ENUM_CONSTANT(LIGHT_GEAR_N);
	BIND_ENUM_CONSTANT(LIGHT_GEAR_1);
	BIND_ENUM_CONSTANT(LIGHT_GEAR_2);
	BIND_ENUM_CONSTANT(LIGHT_GEAR_3);
	BIND_ENUM_CONSTANT(LIGHT_GEAR_4);
	BIND_ENUM_CONSTANT(LIGHT_GEAR_5);
}

void initialize_mj9k_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

	ClassDB::register_class<mj9k>();

	usb_ret = libusb_init(NULL);
	libusb_initialized = usb_ret == LIBUSB_SUCCESS;
	error = libusb_initialized ? mj9k::ERROR_OKAY : mj9k::ERROR_LIBUSB;

	mj9k_singleton = memnew(mj9k);
	Engine::get_singleton()->register_singleton("mj9k", mj9k_singleton);
}

void uninitialize_mj9k_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) return;

	Engine::get_singleton()->unregister_singleton("mj9k");
	memdelete(mj9k_singleton);

        if (controller) {
		libusb_release_interface(controller, 0);
		libusb_close(controller);
	}

	if (libusb_initialized) {
		libusb_exit(NULL);
	}
}

extern "C" {
GDExtensionBool GDE_EXPORT mj9k_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_mj9k_module);
	init_obj.register_terminator(uninitialize_mj9k_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
