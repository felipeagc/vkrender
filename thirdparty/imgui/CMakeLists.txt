add_library(
	imgui
	imgui.cpp
	imgui.h
	imgui_internal.h
	imconfig.h
	cimgui.cpp
	cimgui.h
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_impl_glfw.cpp
	imgui_impl_glfw.h
	imgui_widgets.cpp
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h)

target_link_libraries(
	imgui
	PUBLIC
	volk
	glfw)

target_include_directories(
	imgui
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
	target_compile_options(imgui PRIVATE /wd4190)
else()
	target_compile_options(imgui PRIVATE -Wno-return-type-c-linkage)
endif()
