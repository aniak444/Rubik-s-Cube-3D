# Rubik's Cube 3D - SFML & OpenGL

A functional 3D Rubik's Cube simulator built using **C++**, **OpenGL**, and **SFML**. The project features a custom-built rotation engine, interactive UI with **ImGui**, and a dynamic lighting shader.

## 🚀 Features

* **3D Visualization**: Fully interactive cube rendered with OpenGL.
* **Smooth Animations**: Real-time rotation animations for all cube faces.
* **Custom Shaders**: Fragment shader implementation for ambient, diffuse, and specular (Phong) lighting.
* **Interactive UI**: Integrated ImGui panel for camera control, shuffling, and manual move execution.
* **Mouse & Keyboard Control**: Rotate the entire view with the mouse or use standard Rubik's notation (L, R, U, D, F, B) on the keyboard.
* **Win Detection**: The game automatically detects when the cube is solved.

## 🛠 Tech Stack

* **Language**: C++
* **Graphics**: OpenGL (Legacy pipeline + GLSL Shaders)
* **Window/System**: SFML 2.x
* **UI**: Dear ImGui
* **Math**: Custom matrix multiplication logic for 3D transformations.

## 🎮 Controls

### Keyboard
| Key | Action |
| :--- | :--- |
| **L, R, U, D, F, B** | Rotate specific slice |
| **Shift + Key** | Rotate slice in reverse direction |
| **Space** | Random shuffle (20 moves) |

### Mouse
* **Left Click + Drag**: Rotate camera.
* **Scroll**: Zoom in/out.

## 📸 Screenshots
*(Hint: Add a screenshot or GIF here to make your repo look professional!)*

## ⚙️ Requirements & Building

To run this project, you need to have the following libraries configured in your IDE (e.g., Visual Studio):
1. **SFML 2.5+**
2. **ImGui-SFML**
3. **OpenGL / GLU**

1. Clone the repository:
   ```bash
   git clone [https://github.com/YOUR_USERNAME/rubiks-cube-3d.git](https://github.com/YOUR_USERNAME/rubiks-cube-3d.git)
