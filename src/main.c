// clang-format off

#include <stdio.h>

#include <cglm/cglm.h>
#include <glad/glad.h>

#include <GLFW/glfw3.h>

#define WIDTH 800
#define HEIGHT 600

static const char *vertexShaderSource =
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec3 aNormal;\n"
    "uniform mat4 model;\n"
    "uniform mat4 projection;\n"
    "uniform mat4 view;\n"
    "out vec3 Normal;\n"
    "out vec3 FragPos;\n"
    "void main()\n"
    "{\n"
    "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
    "   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
    "   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
    "}\0";

static const char *fragmentShaderSource =
    "#version 330 core\n"
    "in vec3 FragPos;\n"
    "in vec3 Normal;\n"
    "out vec4 FragColor;\n"
    "uniform vec3 object_color;\n"
    "uniform vec3 light_color;\n"
    "uniform vec3 light_direction;\n"
    "uniform vec3 view_pos;\n"
    "void main()\n"
    "{\n"
    "   float ambient_strength = 0.1;\n"
    "   vec3 ambient = ambient_strength * light_color;\n"
    "   vec3 norm = normalize(Normal);\n"
    "   vec3 light_dir = normalize(-light_direction);\n"
    "   float diff = max(dot(norm, light_dir), 0.0);\n"
    "   vec3 diffuse = diff * light_color;\n"
    "   float specular_strength = 0.4;\n"
    "   vec3 view_dir = normalize(view_pos - FragPos);\n"
    "   vec3 reflect_dir = reflect(-light_dir, norm);\n"
    "   float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);\n"
    "   vec3 specular = specular_strength * spec * light_color;\n"
    "   vec3 result = (ambient + diffuse + specular) * object_color;\n"
    "   FragColor = vec4(result, 1.0);\n"
    "}\n\0";

struct Entity {
  char *name;

  struct Component *components;
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

struct ComponentLight {
  enum LightKind light_kind;
  vec3 color;
  float intensity;
};

struct Component* get_component_by_kind(
    struct Entity *entity,
    enum ComponentKind kind
) {
  for (size_t i = 0; i < 3; i++) {
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

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  struct Entity cube;
  cube.name = "Cube";

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
      .color = {1.0f, 1.0f, 1.0f},
      .intensity = 1.0f,
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

    glUseProgram(shaderProgram);

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

    GLuint model_loc = glGetUniformLocation(shaderProgram, "model");
    GLuint proj_loc = glGetUniformLocation(shaderProgram, "projection");
    GLuint view_loc = glGetUniformLocation(shaderProgram, "view");

    GLuint object_color_loc =
        glGetUniformLocation(shaderProgram, "object_color");
    GLuint light_color_loc = glGetUniformLocation(shaderProgram, "light_color");
    GLuint light_direction_loc = glGetUniformLocation(shaderProgram, "light_direction");
    GLuint view_pos_loc = glGetUniformLocation(shaderProgram, "view_pos");

    mat4 camera_world, rotation;

    vec3 target;
    glm_vec3_add(camera_pos, front, target);
    glm_look(camera_pos, target, up, view_matrix);

    // lights
    for (size_t i = 0; i < entity_count; i++) {
      struct Entity entity = entities[i];

      struct Component *light = NULL;
      if ((light = get_component_by_kind(&entity, CK_LIGHT)) != NULL) {
        struct ComponentLight *light_comp =
            (struct ComponentLight *)light->data;


      }
    }

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
        glUniform3f(light_color_loc, 1.0f, 1.0f, 1.0f);
        glUniform3f(light_direction_loc, 0.0f, 0.0f, -1.0f);
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
