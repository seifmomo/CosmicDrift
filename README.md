# Cosmic Drift: Space Runner

A 3D space-themed endless runner game built entirely with OpenGL (GLUT) and C++. Pilot a spaceship through an asteroid field collecting glowing rings. All objects are modeled from primitives with no external assets.

## Features

- **Spaceship Model** — 12+ part ship built from primitives: body, nose cone, cockpit canopy (emissive cyan), wings, wing tips (emissive blue), engine nacelles, pulsing exhaust, tail fin, neon stripe
- **Asteroid Obstacles** — Randomly placed gray/brown/red spheres with surface bumps, slow rotation, and varied sizes
- **Collectible Rings** — 32-segment torus with inner glowing sphere, 3 color variants (cyan, magenta, orange), pulsing emissive glow
- **Particle Systems** — Engine trail (blue/cyan), ring collection burst (25 particles), collision explosion (50 particles), up to 1500 active
- **Fog** — Linear depth fog for space atmosphere
- **3 Light Sources** — Directional moonlight, fill light, ship headlight spotlight
- **Camera** — Smooth third-person follow with lerp, screen shake on collision
- **HUD** — Score, rings collected, speed display, boost bar, menu screen, game over with high score

## Controls

| Key | Action |
|-----|--------|
| W / S / Up / Down | Steer Up / Down |
| A / D / Left / Right | Steer Left / Right |
| SPACE | Boost (limited fuel) |
| ENTER | Start Game / Menu |
| ESC | Back to Menu / Exit |

## Building

Requires MinGW g++ and FreeGLUT.

```bash
g++ -o CosmicDrift.exe main.cpp -lfreeglut -lopengl32 -lglu32 -lm
```

Or run `build.bat` on Windows.

## Technical Details

- **Transformations** — Translation, rotation, scaling for all 3D objects
- **Camera System** — Smooth lerp follow camera with screen shake
- **Lighting** — 3 light sources with ambient, diffuse, specular properties
- **Materials** — Emissive materials for glowing neon objects, metallic specular for ship body
- **Particle System** — Point sprites with velocity, lifetime, and color fade
- **Fog** — GL_LINEAR fog for depth effect (start 30, end 120)
- **Animation** — Ship tilt on steering, asteroid rotation, ring pulsing, speed increase over time
- **Difficulty Progression** — Speed increases over time, more obstacles appear

## Project Type

Computer Graphics Final Project — demonstrates transformations, camera systems, lighting, materials, object modeling, animation, and user interaction in a space theme.
