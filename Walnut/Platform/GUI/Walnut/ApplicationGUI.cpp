#include "ApplicationGUI.h"

#include "Walnut/Core/Log.h"

//
// Adapted from Dear ImGui Vulkan example
//

#include "imgui.h"
#include "imgui_internal.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <stdio.h>  // printf, fprintf
#include <stdlib.h> // abort
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "ImGui/ImGuiTheme.h"

#include "stb_image.h"

#include <iostream>

// Emedded font
#include "ImGui/Roboto-Bold.embed"
#include "ImGui/Roboto-Italic.embed"
#include "ImGui/Roboto-Regular.embed"

extern bool g_ApplicationRunning;

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to
// maximize ease of testing and compatibility with old VS compilers. To link
// with VS2010-era libraries, VS2015+ requires linking with
// legacy_stdio_definitions.lib, which we do using this pragma. Your own project
// should not be affected, as you are likely to link with a newer binary of GLFW
// that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) &&                                 \
    !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// #define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

static VkAllocationCallbacks *g_Allocator = NULL;
static VkInstance g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice g_Device = VK_NULL_HANDLE;
static uint32_t g_QueueFamily = (uint32_t)-1;
static VkQueue g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
static VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
static VkSampleCountFlagBits g_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t g_MinImageCount = 2;
static bool g_SwapChainRebuild = false;
static VkImageUsageFlags g_SwapChainImageUsage =
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

// Per-frame-in-flight
static std::vector<std::vector<VkCommandBuffer>> s_AllocatedCommandBuffers;
static std::vector<std::vector<std::function<void()>>> s_ResourceFreeQueue;

static VkCommandBuffer s_ActiveCommandBuffer = nullptr;

// Unlike g_MainWindowData.FrameIndex, this is not the the swapchain image index
// and is always guaranteed to increase (eg. 0, 1, 2, 0, 1, 2)
static uint32_t s_CurrentFrameIndex = 0;

static std::unordered_map<std::string, ImFont *> s_Fonts;

static Walnut::Application *s_Instance = nullptr;

void check_vk_result(VkResult err) {
  if (err == 0)
    return;
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0)
    abort();
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
             uint64_t object, size_t location, int32_t messageCode,
             const char *pLayerPrefix, const char *pMessage, void *pUserData) {
  (void)flags;
  (void)object;
  (void)location;
  (void)messageCode;
  (void)pUserData;
  (void)pLayerPrefix; // Unused arguments
  fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n",
          objectType, pMessage);
  return VK_FALSE;
}
#endif // APP_USE_VULKAN_DEBUG_REPORT

static bool
IsExtensionAvailable(const ImVector<VkExtensionProperties> &properties,
                     const char *extension) {
  for (const VkExtensionProperties &p : properties)
    if (strcmp(p.extensionName, extension) == 0)
      return true;
  return false;
}

