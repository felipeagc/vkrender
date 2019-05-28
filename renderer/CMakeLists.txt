add_library(
	renderer

	renderer/renderer.h

	renderer/common.h

	renderer/window.c
	renderer/window.h

	renderer/event.h

	renderer/cmd_buffer.c
	renderer/cmd_buffer.h

	renderer/buffer.c
	renderer/buffer.h

	renderer/canvas.c
	renderer/canvas.h

	renderer/context.c
	renderer/context.h

	renderer/image.c
	renderer/image.h

	renderer/pipeline.c
	renderer/pipeline.h

	renderer/render_target.h

	renderer/shader.c
	renderer/shader.h

	renderer/descriptor_set.c
	renderer/descriptor_set.h

	renderer/util.c
	renderer/util.h
	)

target_link_libraries(
	renderer
	PUBLIC
	${Vulkan_LIBRARIES}
	glfw
	imgui
	fstd
	gmath
	stb_image
	tinycthread
	vma
	spirv_reflect)

target_include_directories(
	renderer
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${Vulkan_INCLUDE_DIRS})
