# VKRender

## TODO

### Rewrite
- [x] General purpose global allocator
- [x] Decide on how to handle entities
- [x] Basic shape rendering
- [ ] Light billboard rendering
- [ ] Gltf models with cgltf

### Shader compilation
- [x] Compile GLSL shaders using glslang
- [x] Shader live refresh

### Filesystem
- [ ] File system utility functions
- [ ] Configurable assets path

### GLTF models
- [ ] Load glTF models
- [ ] Figure out UV flipping of GltfModels
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

### Utilities
- [ ] Noise

### Window
- [ ] Better event system

### Libraries
- [ ] Use volk
- [ ] Add SDL2 as a submodule

### Assets
- [ ] Multi-threaded asset loading

### Portability
- [ ] Windows file watcher
- [ ] `#define`s for dllexport (windows)

### Misc
- [x] Make standalone skybox baker
- [x] Object picking (use OBB testing with bouding box provided by gltf)
- [ ] Object picking with a color buffer
- [ ] Transformation gizmos
- [ ] Selected object outline (with stencil buffer + separate pipeline with `lineWidth = 2.0f`)
- [x] Fullscreen post process shader
- [ ] Document as much as possible
