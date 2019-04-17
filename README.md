# VKRender

[![Build status](https://ci.appveyor.com/api/projects/status/iu63edk658bwjxms?svg=true)](https://ci.appveyor.com/project/felipeagc/vkrender)

![](https://user-images.githubusercontent.com/17355488/55602406-c74e5f80-573b-11e9-83c4-772f1abc79d6.png)

## TODO

### Rewrite
- [x] General purpose global allocator
- [x] Decide on how to handle entities
- [x] Basic shape rendering
- [ ] Light billboard rendering
- [x] Gltf models with cgltf

### Shaders
- [x] Shader reflection
- [x] Use `vkUpdateDescriptorSetWithTemplate`

### Filesystem
- [x] Use PhysicsFS

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

### Utilities
- [ ] Noise

### Window
- [x] Better event system

### Libraries
- [ ] Use volk

### Assets
- [ ] Multi-threaded asset loading

### Portability
- [ ] `#define`s for dllexport (windows)

### Misc
- [x] Make standalone skybox baker
- [x] Object picking with a color buffer
- [x] Transformation gizmos
- [ ] Selected object outline (with stencil buffer + separate pipeline with `lineWidth = 2.0f`)
- [x] Fullscreen post process shader
- [ ] Document as much as possible
