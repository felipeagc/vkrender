#include "context.h"
#include "util.h"
#include "window.h"
#include <fstd_util.h>
#include <string.h>

re_context_t g_ctx;

#ifdef RE_ENABLE_VALIDATION
static const char *const RE_REQUIRED_VALIDATION_LAYERS[] = {
    "VK_LAYER_LUNARG_standard_validation",
};
#endif

static const char *const RE_REQUIRED_DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

// Debug callback

#ifdef RE_ENABLE_VALIDATION
// Ignore warnings for this function
#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char *layerPrefix,
    const char *msg,
    void *userData) {
  RE_LOG_ERROR("Validation layer: %s", msg);

  return VK_FALSE;
}
#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

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
#endif

void glfw_error_callback(int error, const char *description) {
  RE_LOG_ERROR("GLFW error (%d): %s", error, description);
}

#ifdef RE_ENABLE_VALIDATION
static inline bool check_validation_layer_support() {
  uint32_t count = 0;
  vkEnumerateInstanceLayerProperties(&count, NULL);
  VkLayerProperties *available_layers =
      (VkLayerProperties *)malloc(sizeof(VkLayerProperties) * count);
  vkEnumerateInstanceLayerProperties(&count, available_layers);

  for (uint32_t l = 0; l < ARRAY_SIZE(RE_REQUIRED_VALIDATION_LAYERS); l++) {
    const char *layer_name = RE_REQUIRED_VALIDATION_LAYERS[l];
    bool layer_found       = false;

    for (uint32_t i = 0; i < count; i++) {
      if (strcmp(available_layers[i].layerName, layer_name) == 0) {
        layer_found = true;
        break;
      }
    }

    if (!layer_found) {
      free(available_layers);
      return false;
    }
  }

  free(available_layers);
  return true;
}
#endif

static inline void get_required_extensions(
    const char **out_extensions, uint32_t *out_extension_count) {
  uint32_t glfw_extension_count;
  const char **glfw_extensions =
      glfwGetRequiredInstanceExtensions(&glfw_extension_count);
  *out_extension_count = glfw_extension_count;

#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    (*out_extension_count)++;
  }
#endif

  if (out_extensions == NULL) {
    return;
  }

  for (uint32_t i = 0; i < glfw_extension_count; i++) {
    out_extensions[i] = glfw_extensions[i];
  }

#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    out_extensions[glfw_extension_count] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
  }
#endif
}

