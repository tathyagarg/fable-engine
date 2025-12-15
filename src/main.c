// clang-format off

#include <stdio.h>
#include <string.h>

#include <cglm/cglm.h>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <GLFW/glfw3.h>

#define WIDTH 800
#define HEIGHT 600

#define PERSP_FOV glm_rad(63.0f)
#define PERSP_NEAR 0.1f
#define PERSP_FAR 100.0f

//  TODO: Load from config file
#define TITLE "Fable Engine"

static const vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};

struct Context {
  mat4 *projection;
  int** framebuffer_size;
};

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
  CK_CAMERA,
};

struct Component {
  enum ComponentKind kind;
  GLboolean is_enabled;

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

struct Texture {
  GLuint id;

  int width;
  int height;
  int channels;
};

struct Material {
  enum MaterialKind material_kind;
  enum MaterialSurfaceType surface_type;
  enum MaterialRenderFace render_face;

  struct Texture* base_map_texture;
  vec4 base_map;
  float smoothness;

  GLboolean is_alpha_clipping;
  float alpha_clip_threshold;
};

struct ComponentMeshRenderer {
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

enum CameraBackgroundKind {
  CBK_COLOR,
  CBK_SKYBOX,
};

union CameraBackgroundData {
  vec4 color;
  // Texture skybox;
};

struct ComponentCamera {
  float fovy;
  float near;
  float far;

  GLboolean is_perspective;

  GLboolean is_display_to_screen;
  // Texture target;

  float viewport_rect[4];

  enum CameraBackgroundKind background_kind;
  union CameraBackgroundData background_data;
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
  0.5f, -0.5f, -0.5f, 0, 0,-1, 0.0f, 0.0f,
  -0.5f, -0.5f, -0.5f, 0, 0,-1, 1.0f, 0.0f,
  -0.5f, 0.5f, -0.5f, 0, 0,-1, 1.0f, 1.0f,
  0.5f, -0.5f, -0.5f, 0, 0,-1, 0.0f, 0.0f,
  -0.5f, 0.5f, -0.5f, 0, 0,-1, 1.0f, 1.0f,
  0.5f, 0.5f, -0.5f, 0, 0,-1, 0.0f, 1.0f,

  // front face (+Z) CCW
  -0.5f, -0.5f, 0.5f, 0, 0, 1, 0.0f, 0.0f,
  0.5f, -0.5f, 0.5f, 0, 0, 1, 1.0f, 0.0f,
  0.5f, 0.5f, 0.5f, 0, 0, 1, 1.0f, 1.0f,
  -0.5f, -0.5f, 0.5f, 0, 0, 1, 0.0f, 0.0f,
  0.5f, 0.5f, 0.5f, 0, 0, 1, 1.0f, 1.0f,
  -0.5f, 0.5f, 0.5f, 0, 0, 1, 0.0f, 1.0f,

  // left face (−X) CCW
  -0.5f, -0.5f, 0.5f, -1, 0, 0, 1.0f, 0.0f,
  -0.5f, 0.5f, 0.5f, -1, 0, 0, 1.0f, 1.0f,
  -0.5f, -0.5f, -0.5f, -1, 0, 0, 0.0f, 0.0f,
  -0.5f, 0.5f, 0.5f, -1, 0, 0, 1.0f, 1.0f,
  -0.5f, 0.5f, -0.5f, -1, 0, 0, 0.0f, 1.0f,
  -0.5f, -0.5f, -0.5f, -1, 0, 0, 0.0f, 0.0f,

  // right face (+X) CCW
  0.5f, -0.5f, -0.5f, 1, 0, 0, 0.0f, 0.0f,
  0.5f, 0.5f, 0.5f, 1, 0, 0, 1.0f, 1.0f,
  0.5f, -0.5f, 0.5f, 1, 0, 0, 1.0f, 0.0f,
  0.5f, -0.5f, -0.5f, 1, 0, 0, 0.0f, 0.0f,
  0.5f, 0.5f, -0.5f, 1, 0, 0, 0.0f, 1.0f,
  0.5f, 0.5f, 0.5f, 1, 0, 0, 1.0f, 1.0f,

