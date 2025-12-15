#version 330 core

struct Material {
  vec4 base_map;
  sampler2D base_map_texture;
  int has_base_map_texture;
  float smoothness;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 view_pos;

uniform vec3 environment_ambient_color;

uniform Material material;

void main() {
  vec4 result = material.base_map;
  if (material.has_base_map_texture == 1) {
    result *= texture(material.base_map_texture, TexCoords);
  }

  FragColor = result;
}
