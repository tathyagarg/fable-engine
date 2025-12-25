// clang-format off

#include <stdio.h>
#include <string.h>

#include <cglm/cglm.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <GLFW/glfw3.h>
#include "fable/fable.h"

#define WIDTH 800
#define HEIGHT 600

#define PERSP_FOV glm_rad(63.0f)
#define PERSP_NEAR 0.1f
#define PERSP_FAR 100.0f

#define DEBUG
#define SHOW_COLLIDERS

#define FRAME_RATE 60.0f

//  TODO: Load from config file
#define TITLE "Fable Engine"

struct Component* get_comp_by_kind(
    struct Entity *entity,
    enum ComponentKind kind
) {
  for (size_t i = 0; i < entity->component_count; i++) {
    if (entity->components[i].kind == kind) {
      return &entity->components[i];
    }
  }

  return NULL;
}

int read_file(const char* path, char** buffer) {
  FILE* file = fopen(path, "r");
  if (!file) {
    fprintf(stderr, "Failed to open shader file: %s\n", path);
    return 1;
  }

  fseek(file, 0, SEEK_END);

  long length = ftell(file);

  fseek(file, 0, SEEK_SET);

  *buffer = malloc((length + 1) * sizeof(char));
  fread(*buffer, 1, length, file);

  (*buffer)[length] = '\0';
  fclose(file);

  return 0;
}

void update_camera_vectors(vec3 front, vec3 right, vec3 up) {
  glm_normalize(front);

  glm_cross(front, (float*)WORLD_UP, right);
  glm_normalize(right);

  glm_cross(right, front, up);
  glm_normalize(up);
}

void rgba_to_vec4(int r, int g, int b, int a, vec4* out_color) {
  (*out_color)[0] = r / 255.0f;
  (*out_color)[1] = g / 255.0f;
  (*out_color)[2] = b / 255.0f;
  (*out_color)[3] = a / 255.0f;
}

struct Entity empty_entity(void) {
  struct Entity entity;
  entity.name = "Empty";
  entity.component_count = 0;
  entity.reserved_components = 0;
  entity.components = NULL;

  return entity;
}

void add_component(
    struct Entity* entity,
    struct Component component
) {
  if (entity->reserved_components == 0) {
    entity->reserved_components = 4;
    entity->components = malloc(
      entity->reserved_components * sizeof(struct Component)
    );
  } else if (entity->component_count >= entity->reserved_components) {
    entity->reserved_components *= 2;
    entity->components = realloc(
      entity->components,
      entity->reserved_components * sizeof(struct Component)
    );
  }

  entity->components[entity->component_count++] = component;
}

void framebuffer_size_callback(
    GLFWwindow* window,
    int width,
    int height
) {
  struct Context* context =
    (struct Context *)glfwGetWindowUserPointer(window);

  // float aspect = (float)width / (float)height;

  (*context->framebuffer_size)[0] = width;
  (*context->framebuffer_size)[1] = height;
}

struct Texture load_texture(const char* path) {
  struct Texture texture;

  glGenTextures(1, &texture.id);
  unsigned char* data = stbi_load(path,
    &texture.width,
    &texture.height,
    &texture.channels,
    0
  );

  if (data) {
    GLenum format;
    if (texture.channels == 1)
      format = GL_RED;
    else if (texture.channels == 3)
      format = GL_RGB;
    else if (texture.channels == 4)
      format = GL_RGBA;
    else {
      fprintf(stderr, "Unsupported texture format: %s\n", path);
      stbi_image_free(data);
      texture.id = 0;
      texture.width = 0;
      texture.height = 0;
      texture.channels = 0;
      return texture;
    }

    printf("Bound texture: %s (ID: %d, %dx%d, %d channels)\n",
      path, texture.id,
      texture.width,
      texture.height,
      texture.channels
    );
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, format,
      texture.width, texture.height,
      0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D,
      GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,
      GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,
      GL_TEXTURE_MIN_FILTER,
      GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,
      GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  } else {
    fprintf(stderr, "Failed to load texture: %s\n", path);
    texture.id = 0;
    texture.width = 0;
    texture.height = 0;
    texture.channels = 0;
  }

  return texture;
}

