# 🌕 Moon Flyby Simulator (Inspired by Artemis II)

An interactive, 3D OpenGL orbital mechanics and physics simulation. This project simulates an psuedo N-body gravity system featuring the Sun, Earth, Moon, and a player-controllable rocket (representing the Artemis II mission). 

It combines real-time semi-implicit Euler physics integration for gravity calculations with high-fidelity graphics programming techniques, including custom shaders, procedural geometry, and dynamic lighting.

##  Features

### 🌌 Physics & Simulation
*   **Psuedo N-Body Gravity:** Calculates gravitational pull between celestial bodies dynamically using Semi-Implicit Euler integration for stable orbits, with certain design choices made (skipping Sun gravity calculations) to favor an better interactive experience.
*   **Realistic Scales:** Distances and radii, and masses are scaled relative to Earth's radius and mass respectively.
*   **Accurate Rotations:** Features proper axial tilts (e.g., Earth at 23.4°, Moon at 6.63°) and synchronized rotational speeds, allowing the Moon to remain in a tidal lock. 
*   **Orbit Trails:** Dynamically drawn `GL_LINE_STRIP` trails (that can be toggled on or off) that track the historical positions of the planets and the rocket.

### 🎨 Graphics & Rendering
*   **Advanced Shader Lighting:** Implements ambient, diffuse, and specular lighting models.
*   **Dynamic Earth Texturing:** 
    *   **Specular Mapping:** Differentiates ocean reflections from landmasses.
    *   **Night Lights:** Uses a smoothstep gradient at the terminator line to seamlessly blend glowing city lights into the dark side of the planet.
    *   **Clouds:** Renders an independent, rotating cloud layer using an alpha-hack transparency channel.
*   **Procedural Geometry:** The rocket is procedurally generated via C++ code (a combined cylinder and cone) and dynamically rotates its pitch/yaw to face its current velocity vector.
*   **Sun Corona:** A custom shader math effect that renders a glowing, fiery atmosphere to the Sun as a bloom effect, replicating the Corona of the Sun.
*   **Infinite Skybox:** A massive Milky Way backdrop rendered with depth writing disabled so it never clips with the camera.

### 🎥 Camera System
*   **Orbital Camera (Default):** Locks onto a specific target (Earth, Moon, Sun, or Rocket) and orbits around it.
*   **Free-Fly Camera:** Detaches from the orbital lock, allowing standard 6-DOF movement through the solar system.

---

## 🛠️ Setup & Installation
### 1) Prerequisites

To build and run this project, you will need a C++ compiler, CMake, and the following OpenGL development libraries. 
Run the given commands based on your operating system.

* 🐧 **Debian/Ubuntu**
```bash
sudo apt install build-essential cmake libglfw3-dev libglm-dev libstb-dev
```

* 🐧 **Arch Linux**
```bash
sudo pacman -S base-devel cmake glfw glm stb
```
* 🍏 **Mac**
```bash
brew install cmake glfw glm
```
* 🪟 **Windows** (via MSYS2)

Windows does not include a native C++ compiler or package manager. The most straightforward way to build this project from source is by using [MSYS2](https://www.msys2.org/), which provides a Linux-like terminal environment.

* Download and install **MSYS2** from [msys2.org](https://www.msys2.org/).
* Once installed, search your Windows Start Menu for **"MSYS2 UCRT64"** and open that specific terminal. *(Do not use the default MSYS terminal)*.
* Update the package database by running:
   ```bash
   pacman -Syu
   ```
* Install the C++ compiler, CMake, Make, and the required OpenGL libraries by running:
  ```bash
  pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glm
  ```

### 2) Installation
#### **Windows** :
  
Stay inside the MSYS2 UCRT64 terminal and run the following commands to clone and build the simulation:
```bash
# Clone the repository
git clone https://github.com/pokefanchanti/Moon-Flyby-Simulator
cd Moon-Flyby-Simulator

# Create and enter the build directory
mkdir build
cd build

# Generate the Makefiles (Note the specific MinGW flag for Windows)
cmake -G "MinGW Makefiles" ..

# Compile the project
mingw32-make

# Run the simulation!
./gravity_sim.exe
```

#### **Other operating systems** :
```bash
# Clone the repository
git clone https://github.com/pokefanchanti/Moon-Flyby-Simulator
cd Moon-Flyby-Simulator

# Create and enter the build directory
mkdir build
cd build

# Generate the build files with CMake
cmake ..

# Compile the project
make

# Run the executable
./gravity_sim
```
---
## 🎮 Controls
### Global Controls

    ESC - Close the application

    O - Toggle orbit trails on/off

    C - Toggle camera mode (Switch between Orbital and Free-Fly)

### Orbital Camera Mode

    TAB - Cycle camera target (Earth -> Moon -> Sun -> Rocket)

    Mouse Scroll - Zoom in and out

### Free-Fly Camera Mode

    Mouse - Look around

    W / A / S / D - Move forward, left, backward, and right

    Space - Move Up

    Left Ctrl - Move Down

    Mouse Scroll - Adjust Field of View (FOV)

### Rocket Controls (Artemis II)

You can pilot the rocket while in either camera mode!

    Up Arrow - Prograde thrust (Accelerate in the direction of current travel)

    Down Arrow - Retrograde thrust (Brake/Reverse direction of travel)

    I - Thrust toward the camera's current viewing direction

    K - Thrust away from the camera's current viewing direction

---
## 📂 File Structure
```
.
├── CMakeLists.txt
├── README.md
├── assets
│   ├── shaders
│   │   ├── fragment_core.glsl
│   │   └── vertex_core.glsl
│   └── textures
│       ├── clouds.png
│       ├── clouds_8k.png
│       ├── earth.jpg
│       ├── earth_8k.jpg
│       ├── earth_specular.png
│       ├── milkyway.jpg
│       ├── milkyway_8k.jpg
│       ├── moon.jpg
│       ├── night.jpg
│       ├── night_8k.jpg
│       ├── stars.jpg
│       └── sun.jpg
├── include
│   ├── Camera.h
│   ├── Geometry.h
│   ├── Physics.h
│   ├── Planet.h
│   ├── Shader.h
│   ├── TextureLoader.h
│   ├── KHR
│   │   └── khrplatform.h
│   └── glad
│       └── glad.h
└── src
    ├── Camera.cpp
    ├── Geometry.cpp
    ├── Physics.cpp
    ├── Shader.cpp
    ├── TextureLoader.cpp
    ├── glad.c
    └── main.cpp
```
---

## 📋Feature Checklist
### Physics features :-
* ✅ Distances and Masses to scale
* ✅ Rotational Tilt for Earth and Moon
* ✅ Tidal Locking for Moon
* ✅ A rocket in Low Earth Orbit
* ⬛ Earth and Moon revolving around the Sun
* ⬛ Sun should have gravitational pull
* ⬛ Earth and Moon should revolve around their barycentre
* ⬛ Dynamic tilt to Moon's revolution plane like in real life 

### Graphical features :-
* ✅ Clouds that look like theyre actually moving
* ✅ Sun bloom and corona to represent the how bright it is
* ✅ Dynamic textures for Earth - day and night textures should swap along horizon
* ✅ 3D Rocket Model
* ⬛ Shadows

### Functional features:-
* ✅Orbital Camera
* ✅Improved Rocket Controls
* ⬛ UI that displays info (rocket velocity, distance from moon, distance from earth, time, etc..)
* ⬛ Time Scale Feature (allows us to change the rate at which time flows : 1x, 5x, 10x, etc..)
* ⬛ Autopilot Mode for Rocket
