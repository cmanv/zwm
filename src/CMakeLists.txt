file (GLOB sources *.cc *.h)
add_executable (zwm ${sources})
set_target_properties (zwm PROPERTIES LINKER_LANGUAGE C)
install (TARGETS zwm DESTINATION "bin")
