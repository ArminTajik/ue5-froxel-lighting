# UE5 Froxel Lighting Plugin

A real-time **clustered (froxel) lighting and volumetrics system** for Unreal Engine 5.  
Built as a C++/HLSL plugin with Render Graph integration.


## Features (Planned Roadmap)
- [x] Froxel grid builder (frustum voxel partitioning).
- [x] Compute shader for per-froxel light assignment.
- [ ] GPU light buffer with SoA (structure of arrays) layout.
- [ ] Debug heatmap overlay for froxel occupancy.
- [ ] Cascaded shadow maps + shadow atlas visualization.
- [ ] Temporal AA with reactive mask fixes.
- [ ] Volumetric fog with temporal reprojection.
- [ ] Screen-space reflections (scalability tiers).
- [ ] In-engine performance HUD (FPS, draw calls, light count).

### Goal
- Demonstrate modern real-time rendering techniques in UE5.
- Benchmark performance across Apple M3 (Metal) and RTX 3060 (DX12).

### Process Overview
1. **Froxel Grid Construction**: Partition the camera frustum into a 3D grid of froxels (frustum voxels).
2. **Light Counting**: Count the number of visible lights affecting each froxel and store this in a structured buffer.
3. **Buffer Offsets**: Create a buffer of offsets to index into the light list for each froxel."
   1. Blelloch scan (prefix sum) to compute offsets in each Z-slice.
   2. Blelloch scan across Z-slices to get final offsets.
   3. Add a final offset to each froxel to get absolute indices.
4. **Light Assignment**: For each froxel, assign the indices of lights affecting it
5. **Rendering**: Use the froxel light data for clustered lighting and volumetric effects.

### License
MIT — free to use, modify, and share.  

## Build Instructions
1. Clone this repo into your UE5 project’s `Plugins/` directory:
   ```bash
   git clone https://github.com/ArminTajik/ue5-froxel-lighting Plugins/FroxelLighting