  // bottom face (−Y) CCW
  -0.5f, -0.5f, -0.5f, 0,-1, 0, 0.0f, 0.0f,
  0.5f, -0.5f, -0.5f, 0,-1, 0, 1.0f, 0.0f,
  0.5f, -0.5f, 0.5f, 0,-1, 0, 1.0f, 1.0f,
  -0.5f, -0.5f, -0.5f, 0,-1, 0, 0.0f, 0.0f,
  0.5f, -0.5f, 0.5f, 0,-1, 0, 1.0f, 1.0f,
  -0.5f, -0.5f, 0.5f, 0,-1, 0, 0.0f, 1.0f,

  // top face (+Y) CCW
  -0.5f, 0.5f, -0.5f, 0, 1, 0, 0.0f, 0.0f,
  -0.5f, 0.5f, 0.5f, 0, 1, 0, 1.0f, 0.0f,
  0.5f, 0.5f, 0.5f, 0, 1, 0, 1.0f, 1.0f,
  -0.5f, 0.5f, -0.5f, 0, 1, 0, 0.0f, 0.0f,
  0.5f, 0.5f, 0.5f, 0, 1, 0, 1.0f, 1.0f,
  0.5f, 0.5f, -0.5f, 0, 1, 0, 0.0f, 1.0f,
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
    GL_FALSE, 8 * sizeof(float),
    (void *)0
  );
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(
    1, 3, GL_FLOAT,
    GL_FALSE, 8 * sizeof(float),
    (void *)(3 * sizeof(float))
  );
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(
    2, 2, GL_FLOAT,
    GL_FALSE, 8 * sizeof(float),
    (void *)(6 * sizeof(float))
  );
  glEnableVertexAttribArray(2);

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
      material.base_map[3] > material.alpha_clip_threshold)) {
    if (material.surface_type == MST_OPAQUE) {
      return 1.0f;
    } else {
      return material.base_map[3];
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
  struct Context* context =
    (struct Context *)glfwGetWindowUserPointer(window);

  float aspect = (float)width / (float)height;

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

int main() {
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

  char* vertex_shader_source = NULL;
  char* lit_frag_source = NULL;
  char* unlit_frag_source = NULL;

  if (read_file("src/main.vert", &vertex_shader_source) != 0)
    return -1;

  if (read_file("src/lit.frag", &lit_frag_source) != 0)
    return -1;

  if (read_file("src/unlit.frag", &unlit_frag_source) != 0)
    return -1;

  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1,
    (const char* const*)&vertex_shader_source, NULL);
  glCompileShader(vertex_shader);

  GLuint lit_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(lit_frag_shader, 1,
    (const char* const*)&lit_frag_source, NULL);
  glCompileShader(lit_frag_shader);

  GLuint unlit_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(unlit_frag_shader, 1,
    (const char* const*)&unlit_frag_source, NULL);
  glCompileShader(unlit_frag_shader);

  GLuint lit_program = glCreateProgram();
  glAttachShader(lit_program, vertex_shader);
  glAttachShader(lit_program, lit_frag_shader);
  glLinkProgram(lit_program);

  GLuint unlit_program = glCreateProgram();
  glAttachShader(unlit_program, vertex_shader);
  glAttachShader(unlit_program, unlit_frag_shader);
  glLinkProgram(unlit_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(lit_frag_shader);
  glDeleteShader(unlit_frag_shader);
  free(vertex_shader_source);
  free(lit_frag_source);
  free(unlit_frag_source);

  struct Texture box = load_texture("assets/textures/box.jpg");
  struct Texture knob = load_texture("assets/textures/knob.png");

  struct Material mat1 = {
    .material_kind = MK_LIT,
    .surface_type = MST_TRANSPARENT,
    .render_face = MRF_FRONT,
    .is_alpha_clipping = GL_FALSE,

    .base_map_texture = &knob,
    .smoothness = 0.25,
  };
  rgba_to_vec4(255, 255, 0, 255, &mat1.base_map);

  struct Material mat2 = {
    .material_kind = MK_LIT,
    .surface_type = MST_OPAQUE,
    .render_face = MRF_FRONT,
    .is_alpha_clipping = GL_FALSE,
    .smoothness = 0.5,
  };
  rgba_to_vec4(255, 0, 0, 255, &mat2.base_map);

  struct Entity cube = empty_entity();
  cube.name = "Cube";

  struct Material* cube_materials = malloc(1 * sizeof(struct Material));
  cube_materials[0] = mat1;

  add_component(&cube, (struct Component){
    .kind = CK_TRANSFORM,
    .is_enabled = GL_TRUE,
    .data = &(struct ComponentTransform){
      .position = {0.0f, 0.0f, 0.0f},
      .rotation = {glm_rad(45.0f), 0.0f, 0.0f},
      .scale = {1.0f, 1.0f, 1.0f},
    },
  });

  add_component(&cube, (struct Component){
    .kind = CK_MESH_FILTER,
    .is_enabled = GL_TRUE,
    .data = &(struct ComponentMeshFilter){
      .mesh_kind = MFK_CUBE,
      .vao = cube_vao(),
      .vertex_count = 36,
    },
  });

  add_component(&cube, (struct Component){
    .kind = CK_MESH_RENDERER,
    .is_enabled = GL_TRUE,
    .data = &(struct ComponentMeshRenderer){
      .materials = &cube_materials,
      .material_count = 1,
    },
  });

  struct Entity cube2 = empty_entity();
  cube2.name = "Cube2";

  struct Material* cube2_materials = malloc(1 * sizeof(struct Material));
  cube2_materials[0] = mat2;

  add_component(&cube2, (struct Component){
    .kind = CK_TRANSFORM,
    .is_enabled = GL_TRUE,
    .data = &(struct ComponentTransform){
      .position = {2.0f, 0.0f, 2.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {1.0f, 1.0f, 1.0f},
    },
  });

  add_component(&cube2, (struct Component){
    .kind = CK_MESH_FILTER,
    .is_enabled = GL_TRUE,
    .data = &(struct ComponentMeshFilter){
      .mesh_kind = MFK_CUBE,
      .vao = cube_vao(),
      .vertex_count = 36,
    },
  });

  add_component(&cube2, (struct Component){
    .kind = CK_MESH_RENDERER,
    .is_enabled = GL_TRUE,
    .data = &(struct ComponentMeshRenderer){
      .materials = &cube2_materials,
      .material_count = 1,
    },
  });

  vec3 ambient_color = {0.0f, 0.0f, 1.0f};

  struct Entity light = empty_entity();
  light.name = "Light";
  add_component(&light, (struct Component){
    .kind = CK_TRANSFORM,
    .is_enabled = GL_TRUE,
    .data = &(struct ComponentTransform){
      .position = {0.0f, 100.0f, -50.0f},
      .rotation = {0.0f, 0.0f, 0.0f},
      .scale = {1.0f, 1.0f, 1.0f},
    },
  });

  add_component(&light, (struct Component){
    .kind = CK_LIGHT,
    .is_enabled = GL_TRUE,
    .data = &(struct ComponentLight){
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
    .data = &(struct ComponentCamera){
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
    .data = &(struct ComponentTransform){
      .position = {4.0f, 4.0f, -4.0f},
      .rotation = {glm_rad(45.0f), 0.0f, glm_rad(-45.0f)},
      .scale = {1.0f, 1.0f, 1.0f},
    },
  });

  struct Entity entities[] = {cube2, cube, light, camera};
  size_t entity_count = sizeof(entities) / sizeof(entities[0]);

  float aspect = (float)WIDTH / (float)HEIGHT;
  float near = 0.1f;
  float far = 100.0f;

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
      camera_comp = get_component_by_kind(&entity, CK_CAMERA)
    ) != NULL) {
      cam_transform =
          (struct ComponentTransform *)get_component_by_kind(
              &entity, CK_TRANSFORM)
              ->data;

      camera_data = (struct ComponentCamera *)camera_comp->data;
      break;
    }
  }

  vec3 previous_rot = {0.0f, 0.0f, 1.0f};
  while (!glfwWindowShouldClose(window)) {
    float width = framebuffer_size[0];
    float height = framebuffer_size[1];

    if (!glm_vec3_eqv(cam_transform->rotation, previous_rot)) {
      glm_vec3_copy(cam_transform->rotation, previous_rot);

      vec3 rotated_front;
      glm_vec3_copy(front, rotated_front);

      glm_vec3_rotate(rotated_front,
        cam_transform->rotation[0], (vec3){1.0f, 0.0f, 0.0f});
      glm_vec3_rotate(rotated_front,
        cam_transform->rotation[1], (vec3){0.0f, 1.0f, 0.0f});
      glm_vec3_rotate(rotated_front,
        cam_transform->rotation[2], (vec3){0.0f, 0.0f, 1.0f});

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


    aspect = (vp_w) / (vp_h);
    glm_perspective(camera_data->fovy, aspect,
      camera_data->near, camera_data->far, projection);

    // glUseProgram(shader_program);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, 1);

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

    glm_vec3_add(cam_transform->position, translation,
      cam_transform->position);

    int light_count = 0;
    struct ComponentLight dir_lights[10];

    for (size_t i = 0; i < entity_count; i++) {
      struct Entity entity = entities[i];

      struct Component *light = NULL;
      if ((
        light = get_component_by_kind(&entity, CK_LIGHT)
      ) != NULL) {
        struct ComponentLight *light_comp =
            (struct ComponentLight *)light->data;

        if (light_comp->light_kind == LK_DIRECTIONAL) {
          // struct DirLightData dir_light_data = light_comp->light_data.dir_light;
          dir_lights[light_count] = *light_comp;
          light_count++;
        }
      }
    }

    for (size_t entity_i = 0; entity_i < entity_count; entity_i++) {
      struct Entity entity = entities[entity_i];

      struct Component *mesh_r = NULL;

      if ((
        mesh_r = get_component_by_kind(&entity, CK_MESH_RENDERER)
      ) != NULL) {
        struct ComponentMeshRenderer *mesh_renderer =
            (struct ComponentMeshRenderer *)mesh_r->data;
        if (!mesh_r->is_enabled) continue;

        struct ComponentMeshFilter* mesh_filter =
            (struct ComponentMeshFilter *)get_component_by_kind(
                &entity, CK_MESH_FILTER)
                ->data;
        if (mesh_filter == NULL) continue;

        struct ComponentTransform *transform =
            (struct ComponentTransform *)get_component_by_kind(
                &entity, CK_TRANSFORM)
                ->data;
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

        for (int i = 0; i < mesh_renderer->material_count; i++) {
          struct Material material = materials[i];

          GLuint program;
          if (material.material_kind == MK_LIT) {
            glUseProgram(lit_program);
            program = lit_program;

            for (int i = 0; i < light_count; i++) {
              struct ComponentLight light_comp = dir_lights[i];

              struct DirLightData dir_light_data = light_comp.light_data.dir_light;

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

          GLuint base_map_loc =
              glGetUniformLocation(program, "material.base_map");

          GLuint view_pos_loc =
            glGetUniformLocation(program, "view_pos");

          GLuint num_dir_lights_loc =
              glGetUniformLocation(program, "num_dir_lights");
          glUniform1i(num_dir_lights_loc, light_count);

          GLuint environment_ambient_color_loc =
              glGetUniformLocation(program, "environment_ambient_color");
          glUniform3fv(environment_ambient_color_loc, 1,
            ambient_color);

          GLuint has_base_map_texture_loc =
            glGetUniformLocation(program, "material.has_base_map_texture");

          GLuint material_smoothness_loc =
            glGetUniformLocation(program, "material.smoothness");
          glUniform1f(material_smoothness_loc, material.smoothness);

          glUniform4f(base_map_loc,
              material.base_map[0],
              material.base_map[1],
              material.base_map[2],
              calculate_alpha(material)
          );

          glUniform3fv(view_pos_loc, 1,
            cam_transform->position);

          if (material.base_map_texture != NULL) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(
              GL_TEXTURE_2D,
              material.base_map_texture->id
            );

            GLuint base_map_texture_loc =
              glGetUniformLocation(program, "material.base_map_texture");
            glUniform1i(base_map_texture_loc, 0);

            glUniform1i(has_base_map_texture_loc, 1);
          } else {
            glUniform1i(has_base_map_texture_loc, 0);
          }

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
      }
    }

    glfwSwapBuffers(window);
    glfwPollEvents();

    glfwWaitEventsTimeout(1.0 / 60.0);
  }

  free(framebuffer_size);
  free(cube_materials);
  for (size_t i = 0; i < entity_count; i++) {
    free(entities[i].components);
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}
// clang-format on
