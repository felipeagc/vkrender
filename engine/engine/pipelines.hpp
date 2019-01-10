#pragma once

#include <renderer/pipeline.hpp>
#include <renderer/shader.hpp>

namespace engine {
renderer::PipelineParameters standardPipelineParameters();

renderer::PipelineParameters billboardPipelineParameters();

renderer::PipelineParameters wireframePipelineParameters();

renderer::PipelineParameters skyboxPipelineParameters();

renderer::PipelineParameters fullscreenPipelineParameters();
} // namespace engine
