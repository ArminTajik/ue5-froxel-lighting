# UE5 Froxel Lighting Plugin

A real-time **clustered (froxel) lighting and volumetrics system** for Unreal Engine 5.  
Built as a C++/HLSL plugin with Render Graph integration.


## Features (Planned Roadmap)
- [ ] Froxel grid builder (frustum voxel partitioning).
- [ ] Compute shader for per-froxel light assignment.
- [ ] GPU light buffer with SoA layout.
- [ ] Debug heatmap overlay for froxel occupancy.
- [ ] Cascaded shadow maps + shadow atlas visualization.
- [ ] Temporal AA with reactive mask fixes.
- [ ] Volumetric fog with temporal reprojection.
- [ ] Screen-space reflections (scalability tiers).
- [ ] In-engine performance HUD (FPS, draw calls, light count).

### Goal
- Demonstrate **modern real-time rendering techniques** in UE5.
- Benchmark performance across **Apple M2 (Metal)** and **RTX 3060 (DX12)**.
- Publish public, well-documented code as a reference for graphics interviews.

### License
MIT — free to use, modify, and share.  

## Build Instructions
1. Clone this repo into your UE5 project’s `Plugins/` directory:
   ```bash
   git clone https://github.com/ArminTajik/ue5-froxel-lighting Plugins/FroxelLighting