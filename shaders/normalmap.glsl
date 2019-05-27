#ifndef NORMALMAP_GLSL
#define NORMALMAP_GLSL

vec3 normal_map(
    vec3 normal,
    vec3 world_pos,
    Material material,
    sampler2D normal_texture,
    vec2 tex_coords) {
  if (material.has_normal_texture == 0.0f) {
    return normalize(normal);
  }
  // Retrieve the tangent space matrix
  vec3 pos_dx = dFdx(world_pos);
  vec3 pos_dy = dFdy(world_pos);
  vec3 tex_dx = dFdx(vec3(tex_coords, 0.0));
  vec3 tex_dy = dFdy(vec3(tex_coords, 0.0));
  vec3 t = (tex_dy.t * pos_dx - tex_dx.t * pos_dy) / (tex_dx.s * tex_dy.t - tex_dy.s * tex_dx.t);

  vec3 ng = normalize(normal);

  t = normalize(t - ng * dot(ng, t));
  vec3 b = normalize(cross(ng, t));
  mat3 tbn = mat3(t, b, ng);

  // If material has normal texture
  vec3 n = texture(normal_texture, tex_coords).rgb;
  n = normalize(tbn * (2.0 * n - 1.0));

  return n;
}

#endif
