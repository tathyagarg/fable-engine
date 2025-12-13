// clang-format off

#include <stdio.h>
#include <string.h>

#include <cglm/cglm.h>
#include <glad/glad.h>

#include <GLFW/glfw3.h>

#define WIDTH 800
#define HEIGHT 600

#define PERSP_FOV glm_rad(45.0f)
#define PERSP_NEAR 0.1f
#define PERSP_FAR 100.0f

//  TODO: Load from config file
#define TITLE "Fable Engine"

static const vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};

struct Entity {
  char *name;

  struct Component *components;
  unsigned int component_count;
  unsigned int reserved_components;
};

enum ComponentKind {
  CK_TRANSFORM,
  CK_MESH_FILTER,
  CK_MESH_RENDERER,
  CK_MATERIAL,
  CK_LIGHT,
};

struct Component {
  enum ComponentKind kind;
  void *data;
};

struct ComponentTransform {
  vec3 position;
  vec3 rotation;
  vec3 scale;
};

enum MeshFilterKind {
  MFK_CUBE,
  MFK_SPHERE,
  MFK_PLANE,
  MFK_CUSTOM,
};

struct ComponentMeshFilter {
  enum MeshFilterKind mesh_kind;
  GLuint vao;
  unsigned int vertex_count;
};

enum MaterialKind {
  MK_LIT,
  MK_UNLIT,
};

enum MaterialSurfaceType {
  MST_OPAQUE,
  MST_TRANSPARENT,
};

enum MaterialRenderFace {
  MRF_FRONT,
  MRF_BACK,
  MRF_DOUBLE,
};

struct Material {
  enum MaterialKind material_kind;
  enum MaterialSurfaceType surface_type;
  enum MaterialRenderFace render_face;

  vec4 color;

  GLboolean is_alpha_clipping;
  float alpha_clip_threshold;
};

struct ComponentMeshRenderer {
  GLboolean is_enabled;

  struct Material** materials;
  unsigned int material_count;
};

enum LightKind {
  LK_DIRECTIONAL,
  LK_POINT,
  LK_SPOT,
};

struct DirLightData {
  vec3 direction;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

union LightData {
  struct DirLightData dir_light;
};

struct ComponentLight {
  enum LightKind light_kind;
  union LightData light_data;

  vec3 color;