static void SetupVulkan(std::vector<const char *> &instance_extensions) {
  VkResult err;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
  volkInitialize();
#endif

  // Create Vulkan Instance
  {
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    // Enumerate available extensions
    uint32_t properties_count;
    ImVector<VkExtensionProperties> properties;
    vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
    properties.resize(properties_count);
    err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count,
                                                 properties.Data);
    check_vk_result(err);

    // Enable required extensions
    if (IsExtensionAvailable(
            properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
      instance_extensions.push_back(
          VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    if (IsExtensionAvailable(properties,
                             VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
      instance_extensions.push_back(
          VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
#endif

    // Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
    const char *layers[] = {"VK_LAYER_KHRONOS_validation"};
    create_info.enabledLayerCount = 1;
    create_info.ppEnabledLayerNames = layers;
    instance_extensions.push_back("VK_EXT_debug_report");
#endif

    // Create Vulkan Instance
    create_info.enabledExtensionCount = (uint32_t)instance_extensions.size();
    create_info.ppEnabledExtensionNames = instance_extensions.data();
    err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
    check_vk_result(err);
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkLoadInstance(g_Instance);
#endif

    // Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
    auto f_vkCreateDebugReportCallbackEXT =
        (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
            g_Instance, "vkCreateDebugReportCallbackEXT");
    IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
    VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
    debug_report_ci.sType =
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                            VK_DEBUG_REPORT_WARNING_BIT_EXT |
                            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    debug_report_ci.pfnCallback = debug_report;
    debug_report_ci.pUserData = nullptr;
    err = f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci,
                                           g_Allocator, &g_DebugReport);
    check_vk_result(err);
#endif
  }

  // Select Physical Device (GPU)
  g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance);
  IM_ASSERT(g_PhysicalDevice != VK_NULL_HANDLE);

  // MSAA
  {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(g_PhysicalDevice, &physicalDeviceProperties);
    VkSampleCountFlags counts =
        physicalDeviceProperties.limits.framebufferColorSampleCounts &
        physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_4_BIT) {
      g_MsaaSamples = VK_SAMPLE_COUNT_4_BIT;
    } else {
      std::cout << "[UNOPTIMISED] MSAA not supported" << std::endl;
      g_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;
    }
  }

  // Select graphics queue family
  g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);
  IM_ASSERT(g_QueueFamily != (uint32_t)-1);

  // Create Logical Device (with 1 queue)
  {
    ImVector<const char *> device_extensions;
    device_extensions.push_back("VK_KHR_swapchain");

    // Enumerate physical device extension
    uint32_t properties_count;
    ImVector<VkExtensionProperties> properties;
    vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr,
                                         &properties_count, nullptr);
    properties.resize(properties_count);
    vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr,
                                         &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
    if (IsExtensionAvailable(properties,
                             VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
      device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    const float queue_priority[] = {1.0f};
    VkDeviceQueueCreateInfo queue_info[1] = {};
    queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[0].queueFamilyIndex = g_QueueFamily;
    queue_info[0].queueCount = 1;
    queue_info[0].pQueuePriorities = queue_priority;
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount =
        sizeof(queue_info) / sizeof(queue_info[0]);
    create_info.pQueueCreateInfos = queue_info;
    create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
    create_info.ppEnabledExtensionNames = device_extensions.Data;
    err =
        vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
    check_vk_result(err);
    vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
  }

  // Create Descriptor Pool
  // If you wish to load e.g. additional textures you may need to alter pools
  // sizes and maxSets.
  {
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 0;
    for (VkDescriptorPoolSize &pool_size : pool_sizes)
      pool_info.maxSets += pool_size.descriptorCount;
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator,
                                 &g_DescriptorPool);
    check_vk_result(err);
  }
}

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used
// by the demo. Your real engine/app may not use them.
static void SetupVulkanWindow(ImGui_ImplVulkanH_Window *wd,
                              VkSurfaceKHR surface, int width, int height) {
  wd->Surface = surface;

  // Check for WSI support
  VkBool32 res;
  vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily,
                                       wd->Surface, &res);
  if (res != VK_TRUE) {
    fprintf(stderr, "Error no WSI support on physical device 0\n");
    exit(-1);
  }

  // Select Surface Format
  const VkFormat requestSurfaceImageFormat[] = {
      VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
      VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
  const VkColorSpaceKHR requestSurfaceColorSpace =
      VK_COLORSPACE_SRGB_NONLINEAR_KHR;
  wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
      g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat,
      (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
      requestSurfaceColorSpace);

  // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
  VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR,
                                      VK_PRESENT_MODE_IMMEDIATE_KHR,
                                      VK_PRESENT_MODE_FIFO_KHR};
#else
  VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif
  wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
      g_PhysicalDevice, wd->Surface, &present_modes[0],
      IM_ARRAYSIZE(present_modes));
  // printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

  // Create SwapChain, RenderPass, Framebuffer, etc.
  IM_ASSERT(g_MinImageCount >= 2);
  ImGui_ImplVulkanH_CreateOrResizeWindow(
      g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator,
      width, height, g_MinImageCount, g_SwapChainImageUsage);
}

static void CleanupVulkan() {
  vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
  // Remove the debug report callback
  auto vkDestroyDebugReportCallbackEXT =
      (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
          g_Instance, "vkDestroyDebugReportCallbackEXT");
  vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // IMGUI_VULKAN_DEBUG_REPORT

  vkDestroyDevice(g_Device, g_Allocator);
  vkDestroyInstance(g_Instance, g_Allocator);
}

static void CleanupVulkanWindow() {
  ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData,
                                  g_Allocator);
}

