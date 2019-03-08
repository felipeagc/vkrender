#include "context.hpp"
#include "util.hpp"
#include "window.hpp"
#include <fstd/array.h>
#include <util/log.h>

re_context_t g_ctx;

// Debug callback

// Ignore warnings for this function
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char *layerPrefix,
    const char *msg,
    void *userData) {
  ut_log_error("Validation layer: %s", msg);

  return VK_FALSE;
}
#pragma GCC diagnostic pop

VkResult CreateDebugReportCallbackEXT(
    VkInstance instance,
    const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugReportCallbackEXT *pCallback) {
  PFN_vkCreateDebugReportCallbackEXT func =
      (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugReportCallbackEXT");
  if (func != NULL) {
    return func(instance, pCreateInfo, pAllocator, pCallback);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugReportCallbackEXT(
    VkInstance instance,
    VkDebugReportCallbackEXT callback,
    const VkAllocationCallbacks *pAllocator) {
  PFN_vkDestroyDebugReportCallbackEXT func =
      (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugReportCallbackEXT");
  if (func != NULL) {
    func(instance, callback, pAllocator);
  }
}

static inline bool check_validation_layer_support() {
  uint32_t count;
  vkEnumerateInstanceLayerProperties(&count, NULL);
  VkLayerProperties *availableLayers =
      (VkLayerProperties *)malloc(sizeof(VkLayerProperties) * count);
  vkEnumerateInstanceLayerProperties(&count, availableLayers);

  for (const char *layerName : RE_REQUIRED_VALIDATION_LAYERS) {
    bool layerFound = false;

    for (uint32_t i = 0; i < count; i++) {
      if (strcmp(availableLayers[i].layerName, layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) {
      free(availableLayers);
      return false;
    }
  }

  free(availableLayers);
  return true;
}

static inline void get_required_extensions(
    const char *const *required_window_vulkan_extensions,
    uint32_t extension_count,
    const char **out_extensions,
    uint32_t *out_extension_count) {
  *out_extension_count = extension_count;

#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    (*out_extension_count)++;
  }
#endif

  if (out_extensions == NULL) {
    return;
  }

  for (uint32_t i = 0; i < extension_count; i++) {
    out_extensions[i] = required_window_vulkan_extensions[i];
  }

#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    out_extensions[extension_count] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
  }
#endif
}

static inline bool check_physical_device_properties(
    VkPhysicalDevice physical_device,
    VkSurfaceKHR surface,
    uint32_t *graphics_queue_family,
    uint32_t *present_queue_family,
    uint32_t *transfer_queue_family) {
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(
      physical_device, NULL, &extension_count, NULL);
  VkExtensionProperties *available_extensions = (VkExtensionProperties *)malloc(
      sizeof(VkExtensionProperties) * extension_count);
  vkEnumerateDeviceExtensionProperties(
      physical_device, NULL, &extension_count, available_extensions);

  VkPhysicalDeviceProperties device_properties;
  vkGetPhysicalDeviceProperties(physical_device, &device_properties);

  for (uint32_t i = 0; i < ARRAYSIZE(RE_REQUIRED_DEVICE_EXTENSIONS); i++) {
    const char *required_extension = RE_REQUIRED_DEVICE_EXTENSIONS[i];
    bool found = false;
    for (uint32_t i = 0; i < extension_count; i++) {
      if (strcmp(required_extension, available_extensions[i].extensionName) ==
          0) {
        found = true;
      }
    }

    if (!found) {
      ut_log_warn(
          "Physical device {} doesn't support extension named \"{}\"",
          device_properties.deviceName,
          required_extension);
      free(available_extensions);
      return false;
    }
  }

  free(available_extensions);

  uint32_t major_version = VK_VERSION_MAJOR(device_properties.apiVersion);
  // uint32_t minorVersion = VK_VERSION_MINOR(deviceProperties.apiVersion);
  // uint32_t patchVersion = VK_VERSION_PATCH(deviceProperties.apiVersion);

  if (major_version < 1 &&
      device_properties.limits.maxImageDimension2D < 4096) {
    ut_log_warn(
        "Physical device {} doesn't support required parameters!",
        device_properties.deviceName);
    return false;
  }

  // Check for required device features
  VkPhysicalDeviceFeatures features = {};
  vkGetPhysicalDeviceFeatures(physical_device, &features);
  if (!features.wideLines) {
    ut_log_warn(
        "Physical device {} doesn't support required features!",
        device_properties.deviceName);
    return false;
  }

  uint32_t queue_family_prop_count;
  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_prop_count, NULL);
  VkQueueFamilyProperties *queue_family_properties =
      (VkQueueFamilyProperties *)malloc(
          sizeof(VkQueueFamilyProperties) * queue_family_prop_count);
  vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_prop_count, queue_family_properties);

  VkBool32 *queue_present_support =
      (VkBool32 *)malloc(sizeof(VkBool32) * queue_family_prop_count);
  VkBool32 *queue_transfer_support =
      (VkBool32 *)malloc(sizeof(VkBool32) * queue_family_prop_count);

  uint32_t graphics_queue_family_index = UINT32_MAX;
  uint32_t present_queue_family_index = UINT32_MAX;
  uint32_t transfer_queue_family_index = UINT32_MAX;

  // @TODO: figure out better logic to single out a transfer queue

  for (uint32_t i = 0; i < queue_family_prop_count; i++) {
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(
        physical_device, i, surface, &queue_present_support[i]));

    if (queue_family_properties[i].queueCount > 0 &&
        queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT &&
        !(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      transfer_queue_family_index = i;
    }

    if (queue_family_properties[i].queueCount > 0 &&
        queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      if (graphics_queue_family_index == UINT32_MAX) {
        graphics_queue_family_index = i;
      }

      if (queue_present_support[i]) {
        if (transfer_queue_family_index == UINT32_MAX) {
          *transfer_queue_family = i;
        }
        *graphics_queue_family = i;
        *present_queue_family = i;

        free(queue_family_properties);
        free(queue_present_support);
        free(queue_transfer_support);
        return true;
      }
    }
  }

  if (transfer_queue_family_index == UINT32_MAX) {
    transfer_queue_family_index = graphics_queue_family_index;
  }

  for (uint32_t i = 0; i < queue_family_prop_count; i++) {
    if (queue_present_support[i]) {
      present_queue_family_index = i;
      break;
    }
  }

  if (graphics_queue_family_index == UINT32_MAX ||
      present_queue_family_index == UINT32_MAX ||
      transfer_queue_family_index == UINT32_MAX) {
    ut_log_warn(
        "Could not find queue family with requested properties on physical "
        "device {}",
        device_properties.deviceName);

    free(queue_family_properties);
    free(queue_present_support);
    free(queue_transfer_support);
    return false;
  }

  *graphics_queue_family = graphics_queue_family_index;
  *present_queue_family = present_queue_family_index;
  *transfer_queue_family = transfer_queue_family_index;

  free(queue_family_properties);
  free(queue_present_support);
  free(queue_transfer_support);

  return true;
}

static inline void create_instance(
    re_context_t *ctx,
    const char *const *required_window_vulkan_extensions,
    uint32_t window_extension_count) {
  ut_log_debug("Creating vulkan instance");
#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    ut_log_debug("Using validation layers");
  } else {
    ut_log_debug("Validation layers requested but not available");
  }
#else
  ut_log_debug("Not using validation layers");
#endif

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext = NULL;
  appInfo.pApplicationName = "App";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.flags = 0;
  createInfo.pApplicationInfo = &appInfo;

  createInfo.enabledLayerCount = 0;
  createInfo.ppEnabledLayerNames = NULL;

#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    createInfo.enabledLayerCount =
        (uint32_t)ARRAYSIZE(RE_REQUIRED_VALIDATION_LAYERS);
    createInfo.ppEnabledLayerNames = RE_REQUIRED_VALIDATION_LAYERS;
  }
#endif

  // Set required instance extensions
  uint32_t extension_count = 0;
  get_required_extensions(
      required_window_vulkan_extensions,
      window_extension_count,
      NULL,
      &extension_count);
  const char **extensions =
      (const char **)malloc(sizeof(const char *) * extension_count);
  get_required_extensions(
      required_window_vulkan_extensions,
      window_extension_count,
      extensions,
      &extension_count);
  createInfo.enabledExtensionCount = extension_count;
  createInfo.ppEnabledExtensionNames = extensions;

  VK_CHECK(vkCreateInstance(&createInfo, NULL, &ctx->instance));

  free(extensions);
}

