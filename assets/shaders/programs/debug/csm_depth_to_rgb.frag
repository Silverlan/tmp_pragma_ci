#version 440

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "/functions/fs_linearize_depth.glsl"

layout(location = 0) in vec2 vs_vert_uv;

layout(LAYOUT_ID(TEXTURE, DEPTH_MAP)) uniform sampler2DArray u_depthTexture;

layout(LAYOUT_PUSH_CONSTANTS()) uniform Depth
{
	float near;
	float far;
	float contrastFactor;
	int layer;
}
u_depth;

layout(location = 0) out vec4 fs_color;

void main()
{
	float d = texture(u_depthTexture, vec3(vs_vert_uv.xy, u_depth.layer)).x; //get_linearized_depth(u_depthTexture,u_depth.layer,vs_vert_uv,u_depth.near,u_depth.far);
	d = pow(d, u_depth.contrastFactor);
	fs_color = vec4(d, d, d, 1.0);
}