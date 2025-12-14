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

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform int is_lit;

uniform vec3 view_pos;

uniform DirectionalLight directional_lights[MAX_DIR_LIGHTS];
uniform int num_dir_lights;

uniform vec4 object_color;

uniform sampler2D albedo_texture;
uniform int use_albedo_texture;

float fresnel(vec3 view_dir, vec3 normal) {
  float f = 1.0 - max(dot(view_dir, normal), 0.0);
  return pow(f, 5.0);
}

void main() {
  vec4 result = vec4(0.0);

  if (is_lit != 0) {
    vec3 lighting = vec3(0.0);

    vec3 norm = normalize(Normal);
    vec3 view_dir = normalize(view_pos - FragPos);
    for (int i = 0; i < num_dir_lights; i++) {
      DirectionalLight light = directional_lights[i];

      vec3 light_dir = normalize(-light.direction);

      float diff = dot(norm, light_dir);
      diff = diff * 0.5 + 0.5;
      diff = max(diff, 0);

      vec3 ambient = light.ambient * light.color;

      vec3 diffuse = diff * light.diffuse * light.color;

      vec3 reflect_dir = reflect(-light_dir, norm);
      float spec = pow(max(dot(view_dir, reflect_dir), 0.0), light.intensity);
      vec3 specular = light.specular * spec * light.color;

      lighting += (ambient + diffuse + specular);
    }

    vec4 albedo = vec4(0.1);

    if (use_albedo_texture != 0) {
      vec4 texture_color = texture(albedo_texture, TexCoords);

      if (texture_color.a != 0) {
        albedo = texture_color * object_color;
      }
    } else {
      albedo = object_color;
    }

    float f = fresnel(view_dir, norm);
    lighting = lighting * f + lighting * (1.0 - f) * MINIMUM_A_THRESHOLD;

    result = vec4(lighting, 1.0) * albedo;
  } else {
    result = object_color;
  }

  result.rgb = pow(result.rgb, vec3(1.0 / GAMMA));

  FragColor = result;
}
