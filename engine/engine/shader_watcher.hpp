#pragma once

#include "file_watcher.hpp"
#include "util.hpp"
#include <ftl/logging.hpp>
#include <functional>
#include <mutex>
#include <renderer/context.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/render_target.hpp>
#include <renderer/shader.hpp>
#include <renderer/util.hpp>
#include <string>
#include <thread>

namespace engine {
class ShaderWatcher {
public:
  ShaderWatcher(
      re_render_target_t *renderTarget,
      const char *vertexPath,
      const char *fragmentPath,
      re_pipeline_parameters_t pipeline_params)
      : pipeline_params(pipeline_params),
        m_vertexPath(vertexPath),
        m_fragmentPath(fragmentPath),
        m_renderTarget(renderTarget) {
    eg_init_pipeline(
        &this->pipeline,
        *m_renderTarget,
        m_vertexPath.c_str(),
        m_fragmentPath.c_str(),
        this->pipeline_params);

    m_watcher.addFile(m_vertexPath.c_str());
    m_watcher.addFile(m_fragmentPath.c_str());

    m_watcher.onModify = [&](const std::string filename) {
      ftl::debug("Shader \"%s\" was modified", filename.c_str());
      auto lock = this->lockPipeline();

      VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

      try {
        re_pipeline_destroy(&this->pipeline);

        eg_init_pipeline(
            &this->pipeline,
            *m_renderTarget,
            m_vertexPath.c_str(),
            m_fragmentPath.c_str(),
            this->pipeline_params);
      } catch (const std::exception &exception) {
        ftl::error("Error while compiling shader: %s", exception.what());
      }
    };
  }

  ~ShaderWatcher() { re_pipeline_destroy(&this->pipeline); };

  ShaderWatcher(const ShaderWatcher &) = delete;
  ShaderWatcher &operator=(const ShaderWatcher &) = delete;

  ShaderWatcher(ShaderWatcher &&) = delete;
  ShaderWatcher &operator=(ShaderWatcher &&) = delete;

  /*
    Use this to lock the pipeline so it can be used.
   */
  inline std::scoped_lock<std::mutex> lockPipeline() noexcept {
    return std::scoped_lock<std::mutex>(m_mutex);
  }

  inline void startWatching() noexcept { m_watcher.startWatching(); };

  re_pipeline_t pipeline;
  re_pipeline_parameters_t pipeline_params;

private:
  std::string m_vertexPath;
  std::string m_fragmentPath;
  std::mutex m_mutex;
  FileWatcher m_watcher;
  re_render_target_t *m_renderTarget;
};
} // namespace engine
