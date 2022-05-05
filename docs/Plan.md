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
- [x] Generate a simplest scene and draw it
    - [x] Start using multiple objects
    - [x] Create something like objects manager
    - [x] Start using local root signatures and bindings
    - [x] Implement simplest materials system (to use different hit shaders)
    - [X] Implement possibility to create objects externally out of SceneManager
        - [X] Scene manager must provide interface to create objects
        - [X] Scene manager must provide interface to pass a custom geometry
    - [x] Generate a geometry of an island using Perlin noise
        - [x] Generate cubes for a noise data
    - [x] Generate a geometry for water
- [x] Add reflections
- [ ] Add flora
    - [ ] Generate bushes
    - [ ] Generate a tree
    - [ ] Place flora in the world using a random values or the noise value
- [ ] Add clouds
    - [ ] Generate clouds using a noise
    - [ ] Make clouds transparent
    - [ ] Animate them
- [ ] Add sun & sky
    - [ ] Add a sun object on the sky
    - [ ] Add a sky with a Relay scattering algorithm
- [ ] Add shadows & AO
    - [ ] Add depth prepass
    - [ ] Generate rays from each point to the light position and check collision
    - [ ] For AO generate multiple short rays in random directions

## Other tasks

- [x] Switch to C++20 -- EASY
- [ ] Implement procedural generation of simple objects: -- EASY
    - [ ] Sphere
    - [ ] Torus
- [ ] Implement WASD camera -- EASY
- [ ] Implement heap resizing -- EASY
- [ ] Implement scene tree with configurable parameters -- MEDIUM
- [ ] Implement in-app profiler (configurable) to measure: -- MEDIUM
    - [ ] TLAS building time
    - [ ] Ray tracing pass
    - [ ] UI drawing pass
- [ ] Implement microsurface BRDF -- HARD
- [ ] Switch to linear rendering with color-correction -- HARD
- [ ] Optimize a landscape geometry using quad/octtree -- HARD

## Scene description

The main idea is to have an island in a sea:

- A landmass is generated procedurally using a Perlin noise.
- The geometry is tessellated using cubes.
- There are trees, stones, grass on the island. OPTIONAL
- There are volumetric clouds and a sun.
- There are possibility to change day time. OPTIONAL

## Landscape generation

- Landscape consists of vertical columns. Each column has 2 triangles on the top and multiple triangles on sides.
- Landscape is generated using a height map.
- Each 'cube' has color:
    - <50 levels are yellow
    - Sea level is 50 blocks height
    - 50-53 levels are yellow (sand)
    - 53-63 levels are green (grass)
    - 63-80 levels are grey (rocks)
    - 80+ levels are white (snow)