static void FrameRender(Walnut::Application *application,
                        ImGui_ImplVulkanH_Window *wd, ImDrawData *draw_data) {
  VkResult err;

  VkSemaphore image_acquired_semaphore =
      wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
  VkSemaphore render_complete_semaphore =
      wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX,
                              image_acquired_semaphore, VK_NULL_HANDLE,
                              &wd->FrameIndex);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
    g_SwapChainRebuild = true;
  if (err == VK_ERROR_OUT_OF_DATE_KHR)
    return;
  if (err != VK_SUBOPTIMAL_KHR)
    check_vk_result(err);

  s_CurrentFrameIndex = (s_CurrentFrameIndex + 1) % g_MainWindowData.ImageCount;

  ImGui_ImplVulkanH_Frame *fd = &wd->Frames[wd->FrameIndex];
  {
    err = vkWaitForFences(
        g_Device, 1, &fd->Fence, VK_TRUE,
        UINT64_MAX); // wait indefinitely instead of periodically checking
    check_vk_result(err);

    err = vkResetFences(g_Device, 1, &fd->Fence);
    check_vk_result(err);
  }

  {
    // Free resources in queue
    for (auto &func : s_ResourceFreeQueue[s_CurrentFrameIndex])
      func();
    s_ResourceFreeQueue[s_CurrentFrameIndex].clear();
  }
  {
    // Free command buffers allocated by Application::GetCommandBuffer
    // These use g_MainWindowData.FrameIndex and not s_CurrentFrameIndex because
    // they're tied to the swapchain image index
    auto &allocatedCommandBuffers = s_AllocatedCommandBuffers[wd->FrameIndex];
    if (allocatedCommandBuffers.size() > 0) {
      vkFreeCommandBuffers(g_Device, fd->CommandPool,
                           (uint32_t)allocatedCommandBuffers.size(),
                           allocatedCommandBuffers.data());
      allocatedCommandBuffers.clear();
    }

    err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
    check_vk_result(err);
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
    s_ActiveCommandBuffer = fd->CommandBuffer;
    check_vk_result(err);
  }
  {
    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = wd->RenderPass;
    info.framebuffer = fd->Framebuffer;
    info.renderArea.extent.width = wd->Width;
    info.renderArea.extent.height = wd->Height;
    info.clearValueCount = 1;
    info.pClearValues = &wd->ClearValue;
    vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  for (auto &layer : application->GetLayerStack())
    layer->OnRender();

  // Record dear imgui primitives into command buffer
  ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

  // Submit command buffer
  vkCmdEndRenderPass(fd->CommandBuffer);
  {
    VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &image_acquired_semaphore;
    info.pWaitDstStageMask = &wait_stage;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &fd->CommandBuffer;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &render_complete_semaphore;

    err = vkEndCommandBuffer(fd->CommandBuffer);
    s_ActiveCommandBuffer = nullptr;
    check_vk_result(err);
    err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
    check_vk_result(err);
  }
}

static void FramePresent(ImGui_ImplVulkanH_Window *wd) {
  if (g_SwapChainRebuild)
    return;
  VkSemaphore render_complete_semaphore =
      wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  VkPresentInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = &render_complete_semaphore;
  info.swapchainCount = 1;
  info.pSwapchains = &wd->Swapchain;
  info.pImageIndices = &wd->FrameIndex;
  VkResult err = vkQueuePresentKHR(g_Queue, &info);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    g_SwapChainRebuild = true;
    return;
  }
  check_vk_result(err);
  wd->SemaphoreIndex =
      (wd->SemaphoreIndex + 1) %
      wd->ImageCount; // Now we can use the next set of semaphores
}

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

namespace Walnut {

#include "Walnut/Embed/Walnut-Icon.embed"
#include "Walnut/Embed/WindowImages.embed"

Application::Application(const ApplicationSpecification &specification)
    : m_Specification(specification) {
  s_Instance = this;

  Init();
}

Application::~Application() {
  Shutdown();

  s_Instance = nullptr;
}

Application &Application::Get() { return *s_Instance; }

void Application::Init() {
  // Intialize logging
  Log::Init();

  // Setup GLFW window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) {
    std::cerr << "Could not initalize GLFW!\n";
    return;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
  bool is_wayland = glfwGetPlatform() == GLFW_PLATFORM_WAYLAND;
  float main_scale = // HACK: Values for the laptop
      is_wayland ? 0.8f
                 : ImGui_ImplGlfw_GetContentScaleForMonitor(primaryMonitor);
  const GLFWvidmode *videoMode = glfwGetVideoMode(primaryMonitor);

  int monitorX, monitorY;
  glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);

  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  m_WindowHandle =
      glfwCreateWindow(static_cast<int>(m_Specification.Width * main_scale),
                       static_cast<int>(m_Specification.Height * main_scale),
                       m_Specification.Name.c_str(), NULL, NULL);

