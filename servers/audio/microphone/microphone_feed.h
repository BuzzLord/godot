/**************************************************************************/
/*  microphone_feed.h                                                     */
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

#include "servers/audio/microphone_server.h"

class MicrophoneFeed : public RefCounted {
	GDCLASS(MicrophoneFeed, RefCounted);

public:
private:
	int id; // unique id for this, for internal use in case feeds are removed

protected:
	String name; // name of our microphone feed

	bool active; // only when active do we allow getting frames from buffer

	String device_name; // Name of the audio input device this feed uses

	unsigned int buffer_ofs; // offset into the input buffer.

	static void _bind_methods();

public:
	int get_id() const;
	bool is_active() const;
	void set_active(bool p_is_active);

	String get_name() const;
	void set_name(String p_name);

	String get_device_name() const;
	void set_device_name(String p_name);

	int get_mix_rate() const;

	int get_frames_available();
	PackedVector2Array get_buffer(int p_frames);

	MicrophoneFeed();
	MicrophoneFeed(String p_device_name);
	virtual ~MicrophoneFeed();

	virtual bool activate_feed();
	virtual void deactivate_feed();

	// GDVIRTUAL0R(bool, _activate_feed)
	// GDVIRTUAL0(_deactivate_feed)
};
