/**************************************************************************/
/*  camera_server.cpp                                                     */
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

#include "microphone_server.h"
#include "core/variant/typed_array.h"
#include "servers/audio/microphone/microphone_feed.h"

#include "audio_server.h"

////////////////////////////////////////////////////////
// MicrophoneServer

MicrophoneServer::CreateFunc MicrophoneServer::create_func = nullptr;

void MicrophoneServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_feed", "index"), &MicrophoneServer::get_feed);
	ClassDB::bind_method(D_METHOD("get_feed_count"), &MicrophoneServer::get_feed_count);
	ClassDB::bind_method(D_METHOD("get_feeds"), &MicrophoneServer::get_feeds);

	ClassDB::bind_method(D_METHOD("add_feed", "feed"), &MicrophoneServer::add_feed);
	ClassDB::bind_method(D_METHOD("remove_feed", "feed"), &MicrophoneServer::remove_feed);

	ClassDB::bind_method(D_METHOD("get_device_list"), &MicrophoneServer::get_device_list);
	ClassDB::bind_method(D_METHOD("create_feed", "device_name"), &MicrophoneServer::create_feed);

	// These are not exposed, used internally by MicrophoneFeed to activate the device
	// Error MicrophoneServer::activate_feed(const String p_device_name)
	// void MicrophoneServer::deactivate_feed(const String p_device_name)
}

MicrophoneServer *MicrophoneServer::singleton = nullptr;

MicrophoneServer *MicrophoneServer::get_singleton() {
	return singleton;
}

int MicrophoneServer::get_free_id() {
	bool id_exists = true;
	int newid = 0;

	// find a free id
	while (id_exists) {
		newid++;
		id_exists = false;
		for (int i = 0; i < feeds.size() && !id_exists; i++) {
			if (feeds[i]->get_id() == newid) {
				id_exists = true;
			};
		};
	};

	return newid;
}

int MicrophoneServer::get_feed_index(int p_id) {
	for (int i = 0; i < feeds.size(); i++) {
		if (feeds[i]->get_id() == p_id) {
			return i;
		};
	};

	return -1;
}

Ref<MicrophoneFeed> MicrophoneServer::get_feed_by_id(int p_id) {
	int index = get_feed_index(p_id);

	if (index == -1) {
		return nullptr;
	} else {
		return feeds[index];
	}
}

void MicrophoneServer::add_feed(const Ref<MicrophoneFeed> &p_feed) {
	ERR_FAIL_COND(p_feed.is_null());

	// add our feed
	feeds.push_back(p_feed);

	print_verbose(vformat("MicrophoneServer: Registered microphone feed %s with ID %d at index %d", p_feed->get_name(), p_feed->get_id(), (feeds.size() - 1)));

	// TODO should we emit a signal here, like camera_feed?
	// emit_signal(SNAME("camera_feed_added"), p_feed->get_id());
}

void MicrophoneServer::remove_feed(const Ref<MicrophoneFeed> &p_feed) {
	for (int i = 0; i < feeds.size(); i++) {
		if (feeds[i] == p_feed) {
			int feed_id = p_feed->get_id();

			print_verbose(vformat("MicrophoneServer: Removed camera %s with ID %d", p_feed->get_name(), feed_id));

			// remove it from our array, if this results in our feed being unreferenced it will be destroyed
			feeds.remove_at(i);

			// TODO should we emit a signal here, like camera_feed?
			// emit_signal(SNAME("camera_feed_removed"), feed_id);
			return;
		};
	};
}

void MicrophoneServer::remove_all_feeds() {
	// remove existing devices
	for (int i = feeds.size() - 1; i >= 0; i--) {
		remove_feed(feeds[i]);
	}
}

Ref<MicrophoneFeed> MicrophoneServer::get_feed(int p_index) {
	ERR_FAIL_INDEX_V(p_index, feeds.size(), nullptr);

	return feeds[p_index];
}

int MicrophoneServer::get_feed_count() {
	return feeds.size();
}

TypedArray<MicrophoneFeed> MicrophoneServer::get_feeds() {
	TypedArray<MicrophoneFeed> return_feeds;
	int cc = get_feed_count();
	return_feeds.resize(cc);

	for (int i = 0; i < feeds.size(); i++) {
		return_feeds[i] = get_feed(i);
	};

	return return_feeds;
}

PackedStringArray MicrophoneServer::get_device_list() {
	return AudioDriver::get_singleton()->get_input_device_list();
}

Ref<MicrophoneFeed> MicrophoneServer::create_feed(const String p_device_name) {
	if (!is_device_valid(p_device_name)) {
		print_error(vformat("MicrophoneServer: Unknown input device name during create_feed: %s", p_device_name));
		return nullptr;
	}

	Ref<MicrophoneFeed> feed = memnew(MicrophoneFeed(p_device_name));
	add_feed(feed);

	return feed;
}

Error MicrophoneServer::activate_feed(const String p_device_name) {
	if (!is_device_valid(p_device_name)) {
		print_verbose(vformat("MicrophoneServer: Unexpected input device name in activate_feed: %s", p_device_name));
		return ERR_INVALID_PARAMETER;
	}

	if (input_device_active) {
		String input_device = AudioDriver::get_singleton()->get_input_device();
		if (p_device_name == input_device) {
			// Already active, just return true.
			return OK;
		} else {
			// Is there a better error here? Single input situation, but different audio input device is active.
			print_verbose(vformat("MicrophoneServer: another audio input device already active: %s", input_device));
			// ERR_CANT_OPEN? ERR_LOCKED? ERR_UNAVAILABLE?
			return ERR_ALREADY_IN_USE;
		}
	}

	// Change input device in driver
	AudioDriver::get_singleton()->set_input_device(p_device_name);

	Error err = AudioDriver::get_singleton()->input_start();
	if (err != OK) {
		print_error(vformat("MicrophoneServer: audio driver input_start failed on %s: %s", p_device_name, error_names[err]));
		return err;
	}
	input_device_active = true;
	return OK;
}

void MicrophoneServer::deactivate_feed(const String p_device_name) {
	if (!is_device_valid(p_device_name)) {
		print_verbose(vformat("MicrophoneServer: Unexpected input device name in deactivate_feed: %s", p_device_name));
		return;
	}

	if (!input_device_active) {
		return;
	}

	String input_device = AudioDriver::get_singleton()->get_input_device();
	if (p_device_name == input_device) {
		// A feed has deactivated itself, so check if there are any other active feeds
		int active_feeds = 0;

		for (int i = 0; i < feeds.size(); i++) {
			Ref<MicrophoneFeed> feed = get_feed(i);
			if (feed->is_active() && feed->get_device_name() == input_device) {
				active_feeds++;
			}
		};

		if (active_feeds == 0) {
			// No more active feeds, stop input
			Error err = AudioDriver::get_singleton()->input_stop();
			if (err != OK) {
				print_verbose(vformat("MicrophoneServer: audio driver input_stop failed on %s: %s", p_device_name, error_names[err]));
			}
			// All drivers should be stopped, even if they return an error
			input_device_active = false;
		}
	}
}

bool MicrophoneServer::is_device_valid(const String p_device_name) {
	if (p_device_name == "Default") {
		return true;
	}

	PackedStringArray list = get_device_list();
	return list.has(p_device_name);
}

MicrophoneServer::MicrophoneServer() {
	singleton = this;
}

MicrophoneServer::~MicrophoneServer() {
	singleton = nullptr;
	remove_all_feeds();
}