  if (m_Specification.CenterWindow) {
    glfwSetWindowPos(m_WindowHandle,
                     monitorX + (videoMode->width - m_Specification.Width) / 2,
                     monitorY +
                         (videoMode->height - m_Specification.Height) / 2);

    glfwSetWindowAttrib(m_WindowHandle, GLFW_RESIZABLE,
                        m_Specification.WindowResizeable ? GLFW_TRUE
                                                         : GLFW_FALSE);
  }

  glfwShowWindow(m_WindowHandle);

  // Setup Vulkan
  if (!glfwVulkanSupported()) {
    std::cerr << "GLFW: Vulkan not supported!\n";
    return;
  }

  // Set icon
  GLFWimage icon;
  int channels;
  if (!m_Specification.IconPath.empty()) {
    std::string iconPathStr = m_Specification.IconPath.string();
    icon.pixels =
        stbi_load(iconPathStr.c_str(), &icon.width, &icon.height, &channels, 4);
    glfwSetWindowIcon(m_WindowHandle, 1, &icon);
    stbi_image_free(icon.pixels);
  }

  glfwSetWindowUserPointer(m_WindowHandle, this);

  std::vector<const char *> extensions;
  uint32_t glfw_num_extension = 0;
  const char **glfw_extensions =
      glfwGetRequiredInstanceExtensions(&glfw_num_extension);
  extensions.insert(extensions.begin(), glfw_extensions,
                    glfw_extensions + glfw_num_extension);
  SetupVulkan(extensions);

  // Create Window Surface
  VkSurfaceKHR surface;
  VkResult err = glfwCreateWindowSurface(g_Instance, m_WindowHandle,
                                         g_Allocator, &surface);
  check_vk_result(err);

  // Create Framebuffers
  int w, h;
  glfwGetFramebufferSize(m_WindowHandle, &w, &h);
  ImGui_ImplVulkanH_Window *wd = &g_MainWindowData;
  SetupVulkanWindow(wd, surface, w, h);

  s_AllocatedCommandBuffers.resize(wd->ImageCount);
  s_ResourceFreeQueue.resize(wd->ImageCount);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad
  // Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
                                                    // Waylaaaaaaand
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable
  // Multi-Viewport / Platform Windows
  // io.ConfigViewportsNoAutoMerge = true;
  // io.ConfigViewportsNoTaskBarIcon = true;

  // Theme colors
  UI::SetHazelTheme();

  // Style
  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowPadding = ImVec2(10.0f, 10.0f);
  style.FramePadding = ImVec2(8.0f, 6.0f);
  style.ItemSpacing = ImVec2(6.0f, 6.0f);
  style.ChildRounding = 6.0f;
  style.PopupRounding = 6.0f;
  style.FrameRounding = 6.0f;
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
  // Bake a fixed style scale. (until we have a solution for dynamic style
  // scaling, changing this requires resetting Style + calling this again)
  style.ScaleAllSizes(main_scale);
  // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this
  // unnecessary. We leave both here for documentation purpose)
  style.FontScaleDpi = main_scale;
  // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when
  // Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding
  // for now.
  io.ConfigDpiScaleFonts = !is_wayland;
  // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI
  // changes.
  io.ConfigDpiScaleViewports = !is_wayland;

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform
  // windows can look identical to regular ones.
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForVulkan(m_WindowHandle, true);
  ImGui_ImplVulkan_InitInfo init_info = {
      .ApiVersion = VK_VERSION_1_3,
      .Instance = g_Instance,
      .PhysicalDevice = g_PhysicalDevice,
      .Device = g_Device,
      .QueueFamily = g_QueueFamily,
      .Queue = g_Queue,
      .DescriptorPool = g_DescriptorPool,
      .MinImageCount = g_MinImageCount,
      .ImageCount = wd->ImageCount,
      .PipelineCache = g_PipelineCache,
      .PipelineInfoMain = {.RenderPass = wd->RenderPass,
                           .Subpass = 0,
                           .MSAASamples = g_MsaaSamples},
      .Allocator = g_Allocator,
      .CheckVkResultFn = check_vk_result,
  };
  ImGui_ImplVulkan_Init(&init_info);

