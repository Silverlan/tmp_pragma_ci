#ifndef F_SH_PBR_GLS
#define F_SH_PBR_GLS

#define SPECIALIZATION_CONSTANT_ENABLE_LIGHT_MAPS 0
#define SPECIALIZATION_CONSTANT_ENABLE_ANIMATION (SPECIALIZATION_CONSTANT_ENABLE_LIGHT_MAPS + 1)
#define SPECIALIZATION_CONSTANT_ENABLE_MORPH_TARGET_ANIMATION (SPECIALIZATION_CONSTANT_ENABLE_ANIMATION + 1)

#define SPECIALIZATION_CONSTANT_ENABLE_TRANSLUCENCY (SPECIALIZATION_CONSTANT_ENABLE_MORPH_TARGET_ANIMATION + 1)

#define SPECIALIZATION_CONSTANT_SHADOW_QUALITY (SPECIALIZATION_CONSTANT_ENABLE_TRANSLUCENCY + 1)
#define SPECIALIZATION_CONSTANT_DEBUG_MODE_ENABLED (SPECIALIZATION_CONSTANT_SHADOW_QUALITY + 1)
#define SPECIALIZATION_CONSTANT_BLOOM_OUTPUT_ENABLED (SPECIALIZATION_CONSTANT_DEBUG_MODE_ENABLED + 1)
#define SPECIALIZATION_CONSTANT_ENABLE_SSAO (SPECIALIZATION_CONSTANT_BLOOM_OUTPUT_ENABLED + 1)
#define SPECIALIZATION_CONSTANT_ENABLE_IBL (SPECIALIZATION_CONSTANT_ENABLE_SSAO + 1)
#define SPECIALIZATION_CONSTANT_ENABLE_DYNAMIC_LIGHTING (SPECIALIZATION_CONSTANT_ENABLE_IBL + 1)
#define SPECIALIZATION_CONSTANT_ENABLE_DYNAMIC_SHADOWS (SPECIALIZATION_CONSTANT_ENABLE_DYNAMIC_LIGHTING + 1)

layout(constant_id = SPECIALIZATION_CONSTANT_ENABLE_LIGHT_MAPS) const uint CSPEC_ENABLE_LIGHT_MAPS = 1;

#endif