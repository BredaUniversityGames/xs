#version 460 core
in vec3 v_position;
in vec4 v_color;				
in vec2 v_texture_coordinate;
in vec3 v_normal;
out vec4 frag_color;
uniform int u_num_directional_lights;
uniform int u_num_point_lights;
uniform vec4 u_add_color;

const int MAX_DIR_LIGHTS = 4;
const int MAX_POINT_LIGHTS = 4;

struct directional_light
{	
	vec3 direction;
	vec3 color;
	bool cast_shadow;
	mat4 shadow_matrix;
};

struct point_light
{
	vec3 position;	
	vec3 color;
	float radius;	
};

uniform directional_light u_directional_lights[MAX_DIR_LIGHTS];
uniform point_light u_point_lights[MAX_POINT_LIGHTS];

layout(location = 1)	uniform sampler2D u_texture;
layout(location = 10)  	uniform sampler2DShadow[4] s_shadow_maps;

void main()
{
	// Get diffuse color from texture
	vec4 texture_color = texture(u_texture, v_texture_coordinate);
	
	// Initialize diffuse color
	vec4 diffuse_color = vec4(0.0, 0.0, 0.0, 1.0);

	//diffuse_color.rgb = pow(texture_color.rgb, vec3(1.0 / 2.2));
	
	// calculate diffuse light intensity for directional lights		 
	for (int i = 0; i < u_num_directional_lights; i++)
	{		
		directional_light light = u_directional_lights[i];
		float intensity = max(dot(normalize(v_normal), normalize(-light.direction)), 0.0);

		// Get shadow map
		float shadow = 1.0;
		if(light.cast_shadow)
		{
			vec4 pos_light_space = light.shadow_matrix * vec4(v_position, 1.0);
			vec3 proj_coords = pos_light_space.xyz / pos_light_space.w;
			proj_coords = proj_coords * 0.5 + 0.5;
			proj_coords.z -= 0.001; // bias
			shadow = texture(s_shadow_maps[i], proj_coords);
		}
		diffuse_color += vec4(intensity) * vec4(light.color, 1.0) * shadow;
	}

	// calculate diffuse light intensity for point lights
	for (int i = 0; i < u_num_point_lights; i++)
	{
		point_light light = u_point_lights[i];
		vec3 light_direction = light.position - v_position;
		float distance = length(light_direction);		
		light_direction = normalize(light_direction);

		// GLTF 2.0 Spec for attenuation
		float distance2 = distance * distance;
		float attenuation = max( min( 1.0 - pow(distance / light.radius, 4), 1), 0 ) / distance2;
		
		diffuse_color += max(dot(normalize(v_normal), light_direction), 0.0) *
			vec4(light.color, 1.0) * attenuation;
	}

	// Get diffuse color from texture
	diffuse_color *= texture_color;

	// Get diffuse color from vertex color	
	// diffuse_color *= v_color;

	// Set output color
	frag_color = v_color * diffuse_color + u_add_color;
} 
