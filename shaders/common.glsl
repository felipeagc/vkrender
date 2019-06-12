#ifndef COMMON_GLSL
#define COMMON_GLSL

#define MAX_LIGHTS 20

const float PI = 3.14159265359;

struct Light {
  vec4 pos;
  vec4 color;
};

struct Camera {
  mat4 view;
  mat4 proj;
  vec4 pos;
};

struct Environment {
  vec3 sun_direction;
  float exposure;

  vec3 sun_color;
  float sun_intensity;

  float radiance_mip_levels;
  uint light_count;

  float dummy1;
  float dummy2;

  Light lights[MAX_LIGHTS];
};

struct Material {
  vec4 base_color;
  float metallic;
  float roughness;
  vec4 emissive;
  uint has_normal_texture;
};

struct Model {
  mat4 matrix;
};

#endif
