#version 420

layout(location = 0) in vec2 inUV;

out vec4 frag_color;

layout (binding = 0) uniform sampler2D s_screenTex;

uniform float u_Threshold;

void main()
{
	vec4 textureCol = texture(s_screenTex, inUV);

	float luminance = (textureCol.r + textureCol.g + textureCol.b) / 3.0f;

	if (luminance > u_Threshold)
	{
		frag_color = textureCol;
	}
	else
	{
		frag_color = vec4(0.0, 0.0, 0.0, 1.0);
	}
}