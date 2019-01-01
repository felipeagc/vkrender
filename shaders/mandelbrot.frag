#version 450

layout (location = 0) in vec2 texCoords;

layout (set = 0, binding = 0) uniform sampler2D texture0;

layout (location = 0) out vec4 outColor;

double map(double x, double a, double b, double c, double d) {
  return (x - a) * ((d-c)/(b-a)) + c;
}

dvec2 f(dvec2 z, dvec2 pos) {
  double a = z.x;
  double b = z.y;
  return dvec2(a*a - b*b, 2*a*b) + pos;
}

dvec3 hsv2rgb(dvec3 c) {
  dvec4 K = dvec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  dvec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

layout(push_constant) uniform PushConstant {
  double time;
  dvec2 camPos;
} pc;

const uint MAX_ITERATIONS = 100;

void main() {
  double zoom = 1.0 / (pc.time / 0.1);

  dvec2 c = dvec2(0.0);

  c.x = (texCoords.x - 0.5) * zoom + pc.camPos.y;
  c.y = (texCoords.y - 0.5) * zoom + pc.camPos.x;

  dvec2 z = dvec2(0.0);
  uint n;
  for (n = 0; n < MAX_ITERATIONS; n++) {
    z = f(z, c);

    double dist = distance(z, dvec2(0.0));
    if (dist > 2.0) break;
  }

  double dist = distance(z, dvec2(0.0));

  double nsmooth = n + 1 - log(log(float(dist))) / log(2.0);

  dvec3 hsv = dvec3(0.0);
  hsv.x = (pc.time / 10.0) + map(nsmooth, 0, double(MAX_ITERATIONS), 0.0, 1.0);
  hsv.y = 1.0;
  hsv.z = 1.0;
  outColor = vec4(vec3(hsv2rgb(hsv)), 1.0);
}