  float intensity;
};

struct Component* get_component_by_kind(
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

static const float CUBE_VERTICES[] = {
  // back face (−Z)  CCW
  0.5f, -0.5f, -0.5f,  0, 0,-1,
  -0.5f, -0.5f, -0.5f,  0, 0,-1,
  -0.5f,  0.5f, -0.5f,  0, 0,-1,
  0.5f, -0.5f, -0.5f,  0, 0,-1,
  -0.5f,  0.5f, -0.5f,  0, 0,-1,
  0.5f,  0.5f, -0.5f,  0, 0,-1,

  // front face (+Z) CCW
  -0.5f, -0.5f,  0.5f,  0, 0, 1,
  0.5f, -0.5f,  0.5f,  0, 0, 1,
  0.5f,  0.5f,  0.5f,  0, 0, 1,
  -0.5f, -0.5f,  0.5f,  0, 0, 1,
  0.5f,  0.5f,  0.5f,  0, 0, 1,
  -0.5f,  0.5f,  0.5f,  0, 0, 1,

  // left face (−X) CCW
  -0.5f, -0.5f, -0.5f, -1, 0, 0,
  -0.5f, -0.5f,  0.5f, -1, 0, 0,
  -0.5f,  0.5f,  0.5f, -1, 0, 0,
  -0.5f, -0.5f, -0.5f, -1, 0, 0,
  -0.5f,  0.5f,  0.5f, -1, 0, 0,
  -0.5f,  0.5f, -0.5f, -1, 0, 0,

  // right face (+X) CCW
  0.5f, -0.5f, -0.5f,  1, 0, 0,
  0.5f,  0.5f,  0.5f,  1, 0, 0,
  0.5f, -0.5f,  0.5f,  1, 0, 0,
  0.5f, -0.5f, -0.5f,  1, 0, 0,
  0.5f,  0.5f, -0.5f,  1, 0, 0,
  0.5f,  0.5f,  0.5f,  1, 0, 0,

  // bottom face (−Y) CCW
  -0.5f, -0.5f, -0.5f,  0,-1, 0,
  0.5f, -0.5f, -0.5f,  0,-1, 0,
  0.5f, -0.5f,  0.5f,  0,-1, 0,
  -0.5f, -0.5f, -0.5f,  0,-1, 0,
  0.5f, -0.5f,  0.5f,  0,-1, 0,
  -0.5f, -0.5f,  0.5f,  0,-1, 0,

  // top face (+Y) CCW
  -0.5f,  0.5f, -0.5f,  0, 1, 0,
  -0.5f,  0.5f,  0.5f,  0, 1, 0,
  0.5f,  0.5f,  0.5f,  0, 1, 0,
  -0.5f,  0.5f, -0.5f,  0, 1, 0,
  0.5f,  0.5f,  0.5f,  0, 1, 0,
  0.5f,  0.5f, -0.5f,  0, 1, 0,
};

GLuint cube_vao() {
  int vao, vbo;

  glGenVertexArrays(1, (GLuint *)&vao);
  glGenBuffers(1, (GLuint *)&vbo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(CUBE_VERTICES), CUBE_VERTICES,
    GL_STATIC_DRAW
  );

  glVertexAttribPointer(
    0, 3, GL_FLOAT,
    GL_FALSE, 6 * sizeof(float),
    (void *)0
  );
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(
    1, 3, GL_FLOAT,
    GL_FALSE, 6 * sizeof(float),
    (void *)(3 * sizeof(float))
  );
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return vao;
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

/*
 * Alpha calculation with clipping
 * +---------+-----------+-----------------+-----------------+
 * | Opaque? | Clipping? | Alpha > Thresh? | Resulting Alpha |
 * +---------+-----------+-----------------+-----------------+
 * |   Yes   |    Yes    |       Yes       |      1.0f       |
 * |   Yes   |    Yes    |       No        |      0.0f       |
 * |   Yes   |    No     |       N/A       |      1.0f       |
 * |   No    |    Yes    |       Yes       |     Alpha       |
 * |   No    |    Yes    |       No        |      0.0f       |
 * |   No    |    No     |       N/A       |     Alpha       |
 * +---------+-----------+-----------------+-----------------+
 * */
float calculate_alpha(struct Material material) {
  if (!material.is_alpha_clipping || (
      material.is_alpha_clipping &&
      material.color[3] > material.alpha_clip_threshold)) {
    if (material.surface_type == MST_OPAQUE) {
      return 1.0f;
    } else {
      return material.color[3];
    }
  } else {
    return 0.0f;
  }
}

struct Entity empty_entity() {
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
  glad_glViewport(0, 0, width, height);

  mat4* projection =
    (mat4 *)glfwGetWindowUserPointer(window);

  float aspect = (float)width / (float)height;

  glm_perspective(PERSP_FOV, aspect,
    PERSP_NEAR, PERSP_FAR, *projection);
}

int main() {
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

  char* vertex_shader_source = NULL;
  char* fragment_shader_source = NULL;

  if (read_file("src/main.vert", &vertex_shader_source) == 1) {
    return -1;
  }

  if (read_file("src/main.frag", &fragment_shader_source) == 1) {
    return -1;
  }

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1,
    (const char* const*)&vertex_shader_source, NULL);
  glCompileShader(vertex_shader);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1,
    (const char* const*)&fragment_shader_source, NULL);
  glCompileShader(fragment_shader);

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  struct Material lit = {
    .material_kind = MK_LIT,
    .surface_type = MST_TRANSPARENT,
    .render_face = MRF_FRONT,
    .is_alpha_clipping = GL_FALSE,
  };
  rgba_to_vec4(255, 0, 0, 100, &lit.color);

  struct Material unlit = {
    .material_kind = MK_UNLIT,
    .surface_type = MST_TRANSPARENT,
    .color = {1.0f, 1.0f, 1.0f, 0.2f},
  };

  struct Entity cube = empty_entity();
  cube.name = "Cube";

  struct Material* cube_materials = malloc(1 * sizeof(struct Material));
  cube_materials[0] = lit;

  add_component(&cube, (struct Component){
    .kind = CK_TRANSFORM,
    .data = &(struct ComponentTransform){
      .position = {0.0f, 0.0f, 0.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {1.0f, 1.0f, 1.0f},
    },
  });

  add_component(&cube, (struct Component){
    .kind = CK_MESH_FILTER,
    .data = &(struct ComponentMeshFilter){
      .mesh_kind = MFK_CUBE,
      .vao = cube_vao(),
      .vertex_count = 36,
    },
  });

  add_component(&cube, (struct Component){
    .kind = CK_MESH_RENDERER,
    .data = &(struct ComponentMeshRenderer){
      .is_enabled = GL_TRUE,
      .materials = &cube_materials,
      .material_count = 1,
    },
  });

  struct Entity light = empty_entity();
  light.name = "Light";
  add_component(&light, (struct Component){
    .kind = CK_TRANSFORM,
    .data = &(struct ComponentTransform){
      .position = {0.0f, 100.0f, -50.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {1.0f, 1.0f, 1.0f},
    },
  });

  add_component(&light, (struct Component){
    .kind = CK_LIGHT,
    .data = &(struct ComponentLight){
      .light_kind = LK_DIRECTIONAL,
      .light_data.dir_light = {
        .direction = {-0.4f, -0.3f, -0.5f},
        .ambient = {0.02f, 0.02f, 0.02f},
        .diffuse = {0.5f, 0.5f, 0.5f},
        .specular = {1.0f, 1.0f, 1.0f},
      },
      .color = {1.0f, 1.0f, 1.0f},
      .intensity = 64.0f,
    },
  });

  struct Entity entities[] = {cube, light};
  size_t entity_count = sizeof(entities) / sizeof(entities[0]);

  float aspect = (float)WIDTH / (float)HEIGHT;
  float near = 0.1f;
  float far = 100.0f;

  mat4 view_matrix;
  mat4 projection;

  glm_perspective(PERSP_FOV, aspect,
    PERSP_NEAR, PERSP_FAR, projection);

  vec3 front = {0.0f, 0.0f, -1.0f};
  vec3 right, up, target;

  float camera_pos[] = {-1.0f, 1.0f, 5.0f};

  update_camera_vectors(front, right, up);
  glm_vec3_add(camera_pos, front, target);
  glm_look(camera_pos, target, up, view_matrix);

  glEnable(GL_DEPTH_TEST);

  glfwSetWindowUserPointer(window, &projection);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.4f, 0.388f, 0.376f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    update_camera_vectors(front, right, up);
    glm_vec3_add(camera_pos, front, target);
    glm_lookat(camera_pos, target, up, view_matrix);

    glUseProgram(shader_program);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, 1);

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(up, -0.1f, translation);
      glm_vec3_add(camera_pos, translation, camera_pos);
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(up, 0.1f, translation);
      glm_vec3_add(camera_pos, translation, camera_pos);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(front, 0.1f, translation);
      glm_vec3_add(camera_pos, translation, camera_pos);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(front, -0.1f, translation);
      glm_vec3_add(camera_pos, translation, camera_pos);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(right, -0.1f, translation);
      glm_vec3_add(camera_pos, translation, camera_pos);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(right, 0.1f, translation);
      glm_vec3_add(camera_pos, translation, camera_pos);
    }

    GLuint model_loc =
      glGetUniformLocation(shader_program, "model");
    GLuint proj_loc =
      glGetUniformLocation(shader_program, "projection");
    GLuint view_loc =
      glGetUniformLocation(shader_program, "view");

    GLuint object_color_loc =
        glGetUniformLocation(shader_program, "object_color");

    GLuint view_pos_loc =
      glGetUniformLocation(shader_program, "view_pos");

    GLuint is_lit_loc =
      glGetUniformLocation(shader_program, "is_lit");

    int light_count = 0;

    for (size_t i = 0; i < entity_count; i++) {
      struct Entity entity = entities[i];

      struct Component *light = NULL;
      if ((
        light = get_component_by_kind(&entity, CK_LIGHT)
      ) != NULL) {
        struct ComponentLight *light_comp =
            (struct ComponentLight *)light->data;

        if (light_comp->light_kind == LK_DIRECTIONAL) {
          struct DirLightData dir_light_data = light_comp->light_data.dir_light;

          char buffer[100];
          sprintf(buffer, "directional_lights[%d].color", light_count);

          GLuint light_color_loc =
              glGetUniformLocation(shader_program, buffer);

          memset(buffer, 0, sizeof(buffer));
          sprintf(buffer, "directional_lights[%d].direction", light_count);
          GLuint light_direction_loc =
              glGetUniformLocation(shader_program, buffer);

          memset(buffer, 0, sizeof(buffer));
          sprintf(buffer, "directional_lights[%d].ambient", light_count);
          GLuint light_ambient_loc =
              glGetUniformLocation(shader_program, buffer);

          memset(buffer, 0, sizeof(buffer));
          sprintf(buffer, "directional_lights[%d].diffuse", light_count);
          GLuint light_diffuse_loc =
              glGetUniformLocation(shader_program, buffer);

          memset(buffer, 0, sizeof(buffer));
          sprintf(buffer, "directional_lights[%d].specular", light_count);
          GLuint light_specular_loc =
              glGetUniformLocation(shader_program, buffer);

          memset(buffer, 0, sizeof(buffer));
          sprintf(buffer, "directional_lights[%d].intensity", light_count);
          GLuint light_intensity_loc =
              glGetUniformLocation(shader_program, buffer);

          glUniform3fv(light_ambient_loc, 1,
            dir_light_data.ambient);
          glUniform3fv(light_diffuse_loc, 1,
            dir_light_data.diffuse);
          glUniform3fv(light_specular_loc, 1,
            dir_light_data.specular);
          glUniform3fv(light_color_loc, 1,
            light_comp->color);
          glUniform3fv(light_direction_loc, 1,
            dir_light_data.direction);
          glUniform1f(light_intensity_loc, light_comp->intensity);

          // vec3 *specular = &((struct ComponentLight *)light->data)
          //     ->light_data.dir_light.specular;
          // float s = (float)(sin(glfwGetTime())) * 0.5f + 0.5f;

          // glm_vec3_adds(*specular, s, *specular);

          light_count++;
        }
      }
    }

    GLuint num_dir_lights_loc =
        glGetUniformLocation(shader_program, "num_dir_lights");
    glUniform1i(num_dir_lights_loc, light_count);

    for (size_t i = 0; i < entity_count; i++) {
      struct Entity entity = entities[i];

      struct Component *mesh_r = NULL;
      if ((
        mesh_r = get_component_by_kind(&entity, CK_MESH_RENDERER)
      ) != NULL) {
        struct ComponentMeshRenderer *mesh_renderer =
            (struct ComponentMeshRenderer *)mesh_r->data;

        if (!mesh_renderer->is_enabled) {
          continue;
        }

        struct ComponentMeshFilter* mesh_filter =
            (struct ComponentMeshFilter *)get_component_by_kind(
                &entity, CK_MESH_FILTER)
                ->data;
        if (mesh_filter == NULL) {
          continue;
        }

        struct ComponentTransform *transform =
            (struct ComponentTransform *)get_component_by_kind(
                &entity, CK_TRANSFORM)
                ->data;
        if (transform == NULL) {
          continue;
        }

        struct Material* materials = *mesh_renderer->materials;
        if (materials == NULL || mesh_renderer->material_count == 0) {
          continue;
        }

        mat4 model;
        glm_mat4_identity(model);
        glm_translate(model, transform->position);
        glm_rotate_x(model, transform->rotation[0], model);
        glm_rotate_y(model, transform->rotation[1], model);
        glm_rotate_z(model, transform->rotation[2], model);
        glm_scale(model, transform->scale);

        glUniformMatrix4fv(model_loc, 1,
          GL_FALSE, (float *)model);
        glUniformMatrix4fv(proj_loc, 1,
          GL_FALSE, (float *)projection);
        glUniformMatrix4fv(view_loc, 1,
          GL_FALSE, (float *)view_matrix);

        for (int i = 0; i < mesh_renderer->material_count; i++) {
          struct Material material = materials[i];

          if (material.surface_type == MST_TRANSPARENT) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            switch (material.render_face) {
              case MRF_FRONT:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);

                glDepthMask(GL_FALSE);
                break;
              case MRF_BACK:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);

                glDepthMask(GL_FALSE);
                break;
              case MRF_DOUBLE:
                glDisable(GL_CULL_FACE);
                break;
            }
          } else {
            glDisable(GL_BLEND);
            glDisable(GL_CULL_FACE);

            glDepthMask(GL_TRUE);
          }

          glUniform4f(object_color_loc,
              material.color[0],
              material.color[1],
              material.color[2],
              calculate_alpha(material)
          );

          glUniform3fv(view_pos_loc, 1, camera_pos);

          glUniform1i(
              is_lit_loc,
              material.material_kind == MK_LIT
          );

          glBindVertexArray(mesh_filter->vao);
          glDrawArrays(GL_TRIANGLES, 0,
            mesh_filter->vertex_count);
        }
      }
    }

    glfwSwapBuffers(window);
    glfwPollEvents();

    glfwWaitEventsTimeout(1.0 / 60.0);
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}
// clang-format on
