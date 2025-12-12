// clang-format off

#include <stdio.h>
#include <string.h>

#include <cglm/cglm.h>
#include <glad/glad.h>

#include <GLFW/glfw3.h>

#define WIDTH 800
#define HEIGHT 600

struct Entity {
  char *name;

  struct Component *components;
  unsigned int component_count;
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

struct ComponentMeshRenderer {
  unsigned int is_enabled;
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
  -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,
  0.5f,  -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,
  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
  0.5f,  0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
  -0.5f, 0.5f,  -0.5f, 0.0f,  0.0f,  -1.0f,
  -0.5f, -0.5f, -0.5f, 0.0f,  0.0f,  -1.0f,

  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,
  0.5f,  -0.5f, 0.5f, 0.0f,  0.0f,  1.0f,
  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
  -0.5f, 0.5f,  0.5f, 0.0f,  0.0f,  1.0f,
  -0.5f, -0.5f, 0.5f,  0.0f,  0.0f,  1.0f,

  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,
  -0.5f, 0.5f,  -0.5f, -1.0f, 0.0f,  0.0f,
  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
  -0.5f, -0.5f, -0.5f, -1.0f, 0.0f,  0.0f,
  -0.5f, -0.5f, 0.5f, -1.0f, 0.0f,  0.0f,
  -0.5f, 0.5f,  0.5f,  -1.0f, 0.0f,  0.0f,

  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
  0.5f,  0.5f,  -0.5f, 1.0f,  0.0f,  0.0f,
  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
  0.5f,  -0.5f, -0.5f, 1.0f,  0.0f,  0.0f,
  0.5f,  -0.5f, 0.5f, 1.0f,  0.0f,  0.0f,
  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,
  0.5f,  -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,
  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
  0.5f,  -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
  -0.5f, -0.5f, 0.5f,  0.0f,  -1.0f, 0.0f,
  -0.5f, -0.5f, -0.5f, 0.0f,  -1.0f, 0.0f,

  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,
  0.5f,  0.5f,  -0.5f, 0.0f,  1.0f,  0.0f,
  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
  -0.5f, 0.5f,  0.5f, 0.0f,  1.0f,  0.0f,
  -0.5f, 0.5f,  -0.5f, 0.0f,  1.0f,  0.0f
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
  printf("Shader file %s length: %ld\n", path, length);

  fseek(file, 0, SEEK_SET);

  *buffer = malloc((length + 1) * sizeof(char));
  fread(*buffer, 1, length, file);

  (*buffer)[length] = '\0';
  fclose(file);

  return 0;
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

  GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL Window", NULL, NULL);

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
  glShaderSource(vertex_shader, 1, (const char* const*)&vertex_shader_source, NULL);
  glCompileShader(vertex_shader);

  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, (const char* const*)&fragment_shader_source, NULL);
  glCompileShader(fragment_shader);

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  struct Entity cube;
  cube.name = "Cube";
  cube.component_count = 3;

  cube.components = malloc(3 * sizeof(struct Component));

