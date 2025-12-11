#include <stdio.h>
#include <math.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <GLFW/glfw3.h>

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
  "uniform vec3 light_pos;\n"
  "uniform vec3 view_pos;\n"
  "void main()\n"
  "{\n"
  "   float ambient_strength = 0.8;\n"
  "   vec3 ambient = ambient_strength * light_color;\n"
  "   vec3 norm = normalize(Normal);\n"
  "   vec3 light_dir = normalize(light_pos - FragPos);\n"
  "   float diff = max(dot(norm, light_dir), 0.0);\n"
  "   vec3 diffuse = diff * 1.25 * light_color;\n"
  "   float specular_strength = 0.4;\n"
  "   vec3 view_dir = normalize(view_pos - FragPos);\n"
  "   vec3 reflect_dir = reflect(-light_dir, norm);\n"
  "   float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 256);\n"
  "   vec3 specular = specular_strength * spec * light_color;\n"
  "   vec3 result = (ambient + diffuse + specular) * object_color;\n"
  "   FragColor = vec4(result, 1.0);\n"
  "}\n\0";

int generate_cube_vao() {
  static const float vertices[] = {
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
  };

  int vao, vbo;

  glGenVertexArrays(1, (GLuint *)&vao);
  glGenBuffers(1, (GLuint *)&vbo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
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

  GLFWwindow *window = glfwCreateWindow(800, 600, "OpenGL Window", NULL, NULL);

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

  int vao = generate_cube_vao();

  mat4 model_matrix;
  glm_mat4_identity(model_matrix);
  glm_translate(model_matrix, (vec3){0.0f, 0.0f, -25.0f});
  glm_scale(model_matrix, (vec3){4.0f, 2.0f, 4.0f});

  float aspect = 800.0f / 600.0f;
  float near = 0.1f;
  float far  = 100.0f;

  mat4 projection;
  glm_perspective(glm_rad(45.0f), aspect, near, far, projection);

  mat4 view_matrix;

  vec3 front = {0.0f, 0.0f, -1.0f};
  glm_normalize(front);

  vec3 right;
  glm_cross(front, (vec3){0.0f, 1.0f, 0.0f}, right);
  glm_normalize(right);

  vec3 up;
  glm_cross(right, front, up);
  glm_normalize(up);

  glm_look((vec3){0.0f, 0.0f, 0.0f}, front, up, view_matrix);

  float angle = 0.02f;

  glEnable(GL_DEPTH_TEST);

  float camera_pos[] = {0.0f, 0.0f, 0.0f};

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    GLuint model_loc = glGetUniformLocation(shaderProgram, "model");
    GLuint proj_loc = glGetUniformLocation(shaderProgram, "projection");
    GLuint view_loc = glGetUniformLocation(shaderProgram, "view");

    glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float*)model_matrix);
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)projection);
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)view_matrix);

    GLuint object_color_loc = glGetUniformLocation(shaderProgram, "object_color");
    GLuint light_color_loc = glGetUniformLocation(shaderProgram, "light_color");
    GLuint light_pos_loc = glGetUniformLocation(shaderProgram, "light_pos");
    GLuint view_pos_loc = glGetUniformLocation(shaderProgram, "view_pos");

    glUniform3f(object_color_loc, 1.0f, 1.0f, 1.0f);
    glUniform3f(light_color_loc, 1.0f, 1.0f, 1.0f);
    glUniform3f(light_pos_loc, 0.0f, 100.0f, -50.0f);
    glUniform3fv(view_pos_loc, 1, camera_pos);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, 1);

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(up, 0.1f, translation);

      glm_translate(view_matrix, translation);
      camera_pos[1] -= 0.1f;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(up, -0.1f, translation);

      glm_translate(view_matrix, translation);
      camera_pos[1] += 0.1f;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(front, -1.0f, translation);

      glm_translate(view_matrix, translation);
      camera_pos[2] += 0.1f;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(front, 1.0f, translation);

      glm_translate(view_matrix, translation);
      camera_pos[2] -= 0.1f;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(right, 0.1f, translation);

      glm_translate(view_matrix, translation);
      camera_pos[0] += 0.1f;
    }
 
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
      vec3 translation;
      glm_vec3_scale(right, -0.1f, translation);

      glm_translate(view_matrix, translation);
      camera_pos[0] -= 0.1f;
    }

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glfwSwapBuffers(window);
    glfwPollEvents();

    glfwWaitEventsTimeout(1.0 / 60.0);
  }

  glfwDestroyWindow(window);
  glfwTerminate();
}
