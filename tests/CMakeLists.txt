add_executable(hula_tests
  hula_tests_main.cpp
  hula_toml_spec.cpp
  device_server_spec.t.cpp
)

target_link_libraries(hula_tests
  hula_core
  CONAN_PKG::catch2
)
