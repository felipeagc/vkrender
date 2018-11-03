# Vkrender

## TODO
- [x] Compile GLSL shaders using glslang
- [x] Free descriptor sets after destruction of resources
- [x] Camera options
- [x] Figure out a way to scale a whole model (probably a member
      matrix for GltfModel that multiplies before passing to GPU)
- [x] Load glTF models
- [x] Figure out UV flipping of GltfModels
- [x] Add google test as a git submodule
- [x] Add glm as a git submodule
- [ ] Document as much as possible
- [x] Make more flexible DescriptorManager with customizable pools and sets
- [ ] Default descriptor sets for assets that don't have defined properties (like textures)
- [x] Small vector class (for usage in cases where an allocation would be done each frame)
- [x] Multisampling
- [x] Make multisampling configurable
- [ ] Figure out a way to pass more parameters to the pipeline on creation
- [ ] Model buffers whould be on the GPU
- [x] Figure out Unique with a move constructor
- [ ] Make a better abstraction for updating descriptor sets
- [ ] First person camera
- [ ] Imgui
- [ ] Imgui shader uniform menu
- [ ] Shader live refresh

## Shaders

### Vertex format
```glsl
vec3 pos;
vec3 normal;
vec2 texCoords;
```

### Descriptor set 0
For the camera's stuff
```glsl
layout (set = 0, binding = 0) uniform CameraUniform {
    mat4 view;
    mat4 proj;
} camera_ubo;
```

### Descriptor set 1
For the material's stuff
```glsl
layout (set = 1, binding = 0) uniform sampler2D albedo;
layout (set = 1, binding = 1) uniform MaterialUniform {
    vec4 baseColorFactor;
} material;
```

### Descriptor set 2
For the model's mesh stuff
```glsl
layout (set = 2, binding = 0) uniform ModelUniform {
  mat4 model;
} model_ubo;
```

### Descriptor set 3
Lighting stuff
```glsl
layout(set = 3, binding = 0) uniform LightingUniform {
    vec4 lightColor; // vec4 because of padding
} lighting;
```

### Descriptor sets 3 and beyond
For user-defined stuff



