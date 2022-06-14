![build workflow](https://github.com/hbatagelo/impvis/actions/workflows/build.yml/badge.svg)
[![license](https://img.shields.io/github/license/hbatagelo/impvis)](https://github.com/hbatagelo/impvis/blob/main/LICENSE)

ImpVis - 3D Implicit Function Viewer
======

ImpVis is a real-time visualization tool for displaying isosurfaces and scalar fields of algebraic and non-algebraic 3D implicit functions. It is focused on interactivity, allowing the user to change the function parameters and expressions on the fly.

### [Live Demo](https://hbatagelo.github.io/impvis/public/)

Below is a screenshot of ImpVis displaying a quintic surface known as [Togliatti surface](https://en.wikipedia.org/wiki/Togliatti_surface). The positive and negative sides of the surface are shown in red and magenta, respectively.

![Togliatti snapshot](./art/snapshot_togliatti.jpg "Togliatti quintic surface") 
 
ImpVis can also render scalar fields using direct volume rendering, as shown below. The color transfer function maps positive values to red/orange hues, negative values to magenta/purple, and values near zero to white. The mapping can be adjusted using an exponential scaling factor.

![Chmutov snapshot](./art/snapshot_chmutov.jpg "Chmutov Cubic volume rendering") 

## Basic usage
* Drag to rotate the surface (left mouse button) or light source (right mouse button).
* Use the mouse wheel to zoom in/out.
* Press F11 to toggle fullscreen.
* Drag the bottom slider to set the isovalue.
* Use the top-right window to select an equation, change the function parameters and adjust the render settings.

## Equation editor
ImpVis features an equation editor that allows creating new expressions by modifying the existing ones from a catalog of predefined implicit equations.

In the equation editor, the expression for the left-hand side of the equation is written in a modified GLSL ES syntax in which the caret symbol `^` is used as an **exponentiation operator** instead of **bit-wise exclusive or** operator. For example, for the Torus implicit equation

$$
(c-\sqrt{x^2+y^2})^2+z^2-a^2=0,
$$

the left-hand side expression can be written as `(c-sqrt(x^2+y^2))^2+z^2-a^2` instead of `pow(c-sqrt(x*x+y*y),2.0)+z*z-a*a`, which is also supported.

Under the hood, exponentiation expressions of the form `b^n`, where `n` is a positive integer between 2 and 16 (inclusive), are converted to the product of multiplying `n` bases: `b*b*...*b`. This is often more efficient than using the native `pow` function. ImpVis also has custom built-in functions `mpow2(b)`, `mpow3(b)`, up to `mpow16(b)`, that can be used in place of `b^2`, `b^3`, up to `b^16`, where `b` is an expression that evaluates to `float`.

Another syntax difference from GLSL is the use of square brackets as parentheses instead of array indexing. For example, for the Cassini quartic surface equation

$$
\left[(x+r)^2+y^2\right]\left[(x-r)^2+y^2\right]-z^2=0,
$$

the left-hand side expression can be written as `[(x+r)^2+y^2]*[(x-r)^2+y^2]-z^2`.

The editor also allows injecting GLSL ES code both in the global scope of the shader and in the local scope of the shader function that evaluates the implicit function. Try selecting different equations from the catalog to see how their scopes are implemented in the equation editor. As a rule of thumb:
* Use the global scope to define GLSL functions and constants to be used either in the local scope or directly in the implicit function.
* Use the local scope to define GLSL variables to be used in the implicit function expression.

Any changes to the currently selected equation (expression, local scope or global scope) will be saved as a user-defined equation located at that end of the catalog.

#### Remarks
The following apply for code injected in the local scope:
* `p` is the name of a `vec3` variable containing the values of the `x`, `y` and `z` variables of the 3D implicit function. Thus, in the local scope, use `p.x`, `p.y` and `p.z` instead of `x`, `y`, `z`.  
* `p2` is a shortcut for `p*p`, `p3` is a shortcut for `p*p*p`, and so on up to `p16`. Again, these are often more efficient than `pow(p,n)`. The functions `mpow2(b)`, `mpow3(b)`, up to `mpow16(b)`, are also available in the local scope.


## How it works
The isosurfaces are rendered using an adaptive raymarching algorithm. The scalar fields are rendered using direct volume rendering. Both are implemented as [GLSL ES 3.00]((https://www.khronos.org/registry/OpenGL/specs/es/3.0/GLSL_ES_Specification_3.00.pdf)) shaders. The equation names and the expressions shown in the top-left corner of the screen are rendered using [MathJax](https://www.mathjax.org/) (WebAssembly only).

The adaptive raymarching algorithm adjusts the size of the ray's next step according to the value of the scalar field and gradient evaluated at the current step. The step size decreases as the ray approaches the surface, and increases as it moves away from it. This is similar to the _adaptive marching points_ algorithm described in [Real-Time Ray Tracing of Implicit Surfaces on the GPU](https://ieeexplore.ieee.org/document/4815235) (Singh et al. 2009). However, in ImpVis the step size varies gradually as the rays approach the surface, thus reducing the number of conditional branchings. In addition, the step size decreases as the ray approaches the limits of the bounding volume as a measure to avoid clipping artifacts. For details, read the inline comments in the fragment shader (`raycast.frag`).

## Building
ImpVis can be built for desktop (Windows, Linux, macOS) and for the web (WebAssembly).

First clone the repo:
```
git clone https://github.com/hbatagelo/impvis.git
cd impvis
```

Make sure the following tools are installed and are reachable from the path:
* [Conan](https://conan.io/) 1.44 or later (not required for WebAssembly).
* [CMake](https://cmake.org/) 3.18 or later.
* A C++ compiler with at least partial support to C++20 (tested with GCC 11, Clang 13, MSVC 17, and emcc 3.1).

### On Windows

Run `build-vs.bat` for building with Visual Studio 2022. The script will execute the following commands:

```
# Create the build directory
mkdir build && cd build

# Set the build type
set BUILD_TYPE=Release

# Configure (VS 2022 generator for a 64-bit target architecture)
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ..

# Build
cmake --build . --config %BUILD_TYPE%
```

### On macOS and Linux (including WSL2)

Run `./build.sh`. It will execute the following commands:
```
# Create the build directory
mkdir build && cd build

# Set the build type
BUILD_TYPE=Release

# Configure (Makefile generator)
cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..

# Build
cmake --build . --config $BUILD_TYPE
```

### Building for WebAssembly

1. Install [Emscripten](https://emscripten.org/) and activate its environment variables.
2. Run `build-wasm.bat` (Windows) or `build-wasm.sh` (Linux, macOS).

The WASM binaries will be written to `impvis/public`.

----
## License
MIT License.

## Development history
ImpVis was originally designed in 2014, based on Qt and licensed under GPLv3. The current version is a complete rewrite of that [first version](http://professor.ufabc.edu.br/~harlen.batagelo/impvis/). It is now available under the MIT license and is based on the [ABCg](https://github.com/hbatagelo/abcg)  framework.