void uniform_material(GLuint program, struct Material material) {
  GLuint has_base_map_texture_loc =
      glGetUniformLocation(program, "material.has_base_map_texture");

  if (material.base_map_texture->texture != NULL) {
    printf("Using base map texture ID: %d\n",
      material.base_map_texture->texture->id);

    GLuint base_map_loc =
        glGetUniformLocation(program, "material.base_map_texture");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,
      material.base_map_texture->texture->id);
    glUniform1i(base_map_loc, 0);

    glUniform1i(has_base_map_texture_loc, GL_TRUE);
  } else {
    glUniform1i(has_base_map_texture_loc, GL_FALSE);
  }

  GLuint base_map_loc =
      glGetUniformLocation(program, "material.base_map");
  glUniform4fv(base_map_loc, 1,
    material.base_map_texture->color);

  GLuint specular_map_loc =
      glGetUniformLocation(program, "material.specular_map");
  glUniform3fv(specular_map_loc, 1,
    material.specular_map);

  GLuint smoothness_loc =
      glGetUniformLocation(program, "material.smoothness");
  glUniform1f(smoothness_loc,
    material.smoothness);

  GLuint is_alpha_clipping_loc =
      glGetUniformLocation(program, "material.is_alpha_clipping");
  glUniform1i(is_alpha_clipping_loc,
    material.is_alpha_clipping ? GL_TRUE : GL_FALSE);

  GLuint alpha_clip_threshold_loc =
      glGetUniformLocation(program, "material.alpha_clip_threshold");
  glUniform1f(alpha_clip_threshold_loc,
    material.alpha_clip_threshold);

  GLuint surface_type_loc =
      glGetUniformLocation(program, "material.surface_type");
  glUniform1i(surface_type_loc,
    material.surface_type);

  GLuint is_preserve_spec_high_loc =
      glGetUniformLocation(program, "material.is_preserve_spec_high");
  glUniform1i(is_preserve_spec_high_loc,
    material.is_preserve_specular_highlights);
}

void uniform_directional_light(
  GLuint program,
  int i,
  struct DirLightData dir_light_data,
  struct ComponentLight light_comp
) {
  char buffer[100];
  sprintf(buffer, "directional_lights[%d].color", i);
  GLuint light_color_loc =
      glGetUniformLocation(program, buffer);

  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "directional_lights[%d].direction", i);
  GLuint light_direction_loc =
      glGetUniformLocation(program, buffer);

  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "directional_lights[%d].ambient", i);
  GLuint light_ambient_loc =
      glGetUniformLocation(program, buffer);

  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "directional_lights[%d].diffuse", i);
  GLuint light_diffuse_loc =
      glGetUniformLocation(program, buffer);

  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "directional_lights[%d].specular", i);
  GLuint light_specular_loc =
      glGetUniformLocation(program, buffer);

  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "directional_lights[%d].intensity", i);
  GLuint light_intensity_loc =
      glGetUniformLocation(program, buffer);

  glUniform3fv(light_ambient_loc, 1,
    dir_light_data.ambient);
  glUniform3fv(light_diffuse_loc, 1,
    dir_light_data.diffuse);
  glUniform3fv(light_specular_loc, 1,
    dir_light_data.specular);
  glUniform3fv(light_color_loc, 1,
    light_comp.color);
  glUniform3fv(light_direction_loc, 1,
    dir_light_data.direction);
  glUniform1f(light_intensity_loc, light_comp.intensity);
}

GLuint load_shader(const char* path, GLenum shader_type) {
  char* shader_source = NULL;
  if (read_file(path, &shader_source) != 0)
    return 0;

  GLuint shader = glCreateShader(shader_type);
  glShaderSource(shader, 1,
    (const char* const*)&shader_source, NULL);
  glCompileShader(shader);

  free(shader_source);

  return shader;
}

void integrate_entity(
  struct ComponentTransform* transform,
  struct ComponentRigidbody* rb,
  float duration
) {
  if (rb->is_kinematic) return;

  glm_vec3_muladds(rb->velocity, duration, transform->position);

  vec3 resulting_acc;
  glm_vec3_copy(rb->acceleration, resulting_acc);

  glm_vec3_muladds(rb->force_acc, 1 / rb->mass, resulting_acc);
  glm_vec3_muladds(resulting_acc, duration, rb->velocity);

  float damping = powf(rb->linear_damping, duration);
  glm_vec3_scale(rb->velocity, damping, rb->velocity);

  glm_vec3_zero(rb->force_acc);

  printf("Hi\n");
  glm_vec3_muladds(rb->angular_vel, duration, transform->rotation);

  vec3 resulting_angular_acc;
  glm_vec3_copy(rb->angular_acc, resulting_angular_acc);

  glm_vec3_muladds(rb->torque_acc, 1 / rb->mass, resulting_angular_acc);
  glm_vec3_muladds(resulting_angular_acc, duration, rb->angular_vel);

  // TODO: Switch to angular damping
  damping = powf(rb->linear_damping, duration);
  glm_vec3_scale(rb->angular_vel, damping, rb->angular_vel);

  DISPLAY_VEC3(rb->angular_vel);

  glm_vec3_zero(rb->torque_acc);
}

void get_collider_obb(
  struct ComponentBoxCollider* box,
  struct ComponentTransform* transform,
  vec3 out_corners[8]
) {
  mat4 model;
  glm_mat4_identity(model);
  glm_translate(model, transform->position);
  glm_rotate_x(model, transform->rotation[0], model);
  glm_rotate_y(model, transform->rotation[1], model);
  glm_rotate_z(model, transform->rotation[2], model);

  vec3 center_translation;
  glm_vec3_negate_to(box->center, center_translation);
  glm_translate(model, center_translation);

  for (int i = 0; i < 8; i++) {
    glm_vec3_zero(out_corners[i]);

    vec4 local_corner = {
      (i & 1 ? 0.5f : -0.5f) * box->size[0],
      (i & 2 ? 0.5f : -0.5f) * box->size[1],
      (i & 4 ? 0.5f : -0.5f) * box->size[2],
      1.0f
    };

    vec4 rotated_corner;
    glm_mat4_mulv(model, local_corner, rotated_corner);
    glm_vec3_copy(rotated_corner, out_corners[i]);
  }
}

