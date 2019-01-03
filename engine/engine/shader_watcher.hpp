#pragma once

#include "file_watcher.hpp"
#include <ftl/logging.hpp>
#include <functional>
#include <mutex>
#include <renderer/base_render_target.hpp>
#include <renderer/context.hpp>
#include <renderer/pipeline.hpp>
#include <renderer/shader.hpp>
#include <renderer/util.hpp>
#include <string>
#include <thread>

namespace renderer {
class Window;
}

namespace engine {
template <class Pipeline> class ShaderWatcher {
public:
  ShaderWatcher(
      renderer::BaseRenderTarget &renderTarget,
      const std::string &vertexPath,
      const std::string &fragmentPath)
      : m_vertexPath(vertexPath),
        m_fragmentPath(fragmentPath),
        m_renderTarget(renderTarget) {
    renderer::Shader shader{
        m_vertexPath,
        m_fragmentPath,
    };

    m_pipeline = Pipeline{m_renderTarget, shader};

    shader.destroy();

    m_watcher.addFile(m_vertexPath);
    m_watcher.addFile(m_fragmentPath);

    m_watcher.onModify = [&](const std::string filename) {
      ftl::debug("Shader \"%s\" was modified", filename.c_str());
      auto lock = this->lockPipeline();

      VK_CHECK(vkDeviceWaitIdle(renderer::ctx().m_device));

      try {
        renderer::Shader shader{
            m_vertexPath,
            m_fragmentPath,
        };

        m_pipeline = Pipeline{m_renderTarget, shader};

        shader.destroy();
      } catch (const std::exception &exception) {
        ftl::error("Error while compiling shader: %s", exception.what());
      }
    };
  }

  ~ShaderWatcher(){};

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

  inline renderer::GraphicsPipeline &pipeline() noexcept { return m_pipeline; }

  inline void startWatching() noexcept { m_watcher.startWatching(); };

private:
  std::string m_vertexPath;
  std::string m_fragmentPath;
  std::mutex m_mutex;
  Pipeline m_pipeline;
  FileWatcher m_watcher;
  renderer::BaseRenderTarget &m_renderTarget;
};
} // namespace engine