static inline bool check_physical_device_properties(
    VkInstance instance,
    VkPhysicalDevice physical_device,
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

  for (uint32_t i = 0; i < ARRAY_SIZE(RE_REQUIRED_DEVICE_EXTENSIONS); i++) {
    const char *required_extension = RE_REQUIRED_DEVICE_EXTENSIONS[i];
    bool found                     = false;
    for (uint32_t i = 0; i < extension_count; i++) {
      if (strcmp(required_extension, available_extensions[i].extensionName) ==
          0) {
        found = true;
      }
    }

    if (!found) {
      RE_LOG_WARN(
          "Physical device %s doesn't support extension named \"%s\"",
          device_properties.deviceName,
          required_extension);
      free(available_extensions);
      return false;
    }
  }

  free(available_extensions);

  uint32_t major_version = VK_VERSION_MAJOR(device_properties.apiVersion);
  uint32_t minor_version = VK_VERSION_MINOR(device_properties.apiVersion);
  // uint32_t patch_version = VK_VERSION_PATCH(device_properties.apiVersion);

  if (major_version < 1 || minor_version < 1 ||
      device_properties.limits.maxImageDimension2D < 4096) {
    RE_LOG_WARN(
        "Physical device %s doesn't support required parameters!",
        device_properties.deviceName);
    return false;
  }

  // Check for required device features
  VkPhysicalDeviceFeatures features = {0};
  vkGetPhysicalDeviceFeatures(physical_device, &features);
  if (!features.wideLines) {
    RE_LOG_WARN(
        "Physical device %s doesn't support required features!",
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

  VkBool32 *queue_transfer_support =
      malloc(sizeof(*queue_transfer_support) * queue_family_prop_count);
  int *queue_present_support =
      malloc(sizeof(*queue_present_support) * queue_family_prop_count);

  uint32_t graphics_queue_family_index = UINT32_MAX;
  uint32_t present_queue_family_index  = UINT32_MAX;
  uint32_t transfer_queue_family_index = UINT32_MAX;

  // @TODO: figure out better logic to single out a transfer queue

  for (uint32_t i = 0; i < queue_family_prop_count; i++) {
    /* VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR( */
    /*     physical_device, i, surface, &queue_present_support[i])); */

    queue_present_support[i] =
        glfwGetPhysicalDevicePresentationSupport(instance, physical_device, i);

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
        *present_queue_family  = i;

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

  printf("%u\n", graphics_queue_family_index);
  printf("%u\n", present_queue_family_index);
  printf("%u\n", transfer_queue_family_index);

  if (graphics_queue_family_index == UINT32_MAX ||
      present_queue_family_index == UINT32_MAX ||
      transfer_queue_family_index == UINT32_MAX) {
    RE_LOG_WARN(
        "Could not find queue family with requested properties on physical "
        "device %s",
        device_properties.deviceName);

    free(queue_family_properties);
    free(queue_present_support);
    free(queue_transfer_support);
    return false;
  }

  *graphics_queue_family = graphics_queue_family_index;
  *present_queue_family  = present_queue_family_index;
  *transfer_queue_family = transfer_queue_family_index;

  free(queue_family_properties);
  free(queue_present_support);
  free(queue_transfer_support);

  return true;
}

static inline void create_instance(re_context_t *ctx) {
  RE_LOG_DEBUG("Creating vulkan instance");
#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    RE_LOG_DEBUG("Using validation layers");
  } else {
    RE_LOG_DEBUG("Validation layers requested but not available");
  }
#else
  RE_LOG_DEBUG("Not using validation layers");
#endif

  VkApplicationInfo appInfo  = {0};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext              = NULL;
  appInfo.pApplicationName   = "App";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 1, 0);
  appInfo.pEngineName        = "No engine";
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 1, 0);
  appInfo.apiVersion         = VK_API_VERSION_1_1;

  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.flags                = 0;
  createInfo.pApplicationInfo     = &appInfo;

  createInfo.enabledLayerCount   = 0;
  createInfo.ppEnabledLayerNames = NULL;

#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    createInfo.enabledLayerCount =
        (uint32_t)ARRAY_SIZE(RE_REQUIRED_VALIDATION_LAYERS);
    createInfo.ppEnabledLayerNames = RE_REQUIRED_VALIDATION_LAYERS;
  }
#endif

  // Set required instance extensions
  uint32_t extension_count = 0;
  get_required_extensions(NULL, &extension_count);
  const char **extensions = malloc(sizeof(*extensions) * extension_count);
  get_required_extensions(extensions, &extension_count);
  createInfo.enabledExtensionCount   = extension_count;
  createInfo.ppEnabledExtensionNames = extensions;

  VK_CHECK(vkCreateInstance(&createInfo, NULL, &ctx->instance));

  free((void *)extensions);
}

#ifdef RE_ENABLE_VALIDATION
static inline void setup_debug_callback(re_context_t *ctx) {
  VkDebugReportCallbackCreateInfoEXT createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  createInfo.flags =
      VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  createInfo.pfnCallback = debug_callback;

  VK_CHECK(CreateDebugReportCallbackEXT(
      ctx->instance, &createInfo, NULL, &ctx->debug_callback));
}
#endif

