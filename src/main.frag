#version 330 core

#define MAX_DIR_LIGHTS 4
#define MINIMUM_A_THRESHOLD 0.01
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

out vec4 FragColor;

uniform int is_lit;

uniform vec3 view_pos;

uniform DirectionalLight directional_lights[MAX_DIR_LIGHTS];
uniform int num_dir_lights;

uniform vec4 object_color;

void main() {
  vec4 result = vec4(0.0);
  result.a = 1.0;

  if (is_lit != 0) {
    for (int i = 0; i < num_dir_lights; i++) {
      DirectionalLight light = directional_lights[i];

      vec3 norm = normalize(Normal);
      vec3 light_dir = normalize(-light.direction);

      float diff = dot(norm, light_dir);
      diff = diff * 0.5 + 0.5;

      vec3 ambient = light.ambient * light.color;

      vec3 diffuse = diff * light.diffuse * light.color;

      vec3 view_dir = normalize(view_pos - FragPos);
      vec3 reflect_dir = reflect(-light_dir, norm);
      float spec = pow(max(dot(view_dir, reflect_dir), 0.0), light.intensity);
      vec3 specular = light.specular * spec * light.color;

      result += vec4((ambient + diffuse + specular), 0.0);
    }
    result *= object_color;
  } else {
    result = object_color;
  }

  if (result.a < MINIMUM_A_THRESHOLD)
    discard;

  result.rgb = pow(result.rgb, vec3(1.0 / GAMMA));
  FragColor = result;
}
