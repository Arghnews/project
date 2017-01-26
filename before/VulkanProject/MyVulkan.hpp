#ifndef MY_VULKAN_CLASS
#define MY_VULKAN_CLASS

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VDeleter.hpp"
#include <vector>
#include <queue>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <unordered_map>
#include <chrono>
#include <iostream>
#include <unistd.h>

#include "Helper.hpp"

struct SwapchainSupportDetails;
struct QueueFamilyIndices;

static VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

static void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

class HelloTriangleApplication {
	public:
		HelloTriangleApplication();
		void init(GLFWwindow* window, int WIDTH, int HEIGHT);
		VDeleter<VkDebugReportCallbackEXT> callback{instance, DestroyDebugReportCallbackEXT};
		void initVulkan();
		void updateUniformBuffer();
		void updateUniformBuffer(
				const glm::vec3 position, const glm::vec3 lookingAt,const glm::vec3 up
				);
		void drawFrame();
		void cleanup();
		GLFWwindow* window;
		static void onWindowResized(GLFWwindow* window, int width, int height);
		void recreateSwapchain();
		void setWindowSize(int wide, int high);

	private:
		int wide;
		int high;
		VDeleter<VkInstance> instance{vkDestroyInstance};

		// window sys extension - to interface with platform agnostic windowing sys
		VDeleter<VkSurfaceKHR> surface{instance, vkDestroySurfaceKHR};

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VDeleter<VkDevice> device{vkDestroyDevice}; // logical device

		VkQueue graphicsQueue; // implicitly destroyed when device is
		VkQueue presentQueue; // separate queues, potentially one for render one for presenting
		VkQueue transferQueue; // separate queues, potentially one for render one for presenting

		VDeleter<VkRenderPass> renderPass{device, vkDestroyRenderPass};

		// above pipelineLayout
		VDeleter<VkDescriptorSetLayout> descriptorSetLayout{device, vkDestroyDescriptorSetLayout};

		// uniform values in shader specified during pipeline creation
		// by this object that is made in createGraphicsPipeline()
		VDeleter<VkPipelineLayout> pipelineLayout{device, vkDestroyPipelineLayout};

		// swapchain must be after device so gets cleaned up before logical device
		VDeleter<VkSwapchainKHR> swapchain{device, vkDestroySwapchainKHR};
		// cleaned up once swap chain destroyed
		std::vector<VkImage> swapchainImages; // handles to swap chain images
		VkFormat swapchainImageFormat;
		VkExtent2D swapchainExtent;
		// views of images in swap chain, view being how to access what part of image
		std::vector<VDeleter<VkImageView>> swapchainImageViews;

		// the mega graphics pipeline
		VDeleter<VkPipeline> graphicsPipeline{device, vkDestroyPipeline};

		// framebuffers for each image in swapchain
		std::vector<VDeleter<VkFramebuffer>> swapchainFramebuffers;

		VDeleter<VkCommandPool> commandPool{device, vkDestroyCommandPool};
		// command pool for command buffers submitted on transfer queue family
		VDeleter<VkCommandPool> transferCommandPool{device, vkDestroyCommandPool};

		// texture image memory
		VDeleter<VkImage> textureImage{device, vkDestroyImage};
		VDeleter<VkDeviceMemory> textureImageMemory{device, vkFreeMemory};

		// command buffer per swap chain image, automatically freed when pool is
		std::vector<VkCommandBuffer> commandBuffers;

		// semaphores to sync
		// taking image from swapchain, executing command buffer on image in framebuffer
		// return image to swapchain for present, each of these are async calls
		// but need to be done one after the other
		VDeleter<VkSemaphore> imageAvailableSemaphore{device, vkDestroySemaphore};
		VDeleter<VkSemaphore> renderFinishedSemaphore{device, vkDestroySemaphore};

		VDeleter<VkDeviceMemory> vertexBufferMemory{device, vkFreeMemory};
		VDeleter<VkBuffer> vertexBuffer{device, vkDestroyBuffer};

		VDeleter<VkBuffer> indexBuffer{device, vkDestroyBuffer};
		VDeleter<VkDeviceMemory> indexBufferMemory{device, vkFreeMemory};

		// 
		VDeleter<VkBuffer> uniformStagingBuffer{device, vkDestroyBuffer};
		VDeleter<VkDeviceMemory> uniformStagingBufferMemory{device, vkFreeMemory};
		VDeleter<VkBuffer> uniformBuffer{device, vkDestroyBuffer};
		VDeleter<VkDeviceMemory> uniformBufferMemory{device, vkFreeMemory};

		VDeleter<VkDescriptorPool> descriptorPool{device, vkDestroyDescriptorPool};
		VkDescriptorSet descriptorSet;

