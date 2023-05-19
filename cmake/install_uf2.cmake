
add_custom_target(
	program
)

function(install_uf2 TARGET)
	add_custom_command(
		TARGET program
		POST_BUILD
		COMMAND picotool load "${CMAKE_BINARY_DIR}/${TARGET}.uf2"
	)
	add_dependencies(
		program ${TARGET}
	)
endfunction()
