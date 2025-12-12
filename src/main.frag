#version 330 core

#define MAX_DIR_LIGHTS 4

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

uniform vec3 view_pos;

uniform DirectionalLight directional_lights[MAX_DIR_LIGHTS];
uniform int num_dir_lights;

uniform vec3 object_color;

void main() {
  vec3 result = vec3(0.0);

  for (int i = 0; i < num_dir_lights; i++) {
    DirectionalLight light = directional_lights[i];

    vec3 ambient = light.ambient * light.color;

    vec3 norm = normalize(Normal);
    vec3 light_dir = normalize(-light.direction);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = light.diffuse * diff * light.color;

    vec3 view_dir = normalize(view_pos - FragPos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), light.intensity);
    vec3 specular = light.specular * spec * light.color;

    result += (ambient + diffuse + specular) * object_color;
  }

  FragColor = vec4(result, 1.0);
}
