#version 330 core

#define MAX_DIR_LIGHTS 4
#define MINIMUM_A_THRESHOLD 0.0
#define GAMMA 2.2

struct DirectionalLight {
  vec3 direction;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  vec3 color;

  float intensity;
};

struct Material {
  vec4 base_map;
  vec3 specular_map;

  sampler2D base_map_texture;
  int has_base_map_texture;

  float smoothness;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 view_pos;

uniform DirectionalLight directional_lights[MAX_DIR_LIGHTS];
uniform int num_dir_lights;

uniform Material material;

uniform vec3 environment_ambient_color;

void main() {
  vec4 result = vec4(0.0);

  vec3 lighting = vec3(0.0);
  vec3 reflection = vec3(0.0);

  vec3 norm = normalize(Normal);
  vec3 view_dir = normalize(view_pos - FragPos);

  vec4 base_map_color = material.base_map;
  if (material.has_base_map_texture != 0) {
    base_map_color *= texture(material.base_map_texture, TexCoords);
  }

  // float color_dist = length(base_map_color.rgb - material.specular_map) / sqrt(3.0);

  // // vec3 color = base_map_color.rgb * (1.0 - color_dist) + material.specular_map * color_dist;
  // vec3 color = mix(base_map_color.rgb, material.specular_map, color_dist);

  for (int i = 0; i < num_dir_lights; i++) {
    DirectionalLight light = directional_lights[i];

    vec3 light_dir = normalize(-light.direction);

    float diff = max(dot(norm, light_dir), 0.0);

    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), light.intensity);

    vec3 l_ambient = light.ambient * light.color + environment_ambient_color;
    vec3 l_diffuse = diff * light.diffuse * light.color;
    vec3 l_specular = vec3(spec);

    vec3 l_reflection = vec3(dot(norm, light_dir)) * light.color;
    l_reflection *= dot(norm, view_dir);
    // vec3 l_reflection = vec3(dot(norm, view_dir));

    vec3 add_lighting = (l_ambient + l_diffuse + l_specular) * base_map_color.rgb;

    if (length(material.specular_map) > MINIMUM_A_THRESHOLD) {
      float color_distance = min(length(add_lighting - material.specular_map), 1.0);
      lighting = mix(add_lighting, material.specular_map, color_distance);
    } else {
      lighting += add_lighting;
    }

    // lighting = vec3(color_distance);

    // reflection = l_reflection;
  }

  vec4 refl = vec4(reflection, length(reflection));
  vec4 reflection_strength = vec4(material.specular_map, base_map_color.a) + ((1 - material.smoothness) / 8.0);

  result = (refl * reflection_strength) + vec4(lighting, base_map_color.a);
  // result = vec4(vec3(distance_between_basemap_and_specular), 1.0);

  result.rgb = pow(result.rgb, vec3(1.0 / GAMMA));

  FragColor = result;
}