  // Load default font
  ImFontConfig fontConfig;
  fontConfig.FontDataOwnedByAtlas = false;
  ImFont *robotoFont = io.Fonts->AddFontFromMemoryTTF(
      (void *)g_RobotoRegular, sizeof(g_RobotoRegular), 20.0f, &fontConfig);
  s_Fonts["Default"] = robotoFont;
  s_Fonts["Bold"] = io.Fonts->AddFontFromMemoryTTF(
      (void *)g_RobotoBold, sizeof(g_RobotoBold), 20.0f, &fontConfig);
  s_Fonts["Italic"] = io.Fonts->AddFontFromMemoryTTF(
      (void *)g_RobotoItalic, sizeof(g_RobotoItalic), 20.0f, &fontConfig);
  io.FontDefault = robotoFont;

  // Load images
  {
    uint32_t w, h;
    void *data = Image::Decode(g_WalnutIcon, sizeof(g_WalnutIcon), w, h);
    m_AppHeaderIcon =
        std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
    free(data);
  }
  {
    uint32_t w, h;
    void *data =
        Image::Decode(g_WindowMinimizeIcon, sizeof(g_WindowMinimizeIcon), w, h);
    m_IconMinimize =
        std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
    free(data);
  }
  {
    uint32_t w, h;
    void *data =
        Image::Decode(g_WindowMaximizeIcon, sizeof(g_WindowMaximizeIcon), w, h);
    m_IconMaximize =
        std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
    free(data);
  }
  {
    uint32_t w, h;
    void *data =
        Image::Decode(g_WindowRestoreIcon, sizeof(g_WindowRestoreIcon), w, h);
    m_IconRestore =
        std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
    free(data);
  }
  {
    uint32_t w, h;
    void *data =
        Image::Decode(g_WindowCloseIcon, sizeof(g_WindowCloseIcon), w, h);
    m_IconClose =
        std::make_shared<Walnut::Image>(w, h, ImageFormat::RGBA, data);
    free(data);
  }
} // namespace Walnut

void Application::Shutdown() {
  for (auto &layer : m_LayerStack)
    layer->OnDetach();

  m_LayerStack.clear();

  // Release resources
  // NOTE(Yan): to avoid doing this manually, we shouldn't
  //            store resources in this Application class
  m_AppHeaderIcon.reset();
  m_IconClose.reset();
  m_IconMinimize.reset();
  m_IconMaximize.reset();
  m_IconRestore.reset();

  // Cleanup
  VkResult err = vkDeviceWaitIdle(g_Device);
  check_vk_result(err);

  // Free resources in queue
  for (auto &queue : s_ResourceFreeQueue) {
    for (auto &func : queue)
      func();
  }
  s_ResourceFreeQueue.clear();

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  CleanupVulkanWindow();
  CleanupVulkan();

  glfwDestroyWindow(m_WindowHandle);
  glfwTerminate();

  g_ApplicationRunning = false;

  Log::Shutdown();
}

