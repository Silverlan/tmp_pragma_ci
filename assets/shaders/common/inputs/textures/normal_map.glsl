#ifndef F_SH_NORMALMAPPING_GLS
#define F_SH_NORMALMAPPING_GLS

#ifndef USE_NORMAL_MAP
#define USE_NORMAL_MAP 1
#endif

#if USE_NORMAL_MAP == 1
#include "/common/vertex_outputs/tangentspace.glsl"
#include "/common/material_flags.glsl"
#endif

#ifdef GLS_FRAGMENT_SHADER
#include "/lighting/inputs/fs_light_sources.glsl"
#if ENABLE_TANGENT_SPACE_NORMALS == 1
vec3 calc_tangent_space_normal(vec2 uv)
{
	vec3 ncol = texture(u_normalMap, uv).rgb;
	vec3 nmap;
	float normalMapScale = 1.0;
	nmap.xy = (2.0 * ncol.rg - 1.0) * normalMapScale;
	nmap.z = sqrt(1.0 - dot(nmap.xy, nmap.xy));

	return normalize((fs_in.vert_tangent * nmap.x) + (fs_in.vert_bitangent * nmap.y) + (fs_in.vert_normal * nmap.z));
}
#endif
float calculate_normal_light_direction_angle(uint lightIdx, vec2 uv, uint materialFlags)
{
	float cosTheta;
	LightSourceData light = get_light_source(lightIdx);
#if USE_NORMAL_MAP == 1
	if(use_normal_map(materialFlags) == true) {
		vec3 ntex_ts = normalize(texture(u_normalMap, uv).rgb * 2.0 - 1.0);
		vec3 n = ntex_ts;
		vec3 l = normalize(get_light_direction_ts(lightIdx));
		cosTheta = clamp(dot(n, l), 0.0, 1.0);
	}
	else {
		vec3 n = get_vertex_normal_cs();
		vec3 l = normalize(get_light_direction_cs(lightIdx));
		cosTheta = clamp(dot(n, l), 0.0, 1.0);
	}
#endif
	return cosTheta;
}
vec3 get_normal_from_map(vec2 texCoords, uint materialFlags)
{
#if USE_NORMAL_MAP == 1
	if(use_normal_map(materialFlags)) {
		vec3 tangentNormal = texture(u_normalMap, texCoords).xyz * 2.0 - 1.0;

		vec3 Q1 = dFdx(fs_in.vert_pos_ws.xyz);
		vec3 Q2 = dFdy(fs_in.vert_pos_ws.xyz);
		vec2 st1 = dFdx(texCoords);
		vec2 st2 = dFdy(texCoords);

		vec3 N = normalize(fs_in.vert_normal);
		vec3 T = Q1 * st2.t - Q2 * st1.t;
		T = normalize((get_model_matrix() * vec4(T, 0)).xyz);
		vec3 B = -normalize(cross(N, T));
		mat3 TBN = mat3(T, B, N); // TODO: use fs_in.TBN;

		return normalize(TBN * tangentNormal);
	}
#endif
	return fs_in.vert_normal;
}
#endif

#endif
