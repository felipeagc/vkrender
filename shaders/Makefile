SHADER_SOURCES := $(wildcard *.vert *.frag *.geom)
SPVS = $(addprefix out/, $(SHADER_SOURCES:=.spv))

all: out $(SPVS)

out:
	mkdir -p $@

out/%.spv: %
	glslc $< -o $@

clean:
	rm -r out
