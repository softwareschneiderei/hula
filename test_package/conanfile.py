from conan import ConanFile

class HulaTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    def build_requirements(self):
        self.tool_requires(self.tested_reference_str)

    def test(self):
        extension = ".exe" if self.settings_build.os == "Windows" else ""
        # Running it like this will just report the 'how to' and error out, so we ignore the error
        self.run("hula{} mypath".format(extension), ignore_errors=True)