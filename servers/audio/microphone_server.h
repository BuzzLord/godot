/**************************************************************************/
/*  microphone_server.h                                                   */
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

#pragma once

#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/os/thread_safe.h"
#include "core/templates/rid.h"
#include "core/variant/variant.h"

/**
	The microphone server is a singleton object that gives access to the various
	microphones feeds that can be used as access audio sample data.
**/

class MicrophoneFeed;
template <typename T>
class TypedArray;

class MicrophoneServer : public Object {
	GDCLASS(MicrophoneServer, Object);
	_THREAD_SAFE_CLASS_

public:
	typedef MicrophoneServer *(*CreateFunc)();

private:
protected:
	static CreateFunc create_func;

	Vector<Ref<MicrophoneFeed>> feeds;

	bool input_device_active = false;

	static MicrophoneServer *singleton;

	static void _bind_methods();

	template <typename T>
	static MicrophoneServer *_create_builtin() {
		return memnew(T);
	}

public:
	static MicrophoneServer *get_singleton();

	template <typename T>
	static void make_default() {
		create_func = _create_builtin<T>;
	}

	static MicrophoneServer *create() {
		MicrophoneServer *server = create_func ? create_func() : memnew(MicrophoneServer);
		return server;
	}

	int get_free_id();
	int get_feed_index(int p_id);
	Ref<MicrophoneFeed> get_feed_by_id(int p_id);

	// Add and remove feeds.
	void add_feed(const Ref<MicrophoneFeed> &p_feed);
	void remove_feed(const Ref<MicrophoneFeed> &p_feed);
	void remove_all_feeds();

	PackedStringArray get_device_list();

	Ref<MicrophoneFeed> create_feed(const String p_device_name);

	Error activate_feed(const String p_device_name);
	void deactivate_feed(const String p_device_name);

	// Get our feeds.
	Ref<MicrophoneFeed> get_feed(int p_index);
	int get_feed_count();
	TypedArray<MicrophoneFeed> get_feeds();

	bool is_device_valid(const String p_device_name);

	MicrophoneServer();
	~MicrophoneServer();
};