  cube.components[0] = (struct Component){
    .kind = CK_TRANSFORM,
    .data = &(struct ComponentTransform){
      .position = {0.0f, 0.0f, -10.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {1.0f, 1.0f, 1.0f},
    },
  };
  cube.components[1] = (struct Component){
    .kind = CK_MESH_FILTER,
    .data = &(struct ComponentMeshFilter){
      .mesh_kind = MFK_CUBE,
      .vao = cube_vao(),
      .vertex_count = 36,
    },
  };
  cube.components[2] = (struct Component){
    .kind = CK_MESH_RENDERER,
    .data = &(struct ComponentMeshRenderer){
      .is_enabled = 1,
    },
  };

  struct Entity light;
  light.name = "Light";
  light.component_count = 2;

  light.components = malloc(2 * sizeof(struct Component));
  light.components[0] = (struct Component){
    .kind = CK_TRANSFORM,
    .data = &(struct ComponentTransform){
      .position = {0.0f, 100.0f, -50.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {1.0f, 1.0f, 1.0f},
    },
  };

  light.components[1] = (struct Component){
    .kind = CK_LIGHT,
    .data = &(struct ComponentLight){
      .light_kind = LK_DIRECTIONAL,
      .light_data.dir_light = {
        .direction = {-0.2f, -1.0f, -0.3f},
        .ambient = {0.2f, 0.2f, 0.2f},
        .diffuse = {0.5f, 0.5f, 0.5f},
        .specular = {1.0f, 1.0f, 1.0f},
      },
      .color = {1.0f, 1.0f, 1.0f},
      .intensity = 32.0f,
    },
  };

  struct Entity entities[] = {cube, light};
  size_t entity_count = sizeof(entities) / sizeof(entities[0]);

  float aspect = (float)WIDTH / (float)HEIGHT;
  float near = 0.1f;
  float far = 100.0f;

  mat4 view_matrix;
  mat4 projection;

  glm_perspective(glm_rad(45.0f), aspect, near, far, projection);

  vec3 front = {0.0f, 0.0f, -1.0f};
  vec3 right, up;

  vec3 world_up = {0.0f, 1.0f, 0.0f};

  glm_normalize(front);

  glm_cross(front, world_up, right);
  glm_normalize(right);

  glm_cross(right, front, up);
  glm_normalize(up);

  float camera_pos[] = {0.0f, 0.0f, 0.0f};

  glm_look(camera_pos, front, up, view_matrix);

  float angle = 0.02f;

  glEnable(GL_DEPTH_TEST);

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm_normalize(front);
    glm_cross(front, world_up, right);
    glm_normalize(right);
    glm_cross(right, front, up);
    glm_normalize(up);

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

    GLuint model_loc = glGetUniformLocation(shader_program, "model");
    GLuint proj_loc = glGetUniformLocation(shader_program, "projection");
    GLuint view_loc = glGetUniformLocation(shader_program, "view");

    GLuint object_color_loc =
        glGetUniformLocation(shader_program, "object_color");
    GLuint view_pos_loc = glGetUniformLocation(shader_program, "view_pos");

    mat4 camera_world, rotation;

    vec3 target;
    glm_vec3_add(camera_pos, front, target);
    glm_lookat(camera_pos, target, up, view_matrix);

    // lights
    int light_count = 0;

    for (size_t i = 0; i < entity_count; i++) {
      struct Entity entity = entities[i];

      struct Component *light = NULL;
      if ((light = get_component_by_kind(&entity, CK_LIGHT)) != NULL) {
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

          glUniform3fv(light_ambient_loc, 1, dir_light_data.ambient);
          glUniform3fv(light_diffuse_loc, 1, dir_light_data.diffuse);
          glUniform3fv(light_specular_loc, 1, dir_light_data.specular);
          glUniform3fv(light_color_loc, 1, light_comp->color);
          glUniform3fv(light_direction_loc, 1, dir_light_data.direction);
          glUniform1f(light_intensity_loc, light_comp->intensity);

          light_count++;
        }
      }
    }

    GLuint num_dir_lights_loc =
        glGetUniformLocation(shader_program, "num_dir_lights");
    glUniform1i(num_dir_lights_loc, light_count);

    for (size_t i = 0; i < entity_count; i++) {
      struct Entity entity = entities[i];

      struct Component *renderer = NULL;
      if ((renderer = get_component_by_kind(&entity, CK_MESH_RENDERER))
              != NULL) {
        struct ComponentMeshRenderer *mesh_renderer =
            (struct ComponentMeshRenderer *)renderer->data;

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

        mat4 model_matrix;
        glm_mat4_identity(model_matrix);
        glm_translate(model_matrix, transform->position);
        glm_rotate_x(model_matrix, transform->rotation[0], model_matrix);
        glm_rotate_y(model_matrix, transform->rotation[1], model_matrix);
        glm_rotate_z(model_matrix, transform->rotation[2], model_matrix);
        glm_scale(model_matrix, transform->scale);

        glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float *)model_matrix);
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *)projection);
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float *)view_matrix);


        glUniform3f(object_color_loc, 1.0f, 1.0f, 1.0f);
        glUniform3fv(view_pos_loc, 1, camera_pos);


        glBindVertexArray(mesh_filter->vao);
        glDrawArrays(GL_TRIANGLES, 0, mesh_filter->vertex_count);
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
