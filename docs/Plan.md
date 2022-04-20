# DXR Sample plan

## Steps

- [x] Draw a scene with only miss shaders
- [x] Draw a colored cube
    - [x] Create BLAS/TLAS objects
        - [x] Create BLAS
        - [x] Create TLAS
        - [x] Bind it
    - [x] Write & compile simple closest hit shader with a color returned
        - [x] Write a shader with a simple lighting (directional light)
        - [x] Create shader table (for closest/any hit shaders)
        - [x] Create hit group for the object
    - [x] Update Raygen shader
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