GLboolean overlap_on_axis(
  vec3 a_corners[8],
  vec3 b_corners[8],
  vec3 axis,
  float* out_penetration_depth
) {
  float a_min = FLT_MAX;
  float a_max = -FLT_MAX;
  float b_min = FLT_MAX;
  float b_max = -FLT_MAX;

  for (int i = 0; i < 8; i++) {
    float a_proj = glm_vec3_dot(a_corners[i], axis);
    float b_proj = glm_vec3_dot(b_corners[i], axis);

    if (a_proj < a_min) a_min = a_proj;
    if (a_proj > a_max) a_max = a_proj;
    if (b_proj < b_min) b_min = b_proj;
    if (b_proj > b_max) b_max = b_proj;
  }

  if (a_max < b_min || b_max < a_min) {
    return GL_FALSE;
  } else {
    float overlap1 = a_max - b_min;
    float overlap2 = b_max - a_min;
    *out_penetration_depth = (overlap1 < overlap2) ? overlap1 : overlap2;
    return GL_TRUE;
  }
}

void box_and_box_collision(
  struct ComponentBoxCollider* box_a,
  struct ComponentTransform* transform_a,
  struct ComponentBoxCollider* box_b,
  struct ComponentTransform* transform_b,
  struct CollisionManifold* out_manifold
) {
#ifdef COLLIDER_TYPE_AABB
  /*
   * NOTE:
   * While I could delete this AABB method in favor of the OBB method,
   * I want to keep it around just for funsies
   * */
  vec3 a_min, a_max;
  vec3 b_min, b_max;

  glm_vec3_add(transform_a->position, box_a->center, a_min);
  glm_vec3_muladds(transform_a->scale, 0.5f, a_min);
  glm_vec3_copy(a_min, a_max);
  glm_vec3_muladds(box_a->size, -0.5f, a_min);
  glm_vec3_muladds(box_a->size, 0.5f, a_max);

  glm_vec3_add(transform_b->position, box_b->center, b_min);
  glm_vec3_muladds(transform_b->scale, 0.5f, b_min);
  glm_vec3_copy(b_min, b_max);
  glm_vec3_muladds(box_b->size, -0.5f, b_min);
  glm_vec3_muladds(box_b->size, 0.5f, b_max);

  if (a_min[0] > b_max[0] || a_max[0] < b_min[0] ||
      a_min[1] > b_max[1] || a_max[1] < b_min[1] ||
      a_min[2] > b_max[2] || a_max[2] < b_min[2]) {
    out_manifold->is_colliding = GL_FALSE;
  } else {
    out_manifold->is_colliding = GL_TRUE;

    float dx1 = b_max[0] - a_min[0];
    float dx2 = a_max[0] - b_min[0];
    float dy1 = b_max[1] - a_min[1];
    float dy2 = a_max[1] - b_min[1];
    float dz1 = b_max[2] - a_min[2];
    float dz2 = a_max[2] - b_min[2];

    float min_dx = (dx1 < dx2) ? dx1 : dx2;
    float min_dy = (dy1 < dy2) ? dy1 : dy2;
    float min_dz = (dz1 < dz2) ? dz1 : dz2;

    glm_vec3_zero(out_manifold->normal);
    if (min_dx <= min_dy && min_dx <= min_dz) {
      out_manifold->penetration_depth = min_dx;
      out_manifold->normal[0] = (dx1 < dx2) ? -1.0f : 1.0f;
    }
    else if (min_dy <= min_dx && min_dy <= min_dz) {
      out_manifold->penetration_depth = min_dy;
      out_manifold->normal[1] = (dy1 < dy2) ? -1.0f : 1.0f;
    }
    else {
      out_manifold->penetration_depth = min_dz;
      out_manifold->normal[2] = (dz1 < dz2) ? -1.0f : 1.0f;
    }

    glm_vec3_normalize(out_manifold->normal);
  }
#else
  vec3 a_corners[8];
  vec3 b_corners[8];

  get_collider_obb(box_a, transform_a, a_corners);
  get_collider_obb(box_b, transform_b, b_corners);

  vec3 axes[15];
  glm_vec3_sub(a_corners[1], a_corners[0], axes[0]);
  glm_vec3_sub(a_corners[2], a_corners[0], axes[1]);
  glm_vec3_sub(a_corners[4], a_corners[0], axes[2]);

  glm_vec3_sub(b_corners[1], b_corners[0], axes[3]);
  glm_vec3_sub(b_corners[2], b_corners[0], axes[4]);
  glm_vec3_sub(b_corners[4], b_corners[0], axes[5]);

  for (int i = 0; i < 3; i++) {
    for (int j = 3; j < 6; j++) {
      glm_vec3_cross(axes[i], axes[j], axes[6 + (i * 3) + (j - 3)]);
    }
  }

  out_manifold->is_colliding = GL_TRUE;
  out_manifold->penetration_depth = FLT_MAX;
  for (int i = 0; i < 15; i++) {
    float penetration = 0.0f;
    if (!overlap_on_axis(
        a_corners,
        b_corners,
        axes[i],
        &penetration
      )) {
      out_manifold->is_colliding = GL_FALSE;
      return;
    }

    if (penetration < out_manifold->penetration_depth) {
      out_manifold->penetration_depth = penetration;
      glm_vec3_copy(axes[i], out_manifold->normal);
    }
  }

  glm_vec3_normalize(out_manifold->normal);

  float best = FLT_MAX;
  for (int i = 0; i < 8; i++) {
    float distance = glm_vec3_dot(
      a_corners[i],
      out_manifold->normal
    );
    if (distance < best) {
      best = distance;
      glm_vec3_copy(a_corners[i], out_manifold->contact_point);
    }
  }

  vec3 correction;
  glm_vec3_scale(
    out_manifold->normal,
    out_manifold->penetration_depth * 0.5f,
    correction
  );
  glm_vec3_add(
    out_manifold->contact_point,
    correction,
    out_manifold->contact_point
  );
#endif
}

