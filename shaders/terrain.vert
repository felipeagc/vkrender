#version 450

#include "common.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex_coords;

out gl_PerVertex {
  vec4 gl_Position;
};

layout (push_constant) uniform PushConstant {
  float time;
} pc;

layout (set = 0, binding = 0) uniform CameraUniform {
  Camera camera;
};

layout (set = 2, binding = 0) uniform LocalModelUniform {
  Model local_model;
};

layout (set = 3, binding = 0) uniform ModelUniform {
  Model model;
};

// layout (set = 5, binding = 0) uniform sampler2D heightmap;

layout (location = 0) out vec2 tex_coords0;
layout (location = 1) out vec3 world_pos;
layout (location = 2) out vec3 normal0;
layout (location = 3) out vec3 camera_pos;
layout (location = 4) out float vertex_height;

// Simplex 2D noise
//
vec3 permute(vec3 x) { return mod(((x*34.0)+1.0)*x, 289.0); }

float snoise(vec2 v){
  const vec4 C = vec4(0.211324865405187, 0.366025403784439,
           -0.577350269189626, 0.024390243902439);
  vec2 i  = floor(v + dot(v, C.yy) );
  vec2 x0 = v -   i + dot(i, C.xx);
  vec2 i1;
  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;
  i = mod(i, 289.0);
  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
  + i.x + vec3(0.0, i1.x, 1.0 ));
  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy),
    dot(x12.zw,x12.zw)), 0.0);
  m = m*m ;
  m = m*m ;
  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;
  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
  vec3 g;
  g.x  = a0.x  * x0.x  + h.x  * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}

const float mult = 10.0f;
const float hmult = 1.0f;

vec3 get_pos(vec3 from) {
  vec3 new_pos = from;

  new_pos.xz += vec2(
      snoise(new_pos.xz + 5813.0 + pc.time/10.0)/50.0,
      snoise(new_pos.xz + 2311.0 + pc.time/10.0)/50.0);
  new_pos.y = snoise((new_pos.xz + vec2(pc.time/10.0, sin(pc.time/2.0)/10.0)) * mult) * hmult;

  return new_pos;
}

void main() {
  tex_coords0 = tex_coords;

  mat4 model0 = model.matrix * local_model.matrix;

  vec3 new_pos = get_pos(pos);
  vertex_height = (new_pos.y / hmult) + 1.0;

  float left = get_pos(new_pos.xyz + vec3(-1.0, 0.0, 0.0)).y;
  float right = get_pos(new_pos.xyz + vec3(1.0, 0.0, 0.0)).y;
  float top = get_pos(new_pos.xyz + vec3(0.0, 0.0, -1.0)).y;
  float bottom = get_pos(new_pos.xyz + vec3(0.0, 0.0, 1.0)).y;

  vec3 new_normal = normalize(vec3(
      left - right,
      2.0f,
      top - bottom));

  vec4 loc_pos = model0 * vec4(new_pos, 1.0);
  normal0 = mat3(transpose(inverse(model0))) * new_normal;

  world_pos = loc_pos.xyz / loc_pos.w;

  camera_pos = camera.pos.xyz;

  gl_Position = camera.proj * camera.view * vec4(world_pos, 1.0);
}
