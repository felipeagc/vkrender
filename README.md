# VKRender

## TODO
### Shader compilation
- [x] Compile GLSL shaders using glslang
- [x] Shader live refresh

### Camera
- [x] Camera options
- [x] Use quaternion for camera
- [ ] First person camera
- [ ] Use push constants for the camera

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

### Window
- [ ] Better event system
- [ ] Input handling
- [ ] Get state of a key

### Libraries
- [ ] Figure out how to add vulkan SDK as a submodule

### Assets
- [x] Asset management system
- [ ] Clearly specify which types are assets (maybe naming things like TextureAsset, GltfModelAsset, etc)
- [x] Json files for assets as a way of passing additional parameters (maybe treat this as just an option)
      Maybe handle this in the asset constructor itself.
- [x] More flexible asset manager (use constructor instead of json for extra parameters)
- [x] Allow for arbitrary assets
- [ ] Multi-threaded asset loading

### ECS
- [x] Basic ECS
- [ ] Better storage solution (packed array)

#### ECS Components
- [x] Transform component
- [x] glTF model component
- [x] Light component
- [x] Billboard component

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

### Camera descriptor set layout
```glsl
layout (set = 0, binding = 0) uniform CameraUniform {
    mat4 view;
    mat4 proj;
} camera;
```

### Material descriptor set layout
For the material's stuff
```glsl
layout (set = 1, binding = 0) uniform MaterialUniform {
    vec4 albedo;
    float metallic;
    float roughness;
    float ao;
} material;

layout (set = 1, binding = 1) uniform sampler2D albedoTexture;
layout (set = 1, binding = 2) uniform sampler2D metallicRoughnessTexture;
```

### Mesh descriptor set layout
```glsl
layout (set = 2, binding = 0) uniform ModelUniform {
    mat4 model;
} model;
```

### Lighting descriptor set layout
```glsl
layout(set = 3, binding = 0) uniform LightingUniform {
    vec4 color;
    vec4 pos;
} lighting;
```

### Environment descriptor set layout
```glsl
layout (set = 4, binding = 0) uniform EnvironmentUniform {
    float exposure;
} environment;

layout (set = 4, binding = 1) uniform samplerCube envMap;
layout (set = 4, binding = 2) uniform samplerCube irradianceMap;
layout (set = 4, binding = 3) uniform samplerCube radianceMap;
layout (set = 4, binding = 4) uniform sampler2D brdfLut;
```
