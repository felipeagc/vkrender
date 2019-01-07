# VKRender

## TODO
### Shader compilation
- [x] Compile GLSL shaders using glslang
- [x] Shader live refresh

### Camera
- [x] Camera options
- [x] Use quaternion for camera
- [x] First person camera

### Pipeline
- [x] Figure out a way to pass more parameters to the pipeline on creation
- [x] Multisampling
- [x] Make multisampling configurable

### Descriptor sets
- [ ] Make a better abstraction for updating descriptor sets

### GLTF models
- [x] Load glTF models
- [x] Figure out UV flipping of GltfModels
- [ ] Animation
- [ ] Alpha blending

### Graphics
- [ ] Bloom
- [ ] Shadow mapping
- [ ] Forward+ lighting
- [ ] Global illumination

### PBR
- [x] Emissive maps
- [x] Occlusion maps
- [ ] Normal maps
- [ ] Pass radiance mipmap count to shader at runtime

### Mesh generation
- [x] Shape asset
- [ ] MeshComponent class

### Utilities
- [ ] Noise

### Lighting
- [x] Multiple lights
- [x] Add directional light (sun)

### Window
- [ ] Better event system

### Libraries
- [ ] Figure out how to add vulkan SDK as a submodule
- [ ] Add SDL2 as a git submodule

### Assets
- [x] Make AssetManager thread safe
- [x] Multi-threaded asset loading

### ECS
- [ ] Better storage solution (packed array -- arena allocator)
- [x] More organized system abstraction

### FTL
- [ ] Stack allocator with regions
- [ ] String
- [ ] String builder

### Misc
- [ ] Make standalone skybox baker
- [x] Object picking (use OBB testing with bouding box provided by gltf)
- [ ] Selected object outline (with stencil buffer + separate pipeline with `lineWidth = 2.0f`)
- [x] Fullscreen post process shader
- [ ] Document as much as possible


## Shaders
### Vertex format
```glsl
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;
```

### Camera descriptor set layout
```glsl
layout (set = 0, binding = 0) uniform CameraUniform {
    mat4 view;
    mat4 proj;
    vec4 pos;
} camera;
```

### Material descriptor set layout
For the material's stuff
```glsl
layout (set = 1, binding = 0) uniform sampler2D albedoTexture;
layout (set = 1, binding = 1) uniform sampler2D normalTexture;
layout (set = 1, binding = 2) uniform sampler2D metallicRoughnessTexture;
layout (set = 1, binding = 3) uniform sampler2D occlusionTexture;
layout (set = 1, binding = 4) uniform sampler2D emissiveTexture;
```

### Mesh descriptor set layout
```glsl
layout (set = 2, binding = 0) uniform MeshUniform {
    mat4 matrix;
} mesh;
```

### Model descriptor set layout
```glsl
layout (set = 3, binding = 0) uniform ModelUniform {
    mat4 matrix;
} model;
```

### Lighting descriptor set layout
```glsl
layout(set = 4, binding = 0) uniform LightingUniform {
    vec4 color;
    vec4 pos;
} lighting;
```

### Environment descriptor set layout
```glsl
layout (set = 5, binding = 0) uniform EnvironmentUniform {
    float exposure;
} environment;

layout (set = 5, binding = 1) uniform samplerCube envMap;
layout (set = 5, binding = 2) uniform samplerCube irradianceMap;
layout (set = 5, binding = 3) uniform samplerCube radianceMap;
layout (set = 5, binding = 4) uniform sampler2D brdfLut;
```
