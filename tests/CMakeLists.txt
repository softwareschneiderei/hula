add_executable(hula_tests
  hula_tests_main.cpp
  hula_toml_spec.cpp
  device_server_spec.t.cpp
)

target_link_libraries(hula_tests
  PUBLIC hula_core
  PUBLIC Catch2::Catch2WithMain
)