static inline void create_device(re_context_t *ctx) {
  uint32_t physical_device_count;
  vkEnumeratePhysicalDevices(ctx->instance, &physical_device_count, NULL);
  VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)malloc(
      sizeof(VkPhysicalDevice) * physical_device_count);
  vkEnumeratePhysicalDevices(
      ctx->instance, &physical_device_count, physical_devices);

  for (uint32_t i = 0; i < physical_device_count; i++) {
    if (check_physical_device_properties(
            ctx->instance,
            physical_devices[i],
            &ctx->graphics_queue_family_index,
            &ctx->present_queue_family_index,
            &ctx->transfer_queue_family_index)) {
      ctx->physical_device = physical_devices[i];
      break;
    }
  }

  free(physical_devices);

  if (!ctx->physical_device) {
    RE_LOG_FATAL("Could not select physical device based on chosen properties");
    abort();
  }

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(ctx->physical_device, &properties);

  RE_LOG_DEBUG("Using physical device: %s", properties.deviceName);

  uint32_t queue_create_info_count              = 0;
  VkDeviceQueueCreateInfo queue_create_infos[3] = {0};
  float queue_priorities[]                      = {1.0f};

  queue_create_infos[queue_create_info_count++] = (VkDeviceQueueCreateInfo){
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      NULL,
      0,
      ctx->graphics_queue_family_index,
      (uint32_t)ARRAY_SIZE(queue_priorities),
      queue_priorities,
  };

  if (ctx->present_queue_family_index != ctx->graphics_queue_family_index) {
    queue_create_infos[queue_create_info_count++] = (VkDeviceQueueCreateInfo){
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        NULL,
        0,
        ctx->present_queue_family_index,
        (uint32_t)ARRAY_SIZE(queue_priorities),
        queue_priorities,
    };
  }

  if (ctx->transfer_queue_family_index != ctx->graphics_queue_family_index) {
    queue_create_infos[queue_create_info_count++] = (VkDeviceQueueCreateInfo){
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        NULL,
        0,
        ctx->transfer_queue_family_index,
        (uint32_t)ARRAY_SIZE(queue_priorities),
        queue_priorities,
    };
  }

  VkDeviceCreateInfo device_create_info = {0};
  device_create_info.sType              = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.flags              = 0;
  device_create_info.queueCreateInfoCount = queue_create_info_count;
  device_create_info.pQueueCreateInfos    = queue_create_infos;

  device_create_info.enabledLayerCount   = 0;
  device_create_info.ppEnabledLayerNames = NULL;

  // Validation layer stuff
#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    device_create_info.enabledLayerCount =
        (uint32_t)ARRAY_SIZE(RE_REQUIRED_VALIDATION_LAYERS);
    device_create_info.ppEnabledLayerNames = RE_REQUIRED_VALIDATION_LAYERS;
  }
#endif

  device_create_info.enabledExtensionCount =
      (uint32_t)ARRAY_SIZE(RE_REQUIRED_DEVICE_EXTENSIONS);
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

static inline void get_device_limits(re_context_t *ctx) {
  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(ctx->physical_device, &props);

  ctx->physical_limits = props.limits;
}

static inline void setup_memory_allocator(re_context_t *ctx) {
  VmaAllocatorCreateInfo allocator_info = {0};
  allocator_info.physicalDevice         = ctx->physical_device;
  allocator_info.device                 = ctx->device;

  allocator_info.pVulkanFunctions = &(VmaVulkanFunctions) {
    .vkGetPhysicalDeviceProperties       = vkGetPhysicalDeviceProperties,
    .vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
    .vkAllocateMemory = vkAllocateMemory, .vkFreeMemory = vkFreeMemory,
    .vkMapMemory = vkMapMemory, .vkUnmapMemory = vkUnmapMemory,
    .vkFlushMappedMemoryRanges      = vkFlushMappedMemoryRanges,
    .vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
    .vkBindBufferMemory             = vkBindBufferMemory,
    .vkBindImageMemory              = vkBindImageMemory,
    .vkGetBufferMemoryRequirements  = vkGetBufferMemoryRequirements,
    .vkGetImageMemoryRequirements   = vkGetImageMemoryRequirements,
    .vkCreateBuffer = vkCreateBuffer, .vkDestroyBuffer = vkDestroyBuffer,
    .vkCreateImage = vkCreateImage, .vkDestroyImage = vkDestroyImage,
    .vkCmdCopyBuffer = vkCmdCopyBuffer,
#if VMA_DEDICATED_ALLOCATION
    .vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR,
    .vkGetImageMemoryRequirements2KHR  = vkGetImageMemoryRequirements2KHR,
#endif
  };

  VK_CHECK(vmaCreateAllocator(&allocator_info, &ctx->gpu_allocator));
}

static inline void create_graphics_command_pool(re_context_t *ctx) {
  VkCommandPoolCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create_info.pNext = 0;
  create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  create_info.queueFamilyIndex = ctx->graphics_queue_family_index;

  VK_CHECK(vkCreateCommandPool(
      ctx->device, &create_info, NULL, &ctx->graphics_command_pool));
}