static inline void setup_debug_callback(re_context_t *ctx) {
  VkDebugReportCallbackCreateInfoEXT createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags =
      VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  createInfo.pfnCallback = debug_callback;

  VK_CHECK(CreateDebugReportCallbackEXT(
      ctx->instance, &createInfo, NULL, &ctx->debug_callback));
}

static inline void create_device(re_context_t *ctx, VkSurfaceKHR surface) {
  uint32_t physical_device_count;
  vkEnumeratePhysicalDevices(ctx->instance, &physical_device_count, NULL);
  VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)malloc(
      sizeof(VkPhysicalDevice) * physical_device_count);
  vkEnumeratePhysicalDevices(
      ctx->instance, &physical_device_count, physical_devices);

  for (uint32_t i = 0; i < physical_device_count; i++) {
    if (check_physical_device_properties(
            physical_devices[i],
            surface,
            &ctx->graphics_queue_family_index,
            &ctx->present_queue_family_index,
            &ctx->transfer_queue_family_index)) {
      ctx->physical_device = physical_devices[i];
      break;
    }
  }

  free(physical_devices);

  if (!ctx->physical_device) {
    ut_log_fatal("Could not select physical device based on chosen properties");
    abort();
  }

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(ctx->physical_device, &properties);

  ut_log_debug("Using physical device: %s", properties.deviceName);

  uint32_t queue_create_info_count = 0;
  VkDeviceQueueCreateInfo queue_create_infos[3] = {};
  float queue_priorities[] = {1.0f};

  queue_create_infos[queue_create_info_count++] = VkDeviceQueueCreateInfo{
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      NULL,
      0,
      ctx->graphics_queue_family_index,
      (uint32_t)ARRAYSIZE(queue_priorities),
      queue_priorities,
  };

  if (ctx->present_queue_family_index != ctx->graphics_queue_family_index) {
    queue_create_infos[queue_create_info_count++] = VkDeviceQueueCreateInfo{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        NULL,
        0,
        ctx->present_queue_family_index,
        (uint32_t)ARRAYSIZE(queue_priorities),
        queue_priorities,
    };
  }

  if (ctx->transfer_queue_family_index != ctx->graphics_queue_family_index) {
    queue_create_infos[queue_create_info_count++] = VkDeviceQueueCreateInfo{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        NULL,
        0,
        ctx->transfer_queue_family_index,
        (uint32_t)ARRAYSIZE(queue_priorities),
        queue_priorities,
    };
  }

  VkDeviceCreateInfo device_create_info = {};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.flags = 0;
  device_create_info.queueCreateInfoCount = queue_create_info_count;
  device_create_info.pQueueCreateInfos = queue_create_infos;

  device_create_info.enabledLayerCount = 0;
  device_create_info.ppEnabledLayerNames = NULL;

  // Validation layer stuff
