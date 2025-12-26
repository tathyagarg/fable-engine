#include <glad/glad.h>
#include <cglm/cglm.h>

/*
 * World axis definitions
 * */
static const vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};
static const vec3 WORLD_RIGHT = {1.0f, 0.0f, 0.0f};
static const vec3 WORLD_FORWARD = {0.0f, 0.0f, 1.0f};

#define POS_Y_AXIS WORLD_UP
#define POS_X_AXIS WORLD_RIGHT
#define POS_Z_AXIS WORLD_FORWARD

#define GRAVITY 9.81f
// #define GRAVITY 0.0f

static const vec3 GRAVITY_VEC = {0.0f, -GRAVITY, 0.0f};

#define DISPLAY_VEC3(vec) \
  printf("%s: (%f, %f, %f)\n", #vec, (vec)[0], (vec)[1], (vec)[2]);

/*
 * Context struct holds global rendering context information
 * This data is passed to GLFW window user pointer for access in callbacks
 * */
struct Context {
  mat4 *projection;
  int** framebuffer_size;
};

struct Entity {
  /*
   * This will come of use eventually.
   * */
  char *name;

  /*
   * Array of components attached to this entity
   * We use a dynamic array for flexibility
   * Everytime the component count exceeds the reserved count,
   * we reallocate the array with double the previous size
   * */
  struct Component *components;
  unsigned int component_count;
  unsigned int reserved_components;
};

#define ROTATION_VEC_DEG(x, y, z) {glm_rad(x), glm_rad(y), glm_rad(z)}

struct Component {
  /*
   * One ComponentKind per component to identify its type
   * */
  enum ComponentKind {
    CK_TRANSFORM,
    CK_MESH_FILTER,
    CK_MESH_RENDERER,
    CK_MATERIAL,
    CK_LIGHT,
    CK_CAMERA,
    CK_RIGIDBODY,
    CK_BOX_COLLIDER,
  } kind;

  GLboolean is_enabled;

  union {
    struct ComponentTransform {
      /*
       * Position and scale both have the same units
       * */
      vec3 position;

      /*
       * Rotations are stored in radians,
       * A rotation vector can be created from degrees using the
       * ROTATION_VEC_DEG macro:
       *  vec3 rot_rad = ROTATION_VEC_DEG(45.0f, 0.0f, 90.0f);
       * */
      vec3 rotation;

      vec3 scale;
    }* transform;

    struct ComponentMeshFilter {
      /*
       * Default mesh kinds
       * Each mesh kind (except CUSTOM) has a predefined set of vertices
       * - MFK_CUBE: CUBE_VERTICES
       * - MFK_SPHERE: TODO
       * - MFK_PLANE: TODO
       * */
      enum MeshFilterKind {
        MFK_CUBE,
        MFK_SPHERE,
        MFK_PLANE,
        MFK_CUSTOM,
      } mesh_kind;

      /*
       * The vertex array object (VAO) holding the mesh data
       * For the default mesh kinds, these VAOs are generated internally
       * */
      GLuint vao;

      /*
       * Number of vertices in the mesh
       * For the default mesh kinds, these counts are predefined
       * */
      unsigned int vertex_count;
    }* mesh_filter;

    struct ComponentMeshRenderer {
      /*
       * Array of materials applied to this mesh renderer
       * */
      struct Material** materials;
      unsigned int material_count;
    }* mesh_renderer;

    struct ComponentLight {
      enum LightKind {
        LK_DIRECTIONAL,
        LK_POINT,
        LK_SPOT,
      } light_kind;

      union LightData {
        struct DirLightData {
          vec3 direction;

          vec3 ambient;
          vec3 diffuse;
          vec3 specular;
        } dir_light;
      } light_data;

      vec3 color;

      float intensity;
    }* light;

    struct ComponentCamera {
      float fovy;
      float near;
      float far;

      GLboolean is_perspective;

      GLboolean is_display_to_screen;
      // Texture target;

      float viewport_rect[4];

      enum CameraBackgroundKind {
        CBK_COLOR,
        CBK_SKYBOX,
      } background_kind;

      union CameraBackgroundData {
        vec4 color;
        // struct Texture skybox;
      } background_data;
    }* camera;

    struct ComponentRigidbody {
      float mass;
      GLboolean is_kinematic;

      float linear_damping;

      vec3 velocity;
      vec3 acceleration;

      vec3 angular_vel;
      vec3 angular_acc;

      vec3 force_acc;
      vec3 torque_acc;

      struct ForceGenerator* force_generators;
      int force_generator_count;

      struct TorqueGenerator* torque_generators;
      int torque_generator_count;
    }* rigidbody;

    struct ComponentBoxCollider {
      vec3 center;
      vec3 size;
    }* box_collider;
  } data;
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
  enum MaterialShader {
    MS_LIT,
    MS_UNLIT,
  } material_shader;

  // surface options
  enum MaterialSurfaceType {
    MST_OPAQUE = 0,
    MST_TRANSPARENT = 1,
  } surface_type;

  enum MaterialRenderFace {
    MRF_FRONT,
    MRF_BACK,
    MRF_DOUBLE,
  } render_face;

  GLboolean is_preserve_specular_highlights;

  // surface inputs
  struct ColoredTexture* base_map_texture;

  vec3 specular_map;
  float smoothness;

  GLboolean is_alpha_clipping;
  float alpha_clip_threshold;
};

struct ForceGenerator {
  void (*update_force)(
    struct ComponentRigidbody* rigidbody,
    float delta_time,
    void* generator_data
  );

  void* generator_data;
};

struct TorqueGenerator {
  void (*update_torque)(
    struct ComponentRigidbody* rigidbody,
    float delta_time,
    void* generator_data
  );

  void* generator_data;
};

struct BasicTorqueGeneratorData {
  vec3* r;
  vec3* force;
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

  glm_vec3_add(rigidbody->force_acc, force, rigidbody->force_acc);
}

static const struct ForceGenerator GRAVITY_GENERATOR = (struct ForceGenerator){
  .update_force = &_gravity_generator_update_force,
  .generator_data = (void*)GRAVITY_VEC,
};

void _basic_torque_generator_update_torque(
  struct ComponentRigidbody* rigidbody,
  float delta_time,
  void* generator_data
) {
  (void)delta_time;

  struct BasicTorqueGeneratorData* data =
    (struct BasicTorqueGeneratorData*)generator_data;

  if (data->r == NULL || data->force == NULL)
    return;

  DISPLAY_VEC3(*(data->r));

  // vec3 r;
  // glm_vec3_sub(*(data->contact_point), rigidbody->velocity, r);

  vec3 torque;
  glm_vec3_cross(*(data->r), *(data->force), torque);

  glm_vec3_add(rigidbody->torque_acc, torque, rigidbody->torque_acc);
}

static const struct TorqueGenerator BASIC_TORQUE_GENERATOR = (struct TorqueGenerator){
  .update_torque = &_basic_torque_generator_update_torque,
  .generator_data = NULL,
};

static const unsigned int CUBE_VERTEX_COUNT = 36;
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

GLuint cube_vao(void) {
  GLuint vao, vbo;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(CUBE_VERTICES),
    CUBE_VERTICES,
    GL_STATIC_DRAW
  );

  // position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // normal attribute
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // texture coord attribute
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return vao;
}

struct CollisionManifold {
  GLboolean is_colliding;

  vec3 normal;
  float penetration_depth;

  vec3 contact_point;
};