static inline void create_transient_command_pool(re_context_t *ctx) {
  VkCommandPoolCreateInfo create_info = {0};
  create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create_info.pNext            = 0;
  create_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  create_info.queueFamilyIndex = ctx->graphics_queue_family_index;

  VK_CHECK(vkCreateCommandPool(
      ctx->device, &create_info, NULL, &ctx->transient_command_pool));
}

void re_ctx_init() {
  VK_CHECK(volkInitialize());

  glfwInit();
  glfwSetErrorCallback(glfw_error_callback);

  g_ctx.instance        = VK_NULL_HANDLE;
  g_ctx.device          = VK_NULL_HANDLE;
  g_ctx.physical_device = VK_NULL_HANDLE;
  g_ctx.debug_callback  = VK_NULL_HANDLE;

  g_ctx.graphics_queue_family_index = -1;
  g_ctx.present_queue_family_index  = -1;
  g_ctx.transfer_queue_family_index = -1;
  g_ctx.graphics_queue              = VK_NULL_HANDLE;
  g_ctx.present_queue               = VK_NULL_HANDLE;
  g_ctx.transfer_queue              = VK_NULL_HANDLE;

  g_ctx.gpu_allocator          = VK_NULL_HANDLE;
  g_ctx.graphics_command_pool  = VK_NULL_HANDLE;
  g_ctx.transient_command_pool = VK_NULL_HANDLE;

  g_ctx.descriptor_pool = VK_NULL_HANDLE;

  g_ctx.descriptor_set_allocator_count = 0;
  g_ctx.descriptor_set_allocators      = NULL;

  mtx_init(&g_ctx.queue_mutex, mtx_plain);

  create_instance(&g_ctx);
  volkLoadInstance(g_ctx.instance);
#ifdef RE_ENABLE_VALIDATION
  if (check_validation_layer_support()) {
    setup_debug_callback(&g_ctx);
  }
#endif

  create_device(&g_ctx);
  volkLoadDevice(g_ctx.device);

  get_device_queues(&g_ctx);

  get_device_limits(&g_ctx);

  setup_memory_allocator(&g_ctx);

  create_graphics_command_pool(&g_ctx);
  create_transient_command_pool(&g_ctx);

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
      1000 * (uint32_t)ARRAY_SIZE(pool_sizes),           // maxSets
      (uint32_t)ARRAY_SIZE(pool_sizes),                  // poolSizeCount
      pool_sizes,                                        // pPoolSizes
  };

  VK_CHECK(vkCreateDescriptorPool(
      g_ctx.device, &create_info, NULL, &g_ctx.descriptor_pool));

  {
    VkDescriptorSetLayoutBinding single_texture_bindings[] = {{
        0,                                         // binding
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
        1,                                         // descriptorCount
        VK_SHADER_STAGE_FRAGMENT_BIT,              // stageFlags
        NULL,                                      // pImmutableSamplers
    }};

    VkDescriptorSetLayoutCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.bindingCount = ARRAY_SIZE(single_texture_bindings);
    create_info.pBindings    = single_texture_bindings;

    vkCreateDescriptorSetLayout(
        g_ctx.device, &create_info, NULL, &g_ctx.canvas_descriptor_set_layout);
  }

  re_buffer_pool_init(
      &g_ctx.ubo_pool,
      &(re_buffer_options_t){
          .usage  = RE_BUFFER_USAGE_UNIFORM,
          .memory = RE_BUFFER_MEMORY_HOST,
          .size   = 1 << 16, // 65k blocks
      });

  g_ctx.descriptor_set_allocators = calloc(
      RE_MAX_DESCRIPTOR_SET_ALLOCATORS, sizeof(re_descriptor_set_allocator_t));
}

