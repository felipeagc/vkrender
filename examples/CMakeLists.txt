function(symlink_dir target name)
	file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/${name}" source)
	file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${name}" destination)

	if (WIN32)
		add_custom_command(
			TARGET ${target} POST_BUILD
			COMMAND mklink /D ${destination} ${source}
			DEPENDS ${destination}
			COMMENT "symbolic link resources folder from ${source} => ${destination}")
	else()
		add_custom_command(
			TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
			DEPENDS ${destination}
			COMMENT "symbolic link resources folder from ${source} => ${destination}")
	endif()
endfunction()

add_executable(rewrite rewrite.c)
target_link_libraries(rewrite engine)
symlink_dir(rewrite assets)
symlink_dir(rewrite shaders)
