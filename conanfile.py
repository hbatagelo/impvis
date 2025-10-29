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
        self.requires("fmt/12.0.0")
        self.requires("imgui/1.89.4")
        self.requires("cppitertools/2.2")
        self.requires("glm/0.9.9.8")
        self.requires("ms-gsl/4.0.0")
        self.requires("glew/2.2.0")
        self.requires("re2/20220601")
        self.requires("tomlplusplus/3.2.0")
        self.requires("sdl/2.28.3")
        self.requires("sdl_image/2.8.2")
        self.requires("gtest/1.17.0")

    def configure(self):
        # SDL options - only available on Linux
        if self.settings.os == "Linux":
            self.options["sdl"].alsa = False
            self.options["sdl"].pulse = False
            self.options["sdl"].nas = False
            self.options["sdl"].wayland = False

        # SDL_image options (disable many image backends & optional libs)
        self.options["sdl_image"].xv = False
        self.options["sdl_image"].bmp = False
        self.options["sdl_image"].gif = False
        self.options["sdl_image"].lbm = False
        self.options["sdl_image"].pcx = False
        self.options["sdl_image"].pnm = False
        self.options["sdl_image"].qoi = False
        self.options["sdl_image"].svg = False
        self.options["sdl_image"].tga = False
        self.options["sdl_image"].xcf = False
        self.options["sdl_image"].xpm = False
        self.options["sdl_image"].with_libjpeg = False
        self.options["sdl_image"].with_libtiff = False
        self.options["sdl_image"].with_libwebp = False

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
