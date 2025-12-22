#include <glad/glad.h>
#include <cglm/cglm.h>

static const vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};
static const vec3 WORLD_RIGHT = {1.0f, 0.0f, 0.0f};
static const vec3 WORLD_FORWARD = {0.0f, 0.0f, 1.0f};

#define POS_Y_AXIS WORLD_UP
#define POS_X_AXIS WORLD_RIGHT
#define POS_Z_AXIS WORLD_FORWARD

#define GRAVITY_VAL 9.81f

static const vec3 GRAVITY = {0.0f, -GRAVITY_VAL, 0.0f};

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
  CK_RIGIDBODY,
};

struct Component {
  enum ComponentKind kind;
  GLboolean is_enabled;

  union {
    struct ComponentTransform* transform;
    struct ComponentMeshFilter* mesh_filter;
    struct ComponentMeshRenderer* mesh_renderer;
    struct ComponentLight* light;
    struct ComponentCamera* camera;
    struct ComponentRigidbody* rigidbody;
  } data;
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

enum MaterialShader {
  MS_LIT,
  MS_UNLIT,
};

enum MaterialSurfaceType {
  MST_OPAQUE = 0,
  MST_TRANSPARENT = 1,
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

struct ColoredTexture {
  struct Texture* texture;
  vec4 color;
};

struct Material {
  enum MaterialShader material_shader;

  // surface options
  enum MaterialSurfaceType surface_type;

  enum MaterialRenderFace render_face;

  GLboolean is_preserve_specular_highlights;

  // surface inputs
  struct ColoredTexture* base_map_texture;

  vec3 specular_map;
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

struct ForceGenerator {
  void (*update_force)(
    struct ComponentRigidbody* rigidbody,
    float delta_time,
    void* generator_data
  );

  void* generator_data;
};

struct ComponentRigidbody {
  float mass;
  GLboolean is_kinematic;

  float linear_damping;

  vec3 velocity;
  vec3 acceleration;

  vec3 force_accumulator;

  struct ForceGenerator* force_generators;
  int force_generator_count;
};

void _gravity_generator_update_force(
  struct ComponentRigidbody* rigidbody,
  float delta_time,
  void* generator_data
) {
  (void)delta_time;

  vec3 gravity;
  glm_vec3_copy(*(vec3*)generator_data, gravity);

  vec3 force;
  glm_vec3_scale(gravity, rigidbody->mass, force);

  glm_vec3_add(rigidbody->force_accumulator, force, rigidbody->force_accumulator);
}

static const struct ForceGenerator GRAVITY_GENERATOR = (struct ForceGenerator){
  .update_force = &_gravity_generator_update_force,
  .generator_data = (void*)GRAVITY,
};

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