#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    device_create_info.enabledLayerCount =
        (uint32_t)ARRAYSIZE(RE_REQUIRED_VALIDATION_LAYERS);
    device_create_info.ppEnabledLayerNames = RE_REQUIRED_VALIDATION_LAYERS;
  }
#endif

  device_create_info.enabledExtensionCount =
      (uint32_t)ARRAYSIZE(RE_REQUIRED_DEVICE_EXTENSIONS);
  device_create_info.ppEnabledExtensionNames = RE_REQUIRED_DEVICE_EXTENSIONS;

  // Enable all features
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(ctx->physical_device, &features);
  device_create_info.pEnabledFeatures = &features;

  VK_CHECK(vkCreateDevice(
      ctx->physical_device, &device_create_info, NULL, &ctx->device));
}

static inline void get_device_queues(re_context_t *ctx) {
  vkGetDeviceQueue(
      ctx->device, ctx->graphics_queue_family_index, 0, &ctx->graphics_queue);
  vkGetDeviceQueue(
      ctx->device, ctx->present_queue_family_index, 0, &ctx->present_queue);
  vkGetDeviceQueue(
      ctx->device, ctx->transfer_queue_family_index, 0, &ctx->transfer_queue);
}

static inline void setup_memory_allocator(re_context_t *ctx) {
  VmaAllocatorCreateInfo allocator_info = {};
  allocator_info.physicalDevice = ctx->physical_device;
  allocator_info.device = ctx->device;

  VK_CHECK(vmaCreateAllocator(&allocator_info, &ctx->gpu_allocator));
}

