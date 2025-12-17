#version 330 core

#define MAX_DIR_LIGHTS 4
#define MINIMUM_A_THRESHOLD 0.1
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

float fresnel(vec3 view_dir, vec3 normal) {
  float f = 1.0 - max(dot(view_dir, normal), 0.0);
  return pow(f, 5.0);
}

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

    lighting = (l_ambient + l_diffuse) * base_map_color.rgb + l_specular * material.specular_map;
    reflection = l_reflection;
  }

  vec4 refl = vec4(reflection, length(reflection));
  vec4 reflection_strength = vec4(material.specular_map, base_map_color.a) + ((1 - material.smoothness) / 8.0);

  result = (refl * reflection_strength) + vec4(lighting, base_map_color.a);

  result.rgb = pow(result.rgb, vec3(1.0 / GAMMA));

  FragColor = result;
}
