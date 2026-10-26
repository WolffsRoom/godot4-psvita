#pragma once

// Lightweight per-subsystem CPU timing for PS Vita device logs. The main loop
// prints the averages every 300 frames as "[vita-prof]". Compiled out on
// every other platform.
#ifdef VITA_ENABLED
#include "core/os/os.h"

enum VitaProfSlot {
	VITA_PROF_PROCESS, // SceneTree::process (scripts, animation, UI).
	VITA_PROF_PHYSICS_SCRIPTS, // SceneTree::physics_process.
	VITA_PROF_NAVIGATION,
	VITA_PROF_PHYSICS_3D,
	VITA_PROF_PHYSICS_2D,
	VITA_PROF_RENDER, // RenderingServer::draw (cull + GXM submit + present).
	VITA_PROF_ANIMATION, // AnimationMixer::_process_animation.
	VITA_PROF_SKELETON, // Skeleton3D pose updates.
	VITA_PROF_AUDIO3D, // AudioStreamPlayer3D panning and area queries.
	VITA_PROF_MAX
};

extern uint64_t vita_prof_us[VITA_PROF_MAX];

struct VitaProfScope {
	int slot;
	uint64_t start;
	explicit VitaProfScope(int p_slot) :
			slot(p_slot), start(OS::get_singleton()->get_ticks_usec()) {}
	~VitaProfScope() { vita_prof_us[slot] += OS::get_singleton()->get_ticks_usec() - start; }
};

#define VITA_PROF_CONCAT2(a, b) a##b
#define VITA_PROF_CONCAT(a, b) VITA_PROF_CONCAT2(a, b)
#define VITA_PROF(slot) VitaProfScope VITA_PROF_CONCAT(vita_prof_scope_, __LINE__)(slot)
#else
#define VITA_PROF(slot)
#endif