void Application::Run() {
  m_Running = true;

  ImGui_ImplVulkanH_Window *wd = &g_MainWindowData;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  ImGuiIO &io = ImGui::GetIO();

  // Main loop
  while (!glfwWindowShouldClose(m_WindowHandle) && m_Running) {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application. Generally you may always pass all inputs
    // to dear imgui, and hide them from your application based on those two
    // flags.
    glfwPollEvents();

    {
      std::scoped_lock<std::mutex> lock(m_EventQueueMutex);

      // Process custom event queue
      while (m_EventQueue.size() > 0) {
        auto &func = m_EventQueue.front();
        func();
        m_EventQueue.pop();
      }
    }

    for (auto &layer : m_LayerStack)
      layer->OnUpdate(m_TimeStep);

    // Resize swap chain?
    int fb_width, fb_height;
    glfwGetFramebufferSize(m_WindowHandle, &fb_width, &fb_height);
    if (fb_width > 0 && fb_height > 0 &&
        (g_SwapChainRebuild || g_MainWindowData.Width != fb_width ||
         g_MainWindowData.Height != fb_height)) {
      ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
      ImGui_ImplVulkanH_CreateOrResizeWindow(
          g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily,
          g_Allocator, fb_width, fb_height, g_MinImageCount, 0);
      g_MainWindowData.FrameIndex = 0;

      // Clear allocated command buffers from here since entire pool is
      // destroyed
      s_AllocatedCommandBuffers.clear();
      s_AllocatedCommandBuffers.resize(g_MainWindowData.ImageCount);

      g_SwapChainRebuild = false;
    }
    if (glfwGetWindowAttrib(m_WindowHandle, GLFW_ICONIFIED) != 0) {
      ImGui_ImplGlfw_Sleep(10);
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (m_Specification.WindowDockSpace)
      ImGui::DockSpaceOverViewport();

    for (auto &layer : m_LayerStack)
      layer->OnUIRender();

    // Rendering
    ImGui::Render();
    ImDrawData *main_draw_data = ImGui::GetDrawData();
    const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f ||
                                    main_draw_data->DisplaySize.y <= 0.0f);
    wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
    wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
    wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
    wd->ClearValue.color.float32[3] = clear_color.w;
    if (!main_is_minimized)
      FrameRender(this, wd, main_draw_data);

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
    }

    // Present Main Platform Window
    if (!main_is_minimized)
      FramePresent(wd);
    else
      std::this_thread::sleep_for(std::chrono::milliseconds(5));

    float time = GetTime();
    m_FrameTime = time - m_LastFrameTime;
    m_TimeStep = glm::min<float>(m_FrameTime, 0.0333f);
    m_LastFrameTime = time;
  }
}

void Application::SetMenubarCallback(
    const std::function<void()> &menubarCallback) {
  m_MenubarCallback = menubarCallback;
}

void Application::Close() { m_Running = false; }

bool Application::IsMaximized() const {
  return (bool)glfwGetWindowAttrib(m_WindowHandle, GLFW_MAXIMIZED);
}

float Application::GetTime() { return (float)glfwGetTime(); }

VkInstance Application::GetInstance() { return g_Instance; }

VkPhysicalDevice Application::GetPhysicalDevice() { return g_PhysicalDevice; }

VkDevice Application::GetDevice() { return g_Device; }

VkCommandBuffer Application::GetCommandBuffer(bool begin) {
  ImGui_ImplVulkanH_Window *wd = &g_MainWindowData;

  // Use any command queue
  VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;

  VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
  cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdBufAllocateInfo.commandPool = command_pool;
  cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdBufAllocateInfo.commandBufferCount = 1;

  VkCommandBuffer &command_buffer =
      s_AllocatedCommandBuffers[wd->FrameIndex].emplace_back();
  auto err =
      vkAllocateCommandBuffers(g_Device, &cmdBufAllocateInfo, &command_buffer);

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  err = vkBeginCommandBuffer(command_buffer, &begin_info);
  check_vk_result(err);

  return command_buffer;
}

void Application::FlushCommandBuffer(VkCommandBuffer commandBuffer) {
  const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;

  VkSubmitInfo end_info = {};
  end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  end_info.commandBufferCount = 1;
  end_info.pCommandBuffers = &commandBuffer;
  auto err = vkEndCommandBuffer(commandBuffer);
  check_vk_result(err);

  // Create fence to ensure that the command buffer has finished executing
  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = 0;
  VkFence fence;
  err = vkCreateFence(g_Device, &fenceCreateInfo, nullptr, &fence);
  check_vk_result(err);

  err = vkQueueSubmit(g_Queue, 1, &end_info, fence);
  check_vk_result(err);

  err = vkWaitForFences(g_Device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
  check_vk_result(err);

  vkDestroyFence(g_Device, fence, nullptr);
}

void Application::SubmitResourceFree(std::function<void()> &&func) {
  s_ResourceFreeQueue[s_CurrentFrameIndex].emplace_back(func);
}

ImFont *Application::GetFont(const std::string &name) {
  if (auto it = s_Fonts.find(name); it != s_Fonts.end())
    return it->second;

  return nullptr;
}

ImGui_ImplVulkanH_Window *Application::GetMainWindowData() {
  return &g_MainWindowData;
}

VkCommandBuffer Application::GetActiveCommandBuffer() {
  return s_ActiveCommandBuffer;
}

} // namespace Walnut
