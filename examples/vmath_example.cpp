#include <renderer/glm.hpp>
#include <stdio.h>
#include <vmath/vmath.h>

void print_matrix(float *mat) {
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      printf("%f\t", mat[i * 4 + j]);
    }
    printf("\n");
  }
}

void print_quat(float *quat) {
  for (int i = 0; i < 4; i++) {
    printf("%f\t", quat[i]);
  }
  printf("\n");
}

int main() {
  // mat4_t v_mat =
  //     mat4_look_at({0.0, 0.0, 0.0}, {3.0, 3.0, 3.0}, {0.0, 1.0, 0.0});

  // glm::mat4 g_mat =
  //     glm::lookAt(glm::vec3{0.0, 0.0, 0.0}, {3.0, 3.0, 3.0}, {0.0, 1.0,
  //     0.0});

  // float *v_f = &v_mat.elems[0];
  // float *g_f = glm::value_ptr(g_mat);

  // print_matrix(v_f);
  // printf("\n");
  // print_matrix(g_f);

  // for (int i = 0; i < 16; i++) {
  //   printf("%f\t %f\n", v_f[i], g_f[i]);
  //   assert(v_f[i] == g_f[i]);
  // }

  quat_t v_quat = quat_look_at({3.0, 3.0, 3.0}, {0.0, 1.0, 0.0});

  glm::quat g_quat =
      glm::quatLookAt(glm::vec3(3.0, 3.0, 3.0), glm::vec3(0.0, 1.0, 0.0));

  print_quat((float *)&v_quat);
  print_quat((float *)&g_quat);

  assert(v_quat.x == g_quat.x);
  assert(v_quat.y == g_quat.y);
  assert(v_quat.z == g_quat.z);
  assert(v_quat.w == g_quat.w);

  return 0;
}
