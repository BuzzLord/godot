/**************************************************************************/
/*  microphone_feed.cpp                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "microphone_feed.h"

#include "core/config/project_settings.h"
#include "servers/audio/audio_server.h"

void MicrophoneFeed::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &MicrophoneFeed::get_id);

	ClassDB::bind_method(D_METHOD("is_active"), &MicrophoneFeed::is_active);
	ClassDB::bind_method(D_METHOD("set_active", "active"), &MicrophoneFeed::set_active);

	ClassDB::bind_method(D_METHOD("get_name"), &MicrophoneFeed::get_name);
	ClassDB::bind_method(D_METHOD("set_name", "name"), &MicrophoneFeed::set_name);

	ClassDB::bind_method(D_METHOD("get_device_name"), &MicrophoneFeed::get_device_name);

	ClassDB::bind_method(D_METHOD("get_mix_rate"), &MicrophoneFeed::get_mix_rate);

	ClassDB::bind_method(D_METHOD("get_frames_available"), &MicrophoneFeed::get_frames_available);
	ClassDB::bind_method(D_METHOD("get_buffer", "frames"), &MicrophoneFeed::get_buffer);

	ClassDB::bind_method(D_METHOD("activate_feed"), &MicrophoneFeed::activate_feed);
	ClassDB::bind_method(D_METHOD("deactivate_feed"), &MicrophoneFeed::deactivate_feed);

	// GDVIRTUAL_BIND(_activate_feed);
	// GDVIRTUAL_BIND(_deactivate_feed);
}

int MicrophoneFeed::get_id() const {
	return id;
}

bool MicrophoneFeed::is_active() const {
	return active;
}

void MicrophoneFeed::set_active(bool p_is_active) {
	if (p_is_active == active) {
		// all good
	} else if (p_is_active) {
		// attempt to activate this feed
		if (activate_feed()) {
			active = true;
		}
	} else {
		// just deactivate it
		active = false;
		deactivate_feed();
	}
}

String MicrophoneFeed::get_name() const {
	return name;
}

void MicrophoneFeed::set_name(String p_name) {
	name = p_name;
}

String MicrophoneFeed::get_device_name() const {
	return device_name;
}

void MicrophoneFeed::set_device_name(String p_name) {
	device_name = p_name;
}

int MicrophoneFeed::get_mix_rate() const {
	return AudioDriver::get_singleton()->get_input_mix_rate();
}

int MicrophoneFeed::get_frames_available() {
	ERR_FAIL_COND_V_MSG(!active, 0, "Unable to get frames available on inactive microphone feed");
	AudioDriver::get_singleton()->lock();

	unsigned int input_position = AudioDriver::get_singleton()->get_input_position();
	if (input_position < buffer_ofs) {
		Vector<int32_t> buf = AudioDriver::get_singleton()->get_input_buffer();
		input_position += buf.size();
	}

	AudioDriver::get_singleton()->unlock();

	return (input_position - buffer_ofs) / 2;
}

PackedVector2Array MicrophoneFeed::get_buffer(int p_frames) {
	PackedVector2Array ret;
	ERR_FAIL_COND_V_MSG(!active, ret, "Unable to get buffer on inactive microphone feed");

	AudioDriver::get_singleton()->lock();

	unsigned int input_position = AudioDriver::get_singleton()->get_input_position();
	Vector<int32_t> buf = AudioDriver::get_singleton()->get_input_buffer();
	if (input_position < buffer_ofs) {
		input_position += buf.size();
	}
	if ((buffer_ofs + p_frames * 2 <= input_position) && (p_frames >= 0)) {
		ret.resize(p_frames);
		for (int i = 0; i < p_frames; i++) {
			float l = (buf[buffer_ofs++] >> 16) / 32768.f;
			if (buffer_ofs >= buf.size()) {
				buffer_ofs = 0;
			}
			float r = (buf[buffer_ofs++] >> 16) / 32768.f;
			if (buffer_ofs >= buf.size()) {
				buffer_ofs = 0;
			}
			ret.write[i] = Vector2(l, r);
		}
	}

	AudioDriver::get_singleton()->unlock();

	return ret;
}

MicrophoneFeed::MicrophoneFeed() {
	// initialize our feed
	id = MicrophoneServer::get_singleton()->get_free_id();
	active = false;
	device_name = AudioDriver::get_singleton()->get_input_device();
	name = device_name + itos(id);
}

MicrophoneFeed::MicrophoneFeed(String p_device_name) {
	// initialize our feed
	id = MicrophoneServer::get_singleton()->get_free_id();
	active = false;
	device_name = p_device_name;
	name = device_name + itos(id);
}

MicrophoneFeed::~MicrophoneFeed() {
	// TODO: should we deactivate and/or remove ourself from MicrophoneServer?
}

bool MicrophoneFeed::activate_feed() {
	if (!GLOBAL_GET("audio/driver/enable_input")) {
		WARN_PRINT("You must enable the project setting \"audio/driver/enable_input\" to use audio capture.");
		return false;
	}

	buffer_ofs = 0;
	Error ret = MicrophoneServer::get_singleton()->activate_feed(device_name);
	if (ret != OK) {
		return false;
	}
	// GDVIRTUAL_CALL(_activate_feed, ret);
	return true;
}

void MicrophoneFeed::deactivate_feed() {
	MicrophoneServer::get_singleton()->deactivate_feed(device_name);
	// GDVIRTUAL_CALL(_deactivate_feed);
}