static inline void create_graphics_command_pool(re_context_t *ctx) {
  VkCommandPoolCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create_info.pNext = 0;
  create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  create_info.queueFamilyIndex = ctx->graphics_queue_family_index;

  VK_CHECK(vkCreateCommandPool(
      ctx->device, &create_info, NULL, &ctx->graphics_command_pool));
}

static inline void create_transient_command_pool(re_context_t *ctx) {
  VkCommandPoolCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create_info.pNext = 0;
  create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  create_info.queueFamilyIndex = ctx->graphics_queue_family_index;

  VK_CHECK(vkCreateCommandPool(
      ctx->device, &create_info, NULL, &ctx->transient_command_pool));
}

static inline void create_thread_command_pools(re_context_t *ctx) {
  for (uint32_t i = 0; i < RE_THREAD_COUNT; i++) {
    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.pNext = 0;
    create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    create_info.queueFamilyIndex = ctx->graphics_queue_family_index;

    VK_CHECK(vkCreateCommandPool(
        ctx->device, &create_info, NULL, &ctx->thread_command_pools[i]));
  }
}

void re_context_pre_init(
    re_context_t *ctx,
    const char *const *required_window_vulkan_extensions,
    uint32_t window_extension_count) {
  ctx->instance = VK_NULL_HANDLE;
  ctx->device = VK_NULL_HANDLE;
  ctx->physical_device = VK_NULL_HANDLE;
  ctx->debug_callback = VK_NULL_HANDLE;

  ctx->graphics_queue_family_index = -1;
  ctx->present_queue_family_index = -1;
  ctx->transfer_queue_family_index = -1;
  ctx->graphics_queue = VK_NULL_HANDLE;
  ctx->present_queue = VK_NULL_HANDLE;
  ctx->transfer_queue = VK_NULL_HANDLE;

  ctx->gpu_allocator = VK_NULL_HANDLE;
  ctx->graphics_command_pool = VK_NULL_HANDLE;
  ctx->transient_command_pool = VK_NULL_HANDLE;

  ctx->descriptor_pool = VK_NULL_HANDLE;

  fstd_mutex_init(&ctx->queue_mutex);

  for (uint32_t i = 0; i < ARRAYSIZE(ctx->thread_command_pools); i++) {
    ctx->thread_command_pools[i] = VK_NULL_HANDLE;
  }

  create_instance(
      ctx, required_window_vulkan_extensions, window_extension_count);
#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    setup_debug_callback(ctx);
  }
#endif
}

