# VKRender

## Code style
- Avoid using templates when unnecessary
- `m_` prefix for class member variables
- No inheritance
- No virtual member functions

## TODO
### Buffers
- [ ] Create `Buffer` class
- [ ] Remove `Buffers` struct

### Shader compilation
- [x] Compile GLSL shaders using glslang
- [ ] Shader live refresh

### Camera
- [x] Rewrite camera class
- [x] Camera options
- [x] Use quaternion for camera
- [ ] First person camera

### Pipeline
- [x] Figure out a way to pass more parameters to the pipeline on creation
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
- [ ] Animation

### Mesh generation
- [ ] Mesh class

### Utilities
- [ ] Noise

### Lighting
- [ ] Figure out blinn-phong lighting
- [x] Multiple lights
- [ ] Light as a component

### Window
- [ ] Better event system
- [ ] Input handling
- [ ] Get state of a key

### Libraries
- [x] Add google test as a git submodule
- [x] Add glm as a git submodule
- [ ] Figure out how to add vulkan SDK as a submodule
- [x] Add rapidjson

### Assets
- [x] Asset management system
- [x] Separate GltfModel into GltfModel and GltfModelInstance
- [ ] Clearly specify which types are assets (maybe naming things like TextureAsset, GltfModelAsset, etc)
- [x] Json files for assets as a way of passing additional parameters (maybe treat this as just an option)
      Maybe handle this in the asset constructor itself.
- [ ] More flexible asset manager (use constructor instead of json for extra parameters)
- [ ] Allow for arbitrary assets
- [ ] Use templates even more
- [ ] Multi-threaded asset loading

### ECS
- [x] Basic ECS

#### ECS Components
- [x] Transform component
- [ ] Remove useless move constructors
  - [x] GltfModelInstance
  - [x] Camera
  - [ ] Billboard

### Misc
- [ ] Specify which types can be copied (maybe with a Handle suffix)
      Also applies to asset types?
- [x] Billboards
- [ ] Object picking
- [ ] Selected object outline (with stencil buffer + separate pipeline with `lineWidth = 2.0f`)
- [ ] Fullscreen post process shader
- [ ] Document as much as possible
- [x] Imgui
- [ ] Imgui shader uniform menu

## Assets
Async loading:
- diskLoad();
- gpuLoad();


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
