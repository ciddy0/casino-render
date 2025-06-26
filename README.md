# Casino Scene 3D Mesh Viewer

This application renders a detailed casino scene in 3D using Modern OpenGL (3.3+) and SFML for windowing, input, and audio. It loads models via Assimp, displays them in wireframe or lit mode, and animates objects like dice, slot machine, and a dynamic text greeting.

## Demo Video

A video walkthrough of the application is available here:

[![Watch the video](https://media.discordapp.net/attachments/890005374348955719/1387929005742751775/casino-render.png?ex=685f211d&is=685dcf9d&hm=06aeaa54a088c8ebcf4d61bcaf9cd2c5fe14ccdc624d2ae70a3f2aff74cf6730&=&format=webp&quality=lossless&width=1176&height=831)](https://youtu.be/KJ17N7KtkMA?si=QArGzyAqbkqW7e9A)

## Features

- Modern OpenGL (3.3+) pipeline with VAOs, VBOs, and shaders.

- Phong lighting and texture mapping shaders.

- Assimp integration for loading GLTF, OBJ, and other 3D formats.

- Hierarchical animations:

  - Slot machine lever and reels.

- Bouncing dice with realistic physics and snapping rotation.

- Animated letters spelling out "GATO" along Bezier curves.

- Camera controls for exploring the scene (W/A/S/D, arrow keys).

- Sound effects using SFML Audio (dice roll, coin insert, win tone).

## Controls:

Space: Drop the dice

Enter: Start slot machine animation

W/A/S/D: Rotate camera

Arrow Up/Down: Move camera forward/back

## Project Structure
```
├── src/                # C++ source files
├── shaders/            # GLSL vertex & fragment shaders
├── models/             # 3D assets (GLTF, textures)
├── sounds/             # Audio effects (FLAC, WAV)
├── CMakeLists.txt      # Build configuration
└── README.md           # This documentation
```

