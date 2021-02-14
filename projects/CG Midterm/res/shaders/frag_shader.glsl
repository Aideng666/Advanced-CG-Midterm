#version 410
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inNormal;

uniform sampler2D s_Diffuse;

uniform vec3  u_AmbientCol;
uniform float u_AmbientStrength;

uniform vec3  u_LightPos;
uniform vec3  u_LightDir;
uniform vec3  u_LightCol;
uniform float u_AmbientLightStrength;
uniform float u_SpecularLightStrength;
uniform float u_SpecularLightStrength2;
uniform float u_Shininess;
uniform float u_Time;
uniform vec3 u_Position;
uniform int u_LightNum;

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

vec3 CreateDirectionLight(vec3 pos, float strength)
{
	vec3 lightDir = normalize(pos - inPos);

	vec3 ambient = ((u_AmbientLightStrength * u_LightCol) + (u_AmbientCol * u_AmbientStrength));

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

vec3 Lerp(vec3 p0, vec3 p1, float t)
{
	return (1.0f - t) * p0 + t * p1;
}


void main() { 

	vec4 textureColor = texture(s_Diffuse, inUV);

	float strength  = max(min(((2 * cos(u_Time * radians(180.0f))) + (4 * sin((u_Time * radians(180.0f)) / 4)) + (3 *cos(u_Time * radians(180.0f) * 3))) + (4 * sin(u_Time * radians(180.0f) * 3)), 1), 0);

	vec3 result;

	switch (u_LightNum)
	{

	case 1:
		frag_color = vec4(inColor * textureColor.rgb, 1.0);
		break;

	case 2:
		vec3 ambient = ((u_AmbientLightStrength * u_LightCol) + (u_AmbientCol * u_AmbientStrength));
		frag_color = vec4(ambient * inColor * textureColor.rgb, 1.0);
		break;

	case 3:
		vec3 lightDir = normalize(u_LightPos - inPos);
		vec3 N = normalize(inNormal);

		vec3 viewDir = normalize(u_CamPos - inPos);
		vec3 h		 = normalize(lightDir + viewDir);
		
		vec3 reflectDir = reflect(-lightDir, N);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Shininess);
		vec3 specular = u_SpecularLightStrength * spec * u_LightCol; 

		frag_color = vec4(specular * inColor * textureColor.rgb, 1.0);
		break;

	case 4:
		result = CreateDirectionLight(u_LightPos, u_SpecularLightStrength);

		frag_color = vec4(result  * inColor * textureColor.rgb, 1.0);
		break;

	case 5:
		result = CreateSpotlight(u_Position, u_LightDir, strength, cos(radians(60.0f)));

		result += CreateDirectionLight(u_LightPos, u_SpecularLightStrength);

		frag_color = vec4(result  * inColor * textureColor.rgb, 1.0);
	}
}