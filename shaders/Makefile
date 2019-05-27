SHADER_SOURCES := $(wildcard *.vert *.frag *.geom)
SHADER_INCLUDES := $(wildcard *.glsl)
SPVS = $(addprefix out/, $(SHADER_SOURCES:=.spv))

all: out $(SPVS)

out:
	mkdir -p $@

out/%.spv: % $(SHADER_INCLUDES)
	glslc $< -o $@

clean:
	rm -r out