void re_context_late_init(re_context_t *ctx, VkSurfaceKHR surface) {
  if (ctx->device != VK_NULL_HANDLE) {
    return;
  }

  create_device(ctx, surface);

  get_device_queues(ctx);

  setup_memory_allocator(ctx);

  create_graphics_command_pool(ctx);
  create_transient_command_pool(ctx);
  create_thread_command_pools(ctx);

  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
  };

  VkDescriptorPoolCreateInfo create_info = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,     // sType
      NULL,                                              // pNext
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, // flags
      1000 * (uint32_t)ARRAYSIZE(pool_sizes),            // maxSets
      (uint32_t)ARRAYSIZE(pool_sizes),                   // poolSizeCount
      pool_sizes,                                        // pPoolSizes
  };

  VK_CHECK(vkCreateDescriptorPool(
      g_ctx.device, &create_info, NULL, &ctx->descriptor_pool));

  {
    VkDescriptorSetLayoutBinding single_texture_bindings[] = {{
        0,                                         // binding
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
        1,                                         // descriptorCount
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
        NULL, // pImmutableSamplers
    }};

    VkDescriptorSetLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.bindingCount = ARRAYSIZE(single_texture_bindings);
    create_info.pBindings = single_texture_bindings;

    vkCreateDescriptorSetLayout(
        g_ctx.device, &create_info, NULL, &g_ctx.canvas_descriptor_set_layout);
  }

  uint8_t white[] = {255, 255, 255, 255};
  uint8_t black[] = {0, 0, 0, 255};
  re_texture_init(
      &ctx->white_texture, white, sizeof(white), 1, 1, RE_FORMAT_RGBA8_UNORM);
  re_texture_init(
      &ctx->black_texture, black, sizeof(black), 1, 1, RE_FORMAT_RGBA8_UNORM);
}

VkSampleCountFlagBits re_context_get_max_sample_count(re_context_t *ctx) {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(ctx->physical_device, &properties);

  VkSampleCountFlags color_samples =
      properties.limits.framebufferColorSampleCounts;
  VkSampleCountFlags depth_samples =
      properties.limits.framebufferDepthSampleCounts;

  VkSampleCountFlags counts =
      color_samples < depth_samples ? color_samples : depth_samples;

  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    ut_log_debug("Max samples: %d", 64);
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    ut_log_debug("Max samples: %d", 32);
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    ut_log_debug("Max samples: %d", 16);
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    ut_log_debug("Max samples: %d", 8);
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    ut_log_debug("Max samples: %d", 4);
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    ut_log_debug("Max samples: %d", 2);
    return VK_SAMPLE_COUNT_2_BIT;
  }

  ut_log_debug("Max samples: %d", 1);

  return VK_SAMPLE_COUNT_1_BIT;
}

bool re_context_get_supported_depth_format(
    re_context_t *ctx, VkFormat *depthFormat) {
  VkFormat depth_formats[] = {VK_FORMAT_D32_SFLOAT_S8_UINT,
                              VK_FORMAT_D32_SFLOAT,
                              VK_FORMAT_D24_UNORM_S8_UINT,
                              VK_FORMAT_D16_UNORM_S8_UINT,
                              VK_FORMAT_D16_UNORM};
  for (uint32_t i = 0; i < ARRAYSIZE(depth_formats); i++) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(
        ctx->physical_device, depth_formats[i], &formatProps);

    if (formatProps.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      *depthFormat = depth_formats[i];
      return depth_formats[i];
      return true;
    }
  }
  return false;
}

void re_context_destroy(re_context_t *ctx) {
  ut_log_debug("Vulkan context shutting down...");

  VK_CHECK(vkDeviceWaitIdle(ctx->device));

  re_texture_destroy(&ctx->white_texture);
  re_texture_destroy(&ctx->black_texture);

  vkDestroyDescriptorPool(ctx->device, ctx->descriptor_pool, NULL);

  vkDestroyDescriptorSetLayout(
      g_ctx.device, g_ctx.canvas_descriptor_set_layout, NULL);

  vkDestroyCommandPool(ctx->device, ctx->transient_command_pool, NULL);

  vkDestroyCommandPool(ctx->device, ctx->graphics_command_pool, NULL);

  for (uint32_t i = 0; i < ARRAYSIZE(ctx->thread_command_pools); i++) {
    vkDestroyCommandPool(ctx->device, ctx->thread_command_pools[i], NULL);
  }

  vmaDestroyAllocator(ctx->gpu_allocator);

  vkDestroyDevice(ctx->device, NULL);

  DestroyDebugReportCallbackEXT(ctx->instance, ctx->debug_callback, NULL);

  vkDestroyInstance(ctx->instance, NULL);

  fstd_mutex_destroy(&ctx->queue_mutex);

  SDL_Quit();
}
