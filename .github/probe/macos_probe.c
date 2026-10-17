// Probe of GitHub's macOS runner, before the macOS port. Through GLFW, as the engine: opens a window, creates a Vulkan
// instance (MoltenVK, portability enumeration), surface, device and swapchain, presents a few cleared frames, then
// creates OpenGL contexts (through GLFW, then CGL directly). Prints what it finds, the support of the device features the
// engine enables, and exits with 1 when Vulkan fails.

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Negative VkResults are errors; positive ones (VK_SUBOPTIMAL_KHR, VK_INCOMPLETE) are printed and accepted
#define CHECK(call)                                                          \
    do {                                                                     \
        VkResult result_ = (call);                                           \
        if (result_ < 0) {                                                   \
            printf("FAILED: %s -> VkResult %d\n", #call, (int)result_);      \
            return 1;                                                        \
        }                                                                    \
        if (result_ != VK_SUCCESS)                                           \
            printf("Note: %s -> VkResult %d\n", #call, (int)result_);        \
    } while (0)

#define MAX_IMAGES 8

static void ErrorCallback(int code, const char* text)
{
    printf("GLFW error 0x%X: %s\n", code, text);
}

static void PrintWindow(GLFWwindow* window)
{
    int width, height, framebufferWidth, framebufferHeight;
    float scaleX, scaleY;
    glfwGetWindowSize(window, &width, &height);
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glfwGetWindowContentScale(window, &scaleX, &scaleY);
    printf("Window: %dx%d, framebuffer %dx%d, content scale %.2f x %.2f, visible %d, focused %d\n",
        width, height, framebufferWidth, framebufferHeight, scaleX, scaleY,
        glfwGetWindowAttrib(window, GLFW_VISIBLE), glfwGetWindowAttrib(window, GLFW_FOCUSED));
}

static int HasLayer(const char* name)
{
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, NULL);
    VkLayerProperties* layers = malloc(sizeof(VkLayerProperties) * (count ? count : 1));
    vkEnumerateInstanceLayerProperties(&count, layers);
    int found = 0;
    for (uint32_t i = 0; i < count; i++)
        if (strcmp(layers[i].layerName, name) == 0)
            found = 1;
    free(layers);
    return found;
}

static int ProbeVulkan(void)
{
    printf("\n== Vulkan\n");
    if (!glfwVulkanSupported())
    {
        printf("FAILED: glfwVulkanSupported() is false (no Vulkan loader or driver found)\n");
        return 1;
    }

    uint32_t loaderVersion = 0;
    vkEnumerateInstanceVersion(&loaderVersion);
    printf("Loader: Vulkan %u.%u.%u\n", VK_API_VERSION_MAJOR(loaderVersion), VK_API_VERSION_MINOR(loaderVersion),
        VK_API_VERSION_PATCH(loaderVersion));

    // The engine's instance: GLFW's extensions, portability enumeration (MoltenVK isn't a conformant driver, the loader
    // hides it without this), validation when available
    const char* extensions[16];
    uint32_t extensionCount = 0;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount && extensionCount < 14; i++)
    {
        printf("GLFW instance extension: %s\n", glfwExtensions[i]);
        extensions[extensionCount++] = glfwExtensions[i];
    }
    extensions[extensionCount++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    extensions[extensionCount++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;

    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    int validation = HasLayer(validationLayer);
    printf("Validation layer: %s\n", validation ? "enabled" : "not found");

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "MoravaEngine macOS probe";
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instanceInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = extensions;
    instanceInfo.enabledLayerCount = validation ? 1 : 0;
    instanceInfo.ppEnabledLayerNames = &validationLayer;
    VkInstance instance;
    CHECK(vkCreateInstance(&instanceInfo, NULL, &instance));

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (deviceCount == 0)
    {
        printf("FAILED: no Vulkan device\n");
        return 1;
    }
    VkPhysicalDevice devices[4];
    if (deviceCount > 4)
        deviceCount = 4;
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    for (uint32_t i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(devices[i], &properties);
        printf("Device %u: %s, Vulkan %u.%u.%u, type %d, max image 2D %u, bound descriptor sets %u\n", i,
            properties.deviceName, VK_API_VERSION_MAJOR(properties.apiVersion), VK_API_VERSION_MINOR(properties.apiVersion),
            VK_API_VERSION_PATCH(properties.apiVersion), (int)properties.deviceType, properties.limits.maxImageDimension2D,
            properties.limits.maxBoundDescriptorSets);
    }
    VkPhysicalDevice physicalDevice = devices[0];

    // The features VulkanContextH2M enables, and two the renderers may want
    VkPhysicalDeviceFeatures supported;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supported);
    printf("Feature samplerAnisotropy: %d\n", supported.samplerAnisotropy);
    printf("Feature wideLines: %d\n", supported.wideLines);
    printf("Feature fillModeNonSolid: %d\n", supported.fillModeNonSolid);
    printf("Feature pipelineStatisticsQuery: %d\n", supported.pipelineStatisticsQuery);
    printf("Feature imageCubeArray: %d\n", supported.imageCubeArray);
    printf("Feature geometryShader: %d\n", supported.geometryShader);
    printf("Feature shaderStorageImageWriteWithoutFormat: %d\n", supported.shaderStorageImageWriteWithoutFormat);

    // Device extensions the engine uses or will need
    uint32_t deviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &deviceExtensionCount, NULL);
    VkExtensionProperties* deviceExtensionList = malloc(sizeof(VkExtensionProperties) * (deviceExtensionCount ? deviceExtensionCount : 1));
    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &deviceExtensionCount, deviceExtensionList);
    const char* wanted[] = { "VK_KHR_swapchain", "VK_KHR_portability_subset", "VK_EXT_debug_marker", "VK_KHR_acceleration_structure",
        "VK_KHR_ray_query", "VK_KHR_ray_tracing_pipeline", "VK_KHR_deferred_host_operations", "VK_EXT_descriptor_indexing",
        "VK_KHR_buffer_device_address" };
    int hasPortabilitySubset = 0;
    for (size_t w = 0; w < sizeof(wanted) / sizeof(wanted[0]); w++)
    {
        int found = 0;
        for (uint32_t i = 0; i < deviceExtensionCount; i++)
            if (strcmp(deviceExtensionList[i].extensionName, wanted[w]) == 0)
                found = 1;
        printf("Device extension %s: %d\n", wanted[w], found);
        if (w == 1)
            hasPortabilitySubset = found;
    }
    printf("Device extensions in all: %u\n", deviceExtensionCount);
    free(deviceExtensionList);

    // Window and surface
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "MoravaEngine macOS probe (Vulkan)", NULL, NULL);
    if (!window)
    {
        printf("FAILED: no window\n");
        return 1;
    }
    PrintWindow(window);
    VkSurfaceKHR surface;
    CHECK(glfwCreateWindowSurface(instance, window, NULL, &surface));

    // A queue family with graphics and present
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, NULL);
    VkQueueFamilyProperties families[16];
    if (familyCount > 16)
        familyCount = 16;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families);
    uint32_t family = UINT32_MAX;
    for (uint32_t i = 0; i < familyCount; i++)
    {
        VkBool32 present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &present);
        printf("Queue family %u: flags 0x%X, %u queues, present %d\n", i, families[i].queueFlags, families[i].queueCount, present);
        if (family == UINT32_MAX && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present)
            family = i;
    }
    if (family == UINT32_MAX)
    {
        printf("FAILED: no queue family with graphics and present\n");
        return 1;
    }

    // Device: the engine's features, as far as they are supported (an unsupported one would fail the creation)
    VkPhysicalDeviceFeatures enabled = { 0 };
    enabled.samplerAnisotropy = supported.samplerAnisotropy;
    enabled.wideLines = supported.wideLines;
    enabled.fillModeNonSolid = supported.fillModeNonSolid;
    enabled.pipelineStatisticsQuery = supported.pipelineStatisticsQuery;
    enabled.imageCubeArray = supported.imageCubeArray;

    float priority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = family;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    // VK_KHR_portability_subset must be enabled when the device has it
    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset" };
    VkDeviceCreateInfo deviceInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = hasPortabilitySubset ? 2 : 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceInfo.pEnabledFeatures = &enabled;
    VkDevice device;
    CHECK(vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device));
    VkQueue queue;
    vkGetDeviceQueue(device, family, 0, &queue);
    printf("Device created\n");

    // Swapchain
    VkSurfaceCapabilitiesKHR capabilities;
    CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities));
    printf("Surface: images %u-%u, current extent %ux%u, usage 0x%X, composite alpha 0x%X\n", capabilities.minImageCount,
        capabilities.maxImageCount, capabilities.currentExtent.width, capabilities.currentExtent.height,
        capabilities.supportedUsageFlags, capabilities.supportedCompositeAlpha);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
    VkSurfaceFormatKHR formats[32];
    if (formatCount > 32)
        formatCount = 32;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);
    for (uint32_t i = 0; i < formatCount; i++)
        printf("Surface format: %d, color space %d\n", (int)formats[i].format, (int)formats[i].colorSpace);
    if (formatCount == 0)
    {
        printf("FAILED: no surface format\n");
        return 1;
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
    VkPresentModeKHR presentModes[8];
    if (presentModeCount > 8)
        presentModeCount = 8;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);
    for (uint32_t i = 0; i < presentModeCount; i++)
        printf("Present mode: %d\n", (int)presentModes[i]);

    if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
    {
        printf("FAILED: swapchain images can't be cleared (no TRANSFER_DST usage)\n");
        return 1;
    }

    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == UINT32_MAX)
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        extent.width = (uint32_t)width;
        extent.height = (uint32_t)height;
    }
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = formats[0].format;
    swapchainInfo.imageColorSpace = formats[0].colorSpace;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = capabilities.currentTransform;
    swapchainInfo.compositeAlpha = (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
        : (VkCompositeAlphaFlagBitsKHR)(capabilities.supportedCompositeAlpha & -capabilities.supportedCompositeAlpha);
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;
    VkSwapchainKHR swapchain;
    CHECK(vkCreateSwapchainKHR(device, &swapchainInfo, NULL, &swapchain));

    uint32_t swapchainImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
    VkImage images[MAX_IMAGES];
    if (swapchainImageCount > MAX_IMAGES)
        swapchainImageCount = MAX_IMAGES;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, images);
    printf("Swapchain: %u images, %ux%u\n", swapchainImageCount, extent.width, extent.height);

    // A few frames: clear the acquired image and present it
    VkCommandPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    poolInfo.queueFamilyIndex = family;
    VkCommandPool pool;
    CHECK(vkCreateCommandPool(device, &poolInfo, NULL, &pool));

    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = pool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));

    // One "rendered" semaphore per swapchain image: presentation may still wait on the previous one
    VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkSemaphore acquired;
    VkSemaphore rendered[MAX_IMAGES];
    CHECK(vkCreateSemaphore(device, &semaphoreInfo, NULL, &acquired));
    for (uint32_t i = 0; i < swapchainImageCount; i++)
        CHECK(vkCreateSemaphore(device, &semaphoreInfo, NULL, &rendered[i]));

    const int frameCount = 60;
    for (int frame = 0; frame < frameCount; frame++)
    {
        uint32_t imageIndex;
        CHECK(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquired, VK_NULL_HANDLE, &imageIndex));

        VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = images[imageIndex];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL,
            1, &barrier);

        VkClearColorValue color = { { 0.1f, 0.3f + 0.5f * (float)frame / frameCount, 0.6f, 1.0f } };
        vkCmdClearColorImage(commandBuffer, images[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1,
            &barrier.subresourceRange);

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
            NULL, 1, &barrier);
        CHECK(vkEndCommandBuffer(commandBuffer));

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &acquired;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &rendered[imageIndex];
        CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

        VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &rendered[imageIndex];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;
        CHECK(vkQueuePresentKHR(queue, &presentInfo));

        // Simple and slow: the next frame reuses the command buffer
        CHECK(vkQueueWaitIdle(queue));
        CHECK(vkResetCommandPool(device, pool, 0));
        glfwPollEvents();
    }
    printf("Vulkan: %d frames presented\n", frameCount);
    PrintWindow(window);

    CHECK(vkDeviceWaitIdle(device));
    for (uint32_t i = 0; i < swapchainImageCount; i++)
        vkDestroySemaphore(device, rendered[i], NULL);
    vkDestroySemaphore(device, acquired, NULL);
    vkDestroyCommandPool(device, pool, NULL);
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    printf("Vulkan: everything destroyed\n");
    return 0;
}

