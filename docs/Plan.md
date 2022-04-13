# DXR Sample plan

## Steps

- [x] Draw a scene with only miss shaders
- [ ] Draw a gray cube
    - [ ] Create BLAS/TLAS objects (AM)
        - [ ] Create BLAS
        - [ ] Create TLAS
        - [ ] Bind it
    - [ ] Write & compile simple closest hit shader with a color returned (MB)
        - [ ] Write a shader with a simple lighting (directional light)
        - [ ] Create shader table (for closest/any hit shaders)
        - [ ] Create hit group for the object
    - [x] Update Raygen shader (DB)(AS)(DP)
        - [x] Use correct view/projection matrices
        - [x] Put these bindings to the raygen shader (using CBV): view position, direction
        - [x] Start using camera object
- [ ] Generate a simplest scene and draw it
    * Start using multiple objects
    * Create something like objects manager
    * Start using local root signatures and bindings
- [ ] Add textures
- [ ] Add shadows & AO
    * Use results from depth pass
- [ ] Add some action to scene
    * Rebuild TLAS
- [ ] Add a separate pass using a convenient rendering (like a depth pass)
- [ ] Add reflection
- [ ] Add transparency & refraction
