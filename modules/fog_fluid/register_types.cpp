#include "register_types.h"

#include "fog_fluid_sim.h"

#include "core/object/class_db.h"

void initialize_fog_fluid_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(FogFluidSim);
	}
}

void uninitialize_fog_fluid_module(ModuleInitializationLevel p_level) {}
