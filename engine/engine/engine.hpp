#pragma once

#include "asset_manager.hpp"
#include "file_watcher.hpp"
#include "imgui_utils.hpp"
#include "shader_watcher.hpp"
#include "scene.hpp"

#include "components/billboard_component.hpp"
#include "components/camera_component.hpp"
#include "components/environment_component.hpp"
#include "components/gltf_model_component.hpp"
#include "components/light_component.hpp"
#include "components/transform_component.hpp"

#include "systems/billboard_system.hpp"
#include "systems/entity_inspector_system.hpp"
#include "systems/fps_camera_system.hpp"
#include "systems/gltf_model_system.hpp"
#include "systems/lighting_system.hpp"
#include "systems/skybox_system.hpp"

#include "assets/environment_asset.hpp"
#include "assets/gltf_model_asset.hpp"
#include "assets/texture_asset.hpp"
#include "assets/shape_asset.hpp"
