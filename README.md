# VKRender

## TODO
### Template library
- [x] Move it to its own folder and namespace (fstl)
- [x] fixed_vector (for usage in cases where an allocation would be done each frame)
- [x] unique (for optional RAII with a destroy() method)
- [x] array_proxy (for passing parameters that could be converted to an array)

### Shader compilation
- [x] Compile GLSL shaders using glslang
- [ ] Shader live refresh

### Camera
- [x] Camera options
- [ ] Use quaternion for camera
- [ ] First person camera

### Pipeline
- [ ] Figure out a way to pass more parameters to the pipeline on creation
- [x] Multisampling
- [x] Make multisampling configurable

### Descriptor sets
- [x] Free descriptor sets after destruction of resources
- [x] Make more flexible DescriptorManager with customizable pools and sets
- [ ] Default bound descriptor sets for assets that don't have defined
      properties (like textures) -- maybe not needed
- [ ] Make a better abstraction for updating descriptor sets

### GLTF models
- [x] Figure out a way to scale a whole model (probably a member
matrix for GltfModel that multiplies before passing to GPU)
- [x] Load glTF models
- [x] Figure out UV flipping of GltfModels

### Mesh generation
- [ ] Mesh class

### Utilities
- [ ] Noise

### Lighting
- [ ] Figure out phong lighting
- [ ] Multiple lights

### Libraries
- [x] Add google test as a git submodule
- [x] Add glm as a git submodule
- [ ] Figure out how to add vulkan SDK as a submodule

### Misc
- [ ] Document as much as possible
- [ ] Imgui
- [ ] Imgui shader uniform menu


## Shaders
### Vertex format
```glsl
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
```

### Descriptor set 0
For the camera's stuff
```glsl
layout (set = 0, binding = 0) uniform CameraUniform {
    mat4 view;
    mat4 proj;
} camera;
```

### Descriptor set 1
For the material's stuff
```glsl
layout (set = 1, binding = 0) uniform sampler2D albedo;

layout (set = 1, binding = 1) uniform MaterialUniform {
    vec4 color;
} material;
```

### Descriptor set 2
For the model's mesh stuff
```glsl
layout (set = 2, binding = 0) uniform ModelUniform {
    mat4 model;
} model;
```

### Descriptor set 3
Lighting stuff
```glsl
layout(set = 3, binding = 0) uniform LightingUniform {
    vec4 color;
    vec4 pos;
} lighting;
```

### Descriptor sets 3 and beyond
For user-defined stuff
