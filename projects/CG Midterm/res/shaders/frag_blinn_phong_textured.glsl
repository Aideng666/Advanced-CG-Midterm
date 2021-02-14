#version 410

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

uniform sampler2D s_Diffuse;
uniform sampler2D s_Diffuse2;
uniform sampler2D s_Specular;

uniform vec3  u_AmbientCol;
uniform float u_AmbientStrength;

uniform vec3  u_LightPos;
uniform vec3  u_LightDir;
uniform vec3  u_LightCol;
uniform float u_AmbientLightStrength;
uniform float u_SpecularLightStrength;
uniform float u_Shininess;
// NEW in week 7, see https://learnopengl.com/Lighting/Light-casters for a good reference on how this all works, or
// https://developer.valvesoftware.com/wiki/Constant-Linear-Quadratic_Falloff
uniform float u_LightAttenuationConstant;
uniform float u_LightAttenuationLinear;
uniform float u_LightAttenuationQuadratic;

uniform float u_Time;
uniform vec3 u_Position;
uniform int u_LightNum;

uniform float u_TextureMix;

uniform vec3  u_CamPos;

out vec4 frag_color;


vec3 CreateSpotlight(vec3 pos, vec3 direction, float strength, float cutoff)
{
	vec3 lightDir = normalize(pos - inPos);

	vec3 ambient = ((u_AmbientLightStrength * u_LightCol) + (u_AmbientCol * u_AmbientStrength));

	float theta = dot(lightDir, normalize(-direction));

	if (theta > cutoff)
	{
		vec3 N = normalize(inNormal);

		
		float dif = max(dot(N, lightDir), 0.0);
		vec3 diffuse = dif * u_LightCol;// add diffuse intensity
		float dist = length(u_LightPos - inPos);
		diffuse = diffuse / dist; // (dist*dist)

		vec3 viewDir = normalize(u_CamPos - inPos);
		vec3 h		 = normalize(lightDir + viewDir);
		
		vec3 reflectDir = reflect(-lightDir, N);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
		vec3 specular = strength * spec * u_LightCol; 

		return (vec3(ambient + diffuse + specular));
	}
	else
	{
		return ambient;
	}

}

// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() {
	// Lecture 5
	vec3 ambient = u_AmbientLightStrength * u_LightCol;

	// Diffuse
	vec3 N = normalize(inNormal);
	vec3 lightDir = normalize(u_LightPos - inPos);

	float dif = max(dot(N, lightDir), 0.0);
	vec3 diffuse = dif * u_LightCol;// add diffuse intensity

	//Attenuation
	float dist = length(u_LightPos - inPos);
	float attenuation = 1.0f / (
		u_LightAttenuationConstant + 
		u_LightAttenuationLinear * dist +
		u_LightAttenuationQuadratic * dist * dist);

	// Specular
	vec3 viewDir  = normalize(u_CamPos - inPos);
	vec3 h        = normalize(lightDir + viewDir);

	// Get the specular power from the specular map
	float texSpec = texture(s_Specular, inUV).x;
	float spec = pow(max(dot(N, h), 0.0), u_Shininess); // Shininess coefficient (can be a uniform)
	vec3 specular = u_SpecularLightStrength * spec * u_LightCol; // Can also use a specular color

	// Get the albedo from the diffuse / albedo map
	vec4 textureColor = texture(s_Diffuse, inUV);
//	vec4 textureColor2 = texture(s_Diffuse2, inUV);
//	vec4 textureColor = mix(textureColor1, textureColor2, u_TextureMix);

	float strength  = max(min(((2 * cos(u_Time * radians(180.0f)))
		+ (4 * sin((u_Time * radians(180.0f)) / 4)) 
		+ (3 *cos(u_Time * radians(180.0f) * 3)))
		+ (4 * sin(u_Time * radians(180.0f) * 3)), 1), 0);

		vec3 result;

	switch (u_LightNum)
	{
		case 1:
			
			frag_color = vec4(inColor * textureColor.rgb, 1.0);
			break;

		case 2:
				
			result = (u_AmbientCol * u_AmbientStrength) + ambient;

			frag_color = vec4(result * inColor * textureColor.rgb, 1.0);
			break;

		case 3:
			
			frag_color = vec4(specular * inColor * textureColor.rgb, 1.0);
			break;

		case 4:

			result = (
				(u_AmbientCol * u_AmbientStrength) + // global ambient light
				(ambient + diffuse + specular) * attenuation // light factors from our single light
				) * inColor * textureColor.rgb; // Object color

				frag_color = vec4(result, textureColor.a);

				break;
		case 5:

			result = CreateSpotlight(u_Position, u_LightDir, strength, cos(radians(60.0f)));

			frag_color = vec4(result * inColor * textureColor.rgb, textureColor.a);

			break;
	}
}