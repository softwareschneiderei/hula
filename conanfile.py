import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.files import copy


class HulaRecipe(ConanFile):
    name = "hula"
    version = "1.0"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"
    url = "https://github.com/softwareschneiderei/hula"
    exports_sources = "CMakeLists.txt", "main.cpp", "source/*", "tests/*"
    requires = "toml11/3.8.1", "fmt/10.2.1"
    test_requires = "catch2/3.6.0"
    generators = "CMakeDeps"

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        extension = ".exe" if self.settings_build.os == "Windows" else ""
        copy(self, "*hula{}".format(extension),
             self.build_folder, os.path.join(self.package_folder, "bin"), keep_path=False)