int main(void) {
  float delta_time = 1.0f / FRAME_RATE;

  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  glfwWindowHint(GLFW_DEPTH_BITS, 24);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  GLFWwindow *window =
    glfwCreateWindow(
      WIDTH, HEIGHT, TITLE,
      NULL, NULL
    );

  if (!window) {
    fprintf(stderr, "Failed to create GLFW window\n");
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  gladLoadGL();

  GLuint vertex_shader = load_shader("src/main.vert", GL_VERTEX_SHADER);
  GLuint lit_frag_shader = load_shader("src/lit.frag", GL_FRAGMENT_SHADER);
  GLuint unlit_frag_shader = load_shader("src/unlit.frag", GL_FRAGMENT_SHADER);
  GLuint collider_vert_shader = load_shader("src/collider.vert", GL_VERTEX_SHADER);
  GLuint collider_frag_shader = load_shader("src/collider.frag", GL_FRAGMENT_SHADER);

  GLuint lit_program = glCreateProgram();
  glAttachShader(lit_program, vertex_shader);
  glAttachShader(lit_program, lit_frag_shader);
  glLinkProgram(lit_program);

  GLuint unlit_program = glCreateProgram();
  glAttachShader(unlit_program, vertex_shader);
  glAttachShader(unlit_program, unlit_frag_shader);
  glLinkProgram(unlit_program);

  GLuint collider_program = glCreateProgram();
  glAttachShader(collider_program, collider_vert_shader);
  glAttachShader(collider_program, collider_frag_shader);
  glLinkProgram(collider_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(lit_frag_shader);
  glDeleteShader(unlit_frag_shader);
  glDeleteShader(collider_vert_shader);
  glDeleteShader(collider_frag_shader);

  // struct Texture box = load_texture("assets/textures/box.jpg");
  // struct Texture knob = load_texture("assets/textures/knob.png");

  const GLuint CUBE_VAO = cube_vao();

  struct Material mat1 = {
    .material_shader = MS_LIT,
    .surface_type = MST_OPAQUE,
    .render_face = MRF_FRONT,
    .is_preserve_specular_highlights = GL_TRUE,
    .is_alpha_clipping = GL_FALSE,

    .base_map_texture = &(struct ColoredTexture){
      .texture = NULL,
      .color = {0.0f, 1.0f, 0.0f, 1.0f}
    },
    .specular_map = {0.0f, 0.0f, 0.0f},
    .smoothness = 0.25,
  };

  struct Material mat2 = {
    .material_shader = MS_LIT,
    .surface_type = MST_OPAQUE,
    .render_face = MRF_FRONT,
    .is_alpha_clipping = GL_FALSE,
    .smoothness = 0.5,
    .specular_map = {0.0f, 0.0f, 0.0f},
    .base_map_texture = &(struct ColoredTexture){
      .texture = NULL,
    },
  };
  rgba_to_vec4(255, 0, 0, 255,
               &mat2.base_map_texture->color);

  struct Entity platform = empty_entity();
  platform.name = "Platform";

  struct Material* platform_mats = malloc(1 * sizeof(struct Material));
  platform_mats[0] = mat1;

  add_component(&platform, (struct Component){
    .kind = CK_TRANSFORM,
    .is_enabled = GL_TRUE,
    .data.transform = &(struct ComponentTransform){
      .position = {0.0f, 0.0f, 0.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {5.0f, 1.0f, 5.0f},
    },
  });

  add_component(&platform, (struct Component){
    .kind = CK_MESH_FILTER,
    .is_enabled = GL_TRUE,
    .data.mesh_filter = &(struct ComponentMeshFilter){
      .mesh_kind = MFK_CUBE,
      .vao = CUBE_VAO,
      .vertex_count = CUBE_VERTEX_COUNT,
    },
  });

  add_component(&platform, (struct Component){
    .kind = CK_MESH_RENDERER,
    .is_enabled = GL_TRUE,
    .data.mesh_renderer = &(struct ComponentMeshRenderer){
      .materials = &platform_mats,
      .material_count = 1,
    },
  });

  add_component(&platform, (struct Component){
    .kind = CK_BOX_COLLIDER,
    .is_enabled = GL_TRUE,
    .data.box_collider = &(struct ComponentBoxCollider){
      .size = {5.0f, 1.0f, 5.0f},
      .center = {0.0f, 0.0f, 0.0f},
    },
  });

  struct Entity cube = empty_entity();
  cube.name = "Cube";

  struct Material* cube_mats = malloc(1 * sizeof(struct Material));
  cube_mats[0] = mat2;

  add_component(&cube, (struct Component){
    .kind = CK_TRANSFORM,
    .is_enabled = GL_TRUE,
    .data.transform = &(struct ComponentTransform){
      .position = {0.0f, 4.0f, 0.0f},
      .rotation = ROTATION_VEC_DEG(0.0f, 45.0f, 45.0f),
      .scale = {1.0f, 1.0f, 1.0f},
    },
  });

  add_component(&cube, (struct Component){
    .kind = CK_MESH_FILTER,
    .is_enabled = GL_TRUE,
    .data.mesh_filter = &(struct ComponentMeshFilter){
      .mesh_kind = MFK_CUBE,
      .vao = CUBE_VAO,
      .vertex_count = CUBE_VERTEX_COUNT
    },
  });

  add_component(&cube, (struct Component){
    .kind = CK_MESH_RENDERER,
    .is_enabled = GL_TRUE,
    .data.mesh_renderer = &(struct ComponentMeshRenderer){
      .materials = &cube_mats,
      .material_count = 1,
    },
  });

  struct Component rb = {
    .kind = CK_RIGIDBODY,
    .is_enabled = GL_TRUE,
    .data.rigidbody = &(struct ComponentRigidbody){
      .mass = 1.0f,
      .is_kinematic = GL_FALSE,
      .linear_damping = 1.0f,
      .force_generators = (struct ForceGenerator[]){GRAVITY_GENERATOR},
      .force_generator_count = 1,
      .torque_generator_count = 0,
    },
  };
  rb.data.rigidbody->torque_generators = malloc(
    rb.data.rigidbody->torque_generator_count *
    sizeof(struct TorqueGenerator)
  );

  add_component(&cube, rb);

  add_component(&cube, (struct Component){
    .kind = CK_BOX_COLLIDER,
    .is_enabled = GL_TRUE,
    .data.box_collider = &(struct ComponentBoxCollider){
      .size = {1.0f, 1.0f, 1.0f},
      .center = {0.0f, 0.0f, 0.0f},
    },
  });

  vec3 ambient_color = {0.0f, 0.0f, 1.0f};

  struct Entity light = empty_entity();
  light.name = "Light";
  add_component(&light, (struct Component){
    .kind = CK_TRANSFORM,
    .is_enabled = GL_TRUE,
    .data.transform = &(struct ComponentTransform){
      .position = {0.0f, 100.0f, -50.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {1.0f, 1.0f, 1.0f},
    },
  });

  add_component(&light, (struct Component){
    .kind = CK_LIGHT,
    .is_enabled = GL_TRUE,
    .data.light = &(struct ComponentLight){
      .light_kind = LK_DIRECTIONAL,
      .light_data.dir_light = {
        .direction = {0.0f, 0.0f, 1.0f},
        .ambient = {0.0f, 0.0f, 0.0f},
        .diffuse = {1.0f, 1.0f, 1.0f},
        .specular = {1.0f, 1.0f, 1.0f},
      },
      .color = {1.0f, 1.0f, 1.0f},
      .intensity = 64.0f,
    },
  });

  struct Entity camera = empty_entity();
  camera.name = "Camera";
  add_component(&camera, (struct Component){
    .kind = CK_CAMERA,
    .is_enabled = GL_TRUE,
    .data.camera = &(struct ComponentCamera){
      .fovy = PERSP_FOV,
      .near = PERSP_NEAR,
      .far = PERSP_FAR,
      .is_perspective = GL_TRUE,
      .is_display_to_screen = GL_TRUE,
      .viewport_rect = {0.0f, 0.0f, 1.0f, 1.0f},
      .background_kind = CBK_COLOR,
      .background_data.color = {0.2f, 0.2f, 0.2f, 1.0f},
    },
  });

  add_component(&camera, (struct Component){
    .kind = CK_TRANSFORM,
    .is_enabled = GL_TRUE,
    .data.transform = &(struct ComponentTransform){
      .position = {0.0f, 2.0f, -10.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {1.0f, 1.0f, 1.0f},
    },
  });

  struct Entity entities[] = {cube, platform, light, camera};
  size_t entity_count = sizeof(entities) / sizeof(entities[0]);

  float aspect = (float)WIDTH / (float)HEIGHT;
  // float near = 0.1f;
  // float far = 100.0f;

  mat4 view_matrix;
  mat4 projection;

  glm_perspective(PERSP_FOV, aspect,
    PERSP_NEAR, PERSP_FAR, projection);

  vec3 front = {0.0f, 0.0f, 1.0f};
  vec3 right, up, target;

  update_camera_vectors(front, right, up);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);
  glClearDepth(1.0f);

  int* framebuffer_size = malloc(2 * sizeof(int));
  glfwGetFramebufferSize(window,
    &framebuffer_size[0],
    &framebuffer_size[1]);

  struct Context context = {
    .projection = &projection,
    .framebuffer_size = &framebuffer_size,
  };

  glfwSetWindowUserPointer(window, &context);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  struct Component *camera_comp = NULL;
  struct ComponentCamera* camera_data = NULL;
  struct ComponentTransform* cam_transform = NULL;

  for (size_t i = 0; i < entity_count; i++) {
    struct Entity entity = entities[i];

    if ((
      camera_comp = get_comp_by_kind(&entity, CK_CAMERA)
    ) != NULL) {
      cam_transform =
          get_comp_by_kind(
              &entity, CK_TRANSFORM)
              ->data.transform;

      camera_data = camera_comp->data.camera;
      break;
    }
  }

  vec3 previous_rot = {0.0f, 0.0f, 1.0f};

  int is_playing = 1;

  while (!glfwWindowShouldClose(window)) {
    float width = framebuffer_size[0];
    float height = framebuffer_size[1];

    if (!glm_vec3_eqv(cam_transform->rotation, previous_rot)) {
      printf("Camera rotation changed\n");
      glm_vec3_copy(cam_transform->rotation, previous_rot);

      vec3 rotated_front;
      glm_vec3_copy(front, rotated_front);

      glm_vec3_rotate(rotated_front,
        cam_transform->rotation[0], (float*)POS_X_AXIS);
      glm_vec3_rotate(rotated_front,
        cam_transform->rotation[1], (float*)POS_Y_AXIS);
      glm_vec3_rotate(rotated_front,
        cam_transform->rotation[2], (float*)POS_Z_AXIS);

      update_camera_vectors(rotated_front, right, up);
      glm_vec3_copy(rotated_front, front);
    }

    glm_vec3_add(cam_transform->position, front, target);
    glm_lookat(cam_transform->position, target, up,
      view_matrix);

    float vp_x = camera_data->viewport_rect[0] * width;
    float vp_y = camera_data->viewport_rect[1] * height;
    float vp_w = camera_data->viewport_rect[2] * width;
    float vp_h = camera_data->viewport_rect[3] * height;

    glEnable(GL_SCISSOR_TEST);
    glScissor(vp_x, vp_y, vp_w, vp_h);

    switch (camera_data->background_kind) {
      case CBK_COLOR:
        glClearColor(
          camera_data->background_data.color[0],
          camera_data->background_data.color[1],
          camera_data->background_data.color[2],
          camera_data->background_data.color[3]
        );
        break;
      case CBK_SKYBOX:
        break;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);

    glad_glViewport(camera_data->viewport_rect[0] * width,
      camera_data->viewport_rect[1] * height,
      camera_data->viewport_rect[2] * width,
      camera_data->viewport_rect[3] * height);


    aspect = vp_w / vp_h;
    glm_perspective(camera_data->fovy, aspect,
      camera_data->near, camera_data->far, projection);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, 1);

#ifdef DEBUG
    vec3 translation = {0.0f, 0.0f, 0.0f};
    if (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
      glm_vec3_muladds(up, -0.1f, translation);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
      glm_vec3_muladds(up, 0.1f, translation);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      glm_vec3_muladds(front, 0.1f, translation);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      glm_vec3_muladds(front, -0.1f, translation);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      glm_vec3_muladds(right, -0.1f, translation);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      glm_vec3_muladds(right, 0.1f, translation);

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
      is_playing = !is_playing;

    glm_vec3_add(cam_transform->position, translation,
      cam_transform->position);
#endif

    // begin render pipeline
    int light_count = 0;
    struct ComponentLight dir_lights[10];

    for (size_t i = 0; i < entity_count; i++) {
      struct Entity entity = entities[i];

      struct Component *light = NULL;
      if ((
        light = get_comp_by_kind(&entity, CK_LIGHT)
      ) != NULL) {
        struct ComponentLight *light_comp = light->data.light;

        if (light_comp->light_kind == LK_DIRECTIONAL) {
          dir_lights[light_count++] = *light_comp;
        }
      }
    }

    for (size_t entity_i = 0; entity_i < entity_count; entity_i++) {
      struct Entity entity = entities[entity_i];

      struct Component *mesh_r = NULL;

      if ((
        mesh_r = get_comp_by_kind(&entity, CK_MESH_RENDERER)
      ) != NULL) {
        struct ComponentMeshRenderer *mesh_renderer =
            mesh_r->data.mesh_renderer;
        if (!mesh_r->is_enabled) continue;

        struct ComponentMeshFilter* mesh_filter =
            get_comp_by_kind(
                &entity, CK_MESH_FILTER)->data.mesh_filter;
        if (mesh_filter == NULL) continue;

        struct ComponentTransform *transform =
            get_comp_by_kind(
                &entity, CK_TRANSFORM)->data.transform;
        if (transform == NULL) continue;

        struct Material* materials = *mesh_renderer->materials;
        if (materials == NULL || mesh_renderer->material_count == 0) continue;

        mat4 model;
        glm_mat4_identity(model);
        glm_translate(model, transform->position);
        glm_rotate_x(model, transform->rotation[0], model);
        glm_rotate_y(model, transform->rotation[1], model);
        glm_rotate_z(model, transform->rotation[2], model);
        glm_scale(model, transform->scale);

        for (unsigned int i = 0; i < mesh_renderer->material_count; i++) {
          struct Material material = materials[i];

          GLuint program;
          if (material.material_shader == MS_LIT) {
            glUseProgram(lit_program);
            program = lit_program;

            for (int i = 0; i < light_count; i++) {
              struct ComponentLight light_comp = dir_lights[i];
              struct DirLightData dir_light_data =
                light_comp.light_data.dir_light;

              uniform_directional_light(program, i, dir_light_data,
                light_comp);
            }

          } else {
            glUseProgram(unlit_program);
            program = unlit_program;
          }

          GLuint model_loc =
            glGetUniformLocation(program, "model");
          glUniformMatrix4fv(model_loc, 1,
            GL_FALSE, (float *)model);
          GLuint proj_loc =
            glGetUniformLocation(program, "projection");
          glUniformMatrix4fv(proj_loc, 1,
            GL_FALSE, (float *)projection);
          GLuint view_loc =
            glGetUniformLocation(program, "view");
          glUniformMatrix4fv(view_loc, 1,
            GL_FALSE, (float *)view_matrix);

          GLuint view_pos_loc =
            glGetUniformLocation(program, "view_pos");

          GLuint num_dir_lights_loc =
              glGetUniformLocation(program, "num_dir_lights");
          glUniform1i(num_dir_lights_loc, light_count);

          GLuint environment_ambient_color_loc =
              glGetUniformLocation(program, "environment_ambient_color");
          glUniform3fv(environment_ambient_color_loc, 1,
            ambient_color);

          glUniform3fv(view_pos_loc, 1,
            cam_transform->position);

          uniform_material(program, material);

          glDepthMask(GL_TRUE);
          if (material.surface_type == MST_TRANSPARENT) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glDepthFunc(GL_LESS);

            switch (material.render_face) {
              case MRF_FRONT:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                break;
              case MRF_BACK:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
                break;
              case MRF_DOUBLE:
                glDisable(GL_CULL_FACE);
                break;
            }
          } else {
            glDisable(GL_BLEND);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            glDisable(GL_POLYGON_OFFSET_FILL);

            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
          }
        }

        glBindVertexArray(mesh_filter->vao);
        glDrawArrays(GL_TRIANGLES, 0,
          mesh_filter->vertex_count);

#ifdef SHOW_COLLIDERS
        struct Component* box_collider_comp =
            get_comp_by_kind(
                &entity, CK_BOX_COLLIDER);

        if (box_collider_comp != NULL) {
          glUseProgram(collider_program);

          vec3 points[8];
          get_collider_obb(
            box_collider_comp->data.box_collider,
            transform,
            points
          );

          for (int i = 0; i < 8; i++) {
            mat4 point_model;
            glm_mat4_identity(point_model);
            glm_translate(point_model, points[i]);
            glm_scale(point_model, (vec3){0.1f, 0.1f, 0.1f});

            GLuint model_loc =
              glGetUniformLocation(collider_program, "model");
            glUniformMatrix4fv(model_loc, 1,
              GL_FALSE, (float *)point_model);
            GLuint proj_loc =
              glGetUniformLocation(collider_program, "projection");
            glUniformMatrix4fv(proj_loc, 1,
              GL_FALSE, (float *)projection);
            GLuint view_loc =
              glGetUniformLocation(collider_program, "view");
            glUniformMatrix4fv(view_loc, 1,
              GL_FALSE, (float *)view_matrix);

            glBindVertexArray(CUBE_VAO);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawArrays(GL_TRIANGLES, 0,
              CUBE_VERTEX_COUNT);
          }
        }
#endif
      }
    }
    // end render pipeline
    // begin physics engine
    if (is_playing) {
      for (size_t i = 0; i < entity_count; i++) {
        struct Entity* entity = &entities[i];

        struct Component *rigidbody_comp = NULL;
        struct Component *transform_comp = NULL;

        if ((
          rigidbody_comp = get_comp_by_kind(entity, CK_RIGIDBODY)
        ) != NULL && (
          transform_comp = get_comp_by_kind(entity, CK_TRANSFORM)
        ) != NULL) {
          struct ComponentRigidbody* rigidbody =
              rigidbody_comp->data.rigidbody;
          struct ComponentTransform* transform =
              transform_comp->data.transform;

          if (!rigidbody->is_kinematic) {
            for (int i = 0; i < rigidbody->force_generator_count; i++) {
              struct ForceGenerator* fg =
                  &rigidbody->force_generators[i];
              fg->update_force(rigidbody, delta_time, fg->generator_data);
            }

            for (int i = 0; i < rigidbody->torque_generator_count; i++) {
              struct TorqueGenerator* tg =
                  &rigidbody->torque_generators[i];
              tg->update_torque(rigidbody, delta_time, tg->generator_data);
            }

            integrate_entity(transform, rigidbody, delta_time);

            for (size_t j = 0; j < entity_count; j++) {
              if (i == j) continue;

              struct Entity* entity_b = &entities[j];

              struct Component* b_transform_comp = NULL;

              if ((b_transform_comp =
                get_comp_by_kind(entity_b, CK_TRANSFORM)
              ) != NULL) {
                struct ComponentTransform* b_transform =
                    b_transform_comp->data.transform;

                struct Component* a_bc_comp =
                    get_comp_by_kind(entity, CK_BOX_COLLIDER);
                struct Component* b_bc_comp =
                    get_comp_by_kind(entity_b, CK_BOX_COLLIDER);

                if (a_bc_comp != NULL && b_bc_comp != NULL) {
                  struct ComponentBoxCollider* a_box_collider =
                      a_bc_comp->data.box_collider;
                  struct ComponentBoxCollider* b_box_collider =
                      b_bc_comp->data.box_collider;

                  struct CollisionManifold manifold;
                  box_and_box_collision(
                    a_box_collider,
                    transform,
                    b_box_collider,
                    b_transform,
                    &manifold
                  );
                  if (manifold.is_colliding) {
                    glm_vec3_muladds(
                      manifold.normal,
                      manifold.penetration_depth,
                      transform->position
                    );

                    float speed_along_normal =
                      glm_vec3_dot(rigidbody->velocity, manifold.normal);
                    if (speed_along_normal < 0.0f) {
                      vec3 impulse;
                      glm_vec3_scale(manifold.normal,
                        -speed_along_normal * rigidbody->mass,
                        impulse);

                      glm_vec3_muladds(impulse,
                        1 / rigidbody->mass,
                        rigidbody->velocity);
                      DISPLAY_VEC3(rigidbody->velocity);

                      vec3 center;
                      glm_vec3_copy(transform->position, center);
                      glm_vec3_muladds(
                        a_box_collider->size, 0.5f, center
                      );

                      // angular impulse
                      vec3 r;
                      glm_vec3_sub(
                        manifold.contact_point,
                        center,
                        r
                      );

                      DISPLAY_VEC3(r);
                      DISPLAY_VEC3(impulse);

                      vec3 angular_impulse;
                      glm_vec3_cross(r, impulse, angular_impulse);

                      DISPLAY_VEC3(angular_impulse);

                      glm_vec3_muladds(angular_impulse,
                        1 / (rigidbody->mass * 12.0f),
                        rigidbody->angular_vel);

                      if (rigidbody->torque_generator_count == 0) {
                        (void)realloc(rigidbody->torque_generators,
                          sizeof(struct TorqueGenerator) * 1);

                        rigidbody->torque_generators[0] =
                          BASIC_TORQUE_GENERATOR;

                        rigidbody->torque_generators[0].generator_data = malloc(
                          sizeof(struct BasicTorqueGeneratorData));

                        memcpy(rigidbody->torque_generators[0].generator_data,
                          &(struct BasicTorqueGeneratorData){
                            .contact_point = malloc(sizeof(vec3)),
                            .force = malloc(sizeof(vec3)),
                          }, sizeof(struct BasicTorqueGeneratorData));

                        struct BasicTorqueGeneratorData* tg_data =
                          rigidbody->torque_generators[0].generator_data;

                        glm_vec3_copy(manifold.contact_point, *tg_data->contact_point);

                        glm_vec3_copy((float*)GRAVITY_VEC, *tg_data->force);

                        rigidbody->torque_generator_count = 1;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    // end physics engine

    glfwSwapBuffers(window);
    glfwPollEvents();

    glfwWaitEventsTimeout(delta_time);
  }

  free(framebuffer_size);
  free(cube_mats);
  free(platform_mats);
  for (size_t i = 0; i < entity_count; i++) {
    free(entities[i].components);
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}
// clang-format on