		// texturing, this is for image
		VDeleter<VkImage> stagingImage{device, vkDestroyImage};
		VDeleter<VkDeviceMemory> stagingImageMemory{device, vkFreeMemory};

		VDeleter<VkImageView> textureImageView{device, vkDestroyImageView};
		VDeleter<VkSampler> textureSampler{device, vkDestroySampler};

		VDeleter<VkImage> depthImage{device, vkDestroyImage};
		VDeleter<VkDeviceMemory> depthImageMemory{device, vkFreeMemory};
		VDeleter<VkImageView> depthImageView{device, vkDestroyImageView};

		// glm uses floats
		//
		// decel formula for velo in x, x = x*x*worldDecelv - decelConst
		//glm::vec3 velocity;
		// as meaning x,y,z accel relative to current, slightly misleading care
		// ie y is into screen (forward/back), x is A/D left right


		double startOfApplicationTime; // should be set in constructor on creation

		bool hasStencilComponent(VkFormat format);
		VkFormat findDepthFormat();
		VkFormat findSupportedDepthFormat(const std::vector<VkFormat>& candidates, 
				VkImageTiling tiling, VkFormatFeatureFlags features);
		void createDepthResources();
		void createTextureSampler();
		void createTextureImageView();
		void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, 
				VDeleter<VkImageView>& imageView);
		void copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout,
				VkImageLayout newLayout);
		void createImage(uint32_t width, uint32_t height, VkFormat format, 
				VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
				VDeleter<VkImage>& image, VDeleter<VkDeviceMemory>& imageMemory);
		void createTextureImage();
		void createDescriptorSet();
		void createDescriptorPool();
		void createDescriptorSetLayout();
		void createUniformBuffer();
		void createIndexBuffer();
		void createBuffer(
				VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
				VDeleter<VkBuffer>& buffer, VDeleter<VkDeviceMemory>& bufferMemory);
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
				VDeleter<VkBuffer>& buffer, VDeleter<VkDeviceMemory>& bufferMemory, VkSharingMode sharingMode);
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void createVertexBuffer();
		void createSemaphores();
		void createCommandBuffers();
		void createCommandPool();
		void createFrameBuffers();
		void createRenderPass();
		void createShaderModule(const std::vector<char>& code, VDeleter<VkShaderModule>& shaderModule);
		void createGraphicsPipeline();
		void createImageViews();
		void createSwapchain();
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device);
		void createSurface();
		void createLogicalDevice();
		void pickPhysicalDevice();
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		bool isDeviceSuitable(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		void quit();
		void createInstance();
		void setupDebugCallback();
		std::vector<const char*> getRequiredExtensions();
		bool checkValidationLayerSupport();
		static std::vector<char> readFile(const std::string& filename);
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
};

struct QueueFamilyIndices {
	int graphicsFamily = -1;
	int presentFamily = -1;
	int transferFamily = -1;

	bool isComplete() {
		return graphicsFamily >= 0 && presentFamily >= 0 && transferFamily >= 0;
	}
};

// images in swap chain, res
// pixel format, colour space
// presentation modes
struct SwapchainSupportDetails { 
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	//glm::vec2 texCoord;

	// all in one array so only need 1 binding, binding param is index of binding in array
	// stride is size per unit, inputRate can be per {VERTEX,INSTANCE}
	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	// struct for pos/colour, struct describing how to get vert attribute from data from binding
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0; // which binding
		attributeDescriptions[0].location = 0; // location in vertex shader
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		// offset is a byte offset of this attribute relative
		// to the start of an element in the vertex input binding

		attributeDescriptions[1].binding = 0; // colour
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		// removed texture component for now
		//attributeDescriptions[2].binding = 0; // texture
		//attributeDescriptions[2].location = 2;
		//attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		//attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

};

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

// pos, colour, texture coords

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},

	{{-1.5f, -0.5f, -1.5f}, {0.0f, 0.0f, 1.0f}},
	{{1.5f, -0.5f, -1.5f}, {0.0f, 0.5f, 1.0f}},
	{{1.5f, 0.5f, -1.5f}, {0.0f, 0.0f, 1.0f}},
	{{-1.5f, 0.5f, -1.5f}, {1.0f, 1.0f, 1.0f}}
};

// can use 32 or 16 bit ints - using 32 because constant below
// just easier

const std::vector<uint32_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4,
	8, 9, 10,
	0, 5, 8
};


/*const std::vector<Vertex> vertices2 = {
  {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
  {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
  {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
  };
  */

/*const std::vector<Vertex> vertices = {
  {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
  {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
  {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
  {{-1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}},
  {{0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
  {{-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}
  };*/


#endif
