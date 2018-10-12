# Vkrender

## TODO
- [x] Compile GLSL shaders using glslang
- [x] Free descriptor sets after destruction of resources
- [x] Camera options
- [ ] Figure out a way to scale a whole model (probably a member
      matrix for GltfModel that multiplies before passing to GPU)
- [x] Load glTF models
- [ ] Make more flexible DescriptorManager (with unordered_map)
- [ ] Default descriptor sets for assets that don't have defined properties (like textures)
- [ ] Small vector class (for usage in cases where an allocation would be done each frame)
  https://dendibakh.github.io/blog/2016/11/25/Small_size_optimization
- [ ] Model buffers whould be on the GPU
- [ ] Figure out Unique with a move constructor
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
- Uniform buffer containing the view and projection matrices (binding = 0)

### Descriptor set 1
For the material's stuff
- Albedo texture (binding = 0)

### Descriptor set 2
For the model's stuff
- Model matrix (binding = 0)

### Descriptor sets 3 and beyond
For user-defined stuff