static int ProbeOpenGL(int major, int minor)
{
    printf("\n== OpenGL %d.%d core\n", major, minor);
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(640, 360, "MoravaEngine macOS probe (OpenGL)", NULL, NULL);
    if (!window)
    {
        printf("OpenGL %d.%d core: no context\n", major, minor);
        return 1;
    }
    glfwMakeContextCurrent(window);
    printf("GL_VERSION: %s\n", (const char*)glGetString(GL_VERSION));
    printf("GL_RENDERER: %s\n", (const char*)glGetString(GL_RENDERER));
    printf("GL_VENDOR: %s\n", (const char*)glGetString(GL_VENDOR));
    printf("GL_SHADING_LANGUAGE_VERSION: %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    for (int frame = 0; frame < 10; frame++)
    {
        glClearColor(0.6f, 0.3f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    printf("OpenGL: glGetError 0x%X after 10 frames\n", glGetError());
    PrintWindow(window);
    glfwDestroyWindow(window);
    return 0;
}

#if defined(__APPLE__)
// GLFW always asks for an accelerated pixel format. CGL directly: the renderers macOS has, then a 4.1 core context from
// an accelerated renderer and from Apple's software renderer
static void ProbeCGL(void)
{
    printf("\n== CGL renderers\n");
    CGLRendererInfoObj info;
    GLint rendererCount = 0;
    if (CGLQueryRendererInfo(0xFFFFFFFF, &info, &rendererCount) != kCGLNoError)
    {
        printf("CGLQueryRendererInfo failed\n");
        return;
    }
    for (GLint i = 0; i < rendererCount; i++)
    {
        GLint id = 0, accelerated = 0, online = 0, majorVersion = 0;
        CGLDescribeRenderer(info, i, kCGLRPRendererID, &id);
        CGLDescribeRenderer(info, i, kCGLRPAccelerated, &accelerated);
        CGLDescribeRenderer(info, i, kCGLRPOnline, &online);
        CGLDescribeRenderer(info, i, kCGLRPMajorGLVersion, &majorVersion);
        printf("Renderer %d: id 0x%X, accelerated %d, online %d, OpenGL major version %d\n", i, id, accelerated, online, majorVersion);
    }
    CGLDestroyRendererInfo(info);

    for (int software = 0; software < 2; software++)
    {
        const char* name = software ? "software renderer" : "accelerated renderer";
        CGLPixelFormatAttribute accelerated[] = { kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_GL4_Core,
            kCGLPFAAccelerated, kCGLPFAColorSize, (CGLPixelFormatAttribute)24, kCGLPFADepthSize, (CGLPixelFormatAttribute)24,
            (CGLPixelFormatAttribute)0 };
        CGLPixelFormatAttribute generic[] = { kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_GL4_Core,
            kCGLPFARendererID, (CGLPixelFormatAttribute)kCGLRendererGenericFloatID, kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
            kCGLPFADepthSize, (CGLPixelFormatAttribute)24, (CGLPixelFormatAttribute)0 };
        CGLPixelFormatObj pixelFormat = NULL;
        GLint formatCount = 0;
        CGLError error = CGLChoosePixelFormat(software ? generic : accelerated, &pixelFormat, &formatCount);
        if (error != kCGLNoError || !pixelFormat)
        {
            printf("CGL %s, 4.1 core: no pixel format (%s)\n", name, CGLErrorString(error));
            continue;
        }
        CGLContextObj context = NULL;
        error = CGLCreateContext(pixelFormat, NULL, &context);
        CGLDestroyPixelFormat(pixelFormat);
        if (error != kCGLNoError || !context)
        {
            printf("CGL %s, 4.1 core: no context (%s)\n", name, CGLErrorString(error));
            continue;
        }
        CGLSetCurrentContext(context);
        printf("CGL %s, 4.1 core: GL_VERSION %s, GL_RENDERER %s\n", name, (const char*)glGetString(GL_VERSION),
            (const char*)glGetString(GL_RENDERER));
        CGLSetCurrentContext(NULL);
        CGLDestroyContext(context);
    }
}
#endif

int main(void)
{
    glfwSetErrorCallback(ErrorCallback);

    // GLFW looks for libvulkan.1.dylib by itself only in the default library paths, not Homebrew's
    glfwInitVulkanLoader(vkGetInstanceProcAddr);
    if (!glfwInit())
    {
        printf("FAILED: glfwInit\n");
        return 1;
    }
    printf("GLFW %s, platform 0x%X\n", glfwGetVersionString(), glfwGetPlatform());

    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    for (int i = 0; i < monitorCount; i++)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
        float scaleX, scaleY;
        glfwGetMonitorContentScale(monitors[i], &scaleX, &scaleY);
        printf("Monitor %d: %s, %dx%d at %d Hz, content scale %.2f x %.2f\n", i, glfwGetMonitorName(monitors[i]),
            mode->width, mode->height, mode->refreshRate, scaleX, scaleY);
    }
    if (monitorCount == 0)
        printf("No monitor\n");

    int vulkanFailed = ProbeVulkan();
    int openGLFailed = ProbeOpenGL(4, 1);
    ProbeOpenGL(4, 5); // expected to fail: macOS stops at 4.1
#if defined(__APPLE__)
    ProbeCGL();
#endif

    glfwTerminate();
    // Only Vulkan fails the step: the runner's virtual GPU may have no OpenGL at all
    printf("\nResult: Vulkan %s, OpenGL 4.1 through GLFW %s\n", vulkanFailed ? "FAILED" : "ok", openGLFailed ? "not available" : "ok");
    return vulkanFailed ? 1 : 0;
}