void re_ctx_destroy() {
  RE_LOG_DEBUG("Vulkan context shutting down...");

  VK_CHECK(vkDeviceWaitIdle(g_ctx.device));

  for (uint32_t i = 0; i < g_ctx.descriptor_set_allocator_count; i++) {
    re_descriptor_set_allocator_destroy(&g_ctx.descriptor_set_allocators[i]);
  }

  if (g_ctx.descriptor_set_allocators != NULL) {
    free(g_ctx.descriptor_set_allocators);
  }

  re_buffer_pool_destroy(&g_ctx.ubo_pool);

  vkDestroyDescriptorPool(g_ctx.device, g_ctx.descriptor_pool, NULL);

  vkDestroyDescriptorSetLayout(
      g_ctx.device, g_ctx.canvas_descriptor_set_layout, NULL);

  vkDestroyCommandPool(g_ctx.device, g_ctx.transient_command_pool, NULL);

  vkDestroyCommandPool(g_ctx.device, g_ctx.graphics_command_pool, NULL);

  vmaDestroyAllocator(g_ctx.gpu_allocator);

  vkDestroyDevice(g_ctx.device, NULL);

#ifdef RE_ENABLE_VALIDATION
  DestroyDebugReportCallbackEXT(g_ctx.instance, g_ctx.debug_callback, NULL);
#endif

  vkDestroyInstance(g_ctx.instance, NULL);

  mtx_destroy(&g_ctx.queue_mutex);

  glfwTerminate();
}

void re_ctx_begin_frame() {
  for (uint32_t i = 0; i < g_ctx.descriptor_set_allocator_count; i++) {
    re_descriptor_set_allocator_begin_frame(
        &g_ctx.descriptor_set_allocators[i]);
  }

  re_buffer_pool_begin_frame(&g_ctx.ubo_pool);
}

re_descriptor_set_allocator_t *
rx_ctx_request_descriptor_set_allocator(re_descriptor_set_layout_t layout) {
  for (uint32_t i = 0; i < g_ctx.descriptor_set_allocator_count; i++) {
    if (memcmp(
            &g_ctx.descriptor_set_allocators[i].layout,
            &layout,
            sizeof(layout)) == 0) {
      return &g_ctx.descriptor_set_allocators[i];
    }
  }

  g_ctx.descriptor_set_allocator_count++;

  assert(
      g_ctx.descriptor_set_allocator_count <= RE_MAX_DESCRIPTOR_SET_ALLOCATORS);

  re_descriptor_set_allocator_init(
      &g_ctx
           .descriptor_set_allocators[g_ctx.descriptor_set_allocator_count - 1],
      layout);

  return &g_ctx.descriptor_set_allocators
              [g_ctx.descriptor_set_allocator_count - 1];
}

VkSampleCountFlagBits re_ctx_get_max_sample_count() {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(g_ctx.physical_device, &properties);

  VkSampleCountFlags color_samples =
      properties.limits.framebufferColorSampleCounts;
  VkSampleCountFlags depth_samples =
      properties.limits.framebufferDepthSampleCounts;

  VkSampleCountFlags counts =
      color_samples < depth_samples ? color_samples : depth_samples;

  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    RE_LOG_DEBUG("Max samples: %d", 64);
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    RE_LOG_DEBUG("Max samples: %d", 32);
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    RE_LOG_DEBUG("Max samples: %d", 16);
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    RE_LOG_DEBUG("Max samples: %d", 8);
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    RE_LOG_DEBUG("Max samples: %d", 4);
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    RE_LOG_DEBUG("Max samples: %d", 2);
    return VK_SAMPLE_COUNT_2_BIT;
  }

  RE_LOG_DEBUG("Max samples: %d", 1);

  return VK_SAMPLE_COUNT_1_BIT;
}

bool re_ctx_get_supported_depth_format(VkFormat *depth_format) {
  VkFormat depth_formats[] = {VK_FORMAT_D24_UNORM_S8_UINT,
                              VK_FORMAT_D32_SFLOAT_S8_UINT,
                              VK_FORMAT_D16_UNORM_S8_UINT,
                              VK_FORMAT_D32_SFLOAT,
                              VK_FORMAT_D16_UNORM};
  for (uint32_t i = 0; i < ARRAY_SIZE(depth_formats); i++) {
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(
        g_ctx.physical_device, depth_formats[i], &format_props);

    if (format_props.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      *depth_format = depth_formats[i];
      return depth_formats[i];
      return true;
    }
  }
  return false;
}
