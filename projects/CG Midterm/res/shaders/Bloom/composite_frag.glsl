#version 420

layout(location = 0) in vec2 inUV;

out vec4 frag_color;

layout (binding = 0) uniform sampler2D s_screenTex;
layout (binding = 1) uniform sampler2D s_Bloom;

void main()
{
	vec4 texCol1 = texture(s_screenTex, inUV);
	vec4 texCol2 = texture(s_Bloom, inUV);

	frag_color = 1.0 - (1.0 - texCol1) * (1.0 - texCol2);
}