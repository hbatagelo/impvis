from conan import ConanFile
from conan.tools.cmake import CMake
from conan.tools.cmake import cmake_layout
from conan.tools.files import copy
import os

class ImpVis(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def requirements(self):
        self.requires("cppitertools/2.2")
        self.requires("fmt/12.1.0")
        if self.settings.os != "Emscripten":
            self.requires("glew/2.2.0")
        self.requires("glm/1.0.1")
        self.requires("gtest/1.17.0")
        self.requires("imgui/1.92.5")
        self.requires("ms-gsl/4.2.0")
        self.requires("re2/20251105")
        self.requires("sdl/3.2.20")
        self.requires("stb/cci.20240531")
        self.requires("tomlplusplus/3.4.0")

    def configure(self):
        if self.settings.os == "Linux":
            self.options["sdl"].audio = False
            # Uncomment if building for Wayland (run with EGL_PLATFORM=wayland SDL_VIDEODRIVER=wayland)
            # self.options["glew"].with_egl = True

    def layout(self):
        cmake_layout(self)

    def generate(self):
        # Copy imgui bindings into source bindings
        for dep in self.dependencies.values():
            if dep.ref.name == "imgui":
                if dep.cpp_info.resdirs:
                    bindings_src = os.path.join(dep.cpp_info.resdirs[0], "bindings")
                else:
                    bindings_src = os.path.join(dep.package_folder, "res", "bindings")

                bindings_dst = os.path.join(self.source_folder, "bindings")

                if os.path.exists(bindings_src):
                    copy(self, "*.cpp", src=bindings_src, dst=bindings_dst)
                    copy(self, "*.h", src=bindings_src, dst=bindings_dst)
                else:
                    self.output.warning(f"imgui bindings not found at {bindings_src}")