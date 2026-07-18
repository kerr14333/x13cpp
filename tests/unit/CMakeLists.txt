# Unit tests -- self-contained (microtest.hpp), zero external dependencies.

set(X13_UNIT_TESTS
    test_farray
    test_fstring
    test_channels
    test_fformat)

foreach(t ${X13_UNIT_TESTS})
  add_executable(${t} ${t}.cpp)
  target_include_directories(${t} PRIVATE
    "${CMAKE_SOURCE_DIR}/core/include"
    "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(${t} PRIVATE x13_fp_flags)
  add_test(NAME ${t} COMMAND ${t})
endforeach()

# The fformat test validates against gfortran when available. Point it at the
# gfortran the build already knows about (Fortran optional), and give it a
# writable scratch directory. If gfortran cannot be found the test self-skips.
find_program(X13_GFORTRAN gfortran)
if(X13_GFORTRAN)
  set_tests_properties(test_fformat PROPERTIES
    ENVIRONMENT "FFORMAT_GFORTRAN=${X13_GFORTRAN};FFORMAT_SCRATCH=${CMAKE_CURRENT_BINARY_DIR}")
endif()
