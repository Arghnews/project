#include "MyVulkan.hpp"

#include <stdexcept>
#include <algorithm>
#include <functional>
#include <cstring>
#include <iostream>
#include <fstream>
#include <set>
#include <array>
#include <queue>
#include <chrono>
#include <vector>
#include <math.h>
#include <string>
#include <thread>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VDeleter.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // vulkan range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Helper.hpp"
#include "Thing.hpp"
#include "Camera.hpp"

// using Swapchain to be consistent with Vulkan types, the tutorial uses SwapChain so
// when pasting code change this

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// inlining stop's gcc's bitching if unused - obviously should take out from final

HelloTriangleApplication::HelloTriangleApplication() {
	// constructor
}

void HelloTriangleApplication::init(GLFWwindow* window, int width, int height) {

	this->window = window;
	// initial place so can see the damn squares
	wide = width;
	high = height;
	// sets up map of keys down
	// assume for now start with no keys pressed

	startOfApplicationTime = time_now();

}

void HelloTriangleApplication::setWindowSize(int new_width, int new_height) {
	wide = new_width;
	high = new_height;
}

void HelloTriangleApplication::cleanup() {
	// wait for logical device to finish before closing
	vkDeviceWaitIdle(device);
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);
}

void HelloTriangleApplication::initVulkan() {
	createInstance();
	setupDebugCallback();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapchain();
	createImageViews();
	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();
	createCommandPool();
	createDepthResources();
	createFrameBuffers();
	//createTextureImage();
	//createTextureImageView();
	//createTextureSampler();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffer();
	createDescriptorPool();
	createDescriptorSet();
	createCommandBuffers();
	createSemaphores();
}

void HelloTriangleApplication::createUniformBuffer() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformStagingBuffer, uniformStagingBufferMemory);
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniformBuffer, uniformBufferMemory);
}


bool HelloTriangleApplication::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat HelloTriangleApplication::findDepthFormat() {
	return findSupportedDepthFormat(
			{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);
}

VkFormat HelloTriangleApplication::findSupportedDepthFormat(const std::vector<VkFormat>& candidates, 
		VkImageTiling tiling, VkFormatFeatureFlags features) {
	// finds best depth format from options given
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && 
				(props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && 
				(props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

void HelloTriangleApplication::createDepthResources() {

	VkFormat depthFormat = findDepthFormat();

	// creates image, don't need to map or copy another image to it
	// as cleared at started of render pass like color attachment
	createImage(swapchainExtent.width, swapchainExtent.height, depthFormat, 
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT,depthImageView);

	transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED,
		   	VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

}

void HelloTriangleApplication::createTextureSampler() {

	// mag/min filter - how to interpolate texels that are magnified/minified
	// mag - oversampling, min - undersampling
	// U,V,W -> X,Y,Z
	// address modes are like repeat, repeat mirrored, clamp to border etc
	// borderColor - when sampling beyond image w/ clamp
	// unnormalized - can use [0,1) range

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK ;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	if (vkCreateSampler(device, &samplerInfo, nullptr, textureSampler.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}

}

void HelloTriangleApplication::createTextureImageView() {
	createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT,textureImageView);
}

void HelloTriangleApplication::createImageView(VkImage image, VkFormat format, 
		VkImageAspectFlags aspectFlags, VDeleter<VkImageView>& imageView) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &viewInfo, nullptr, imageView.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}
}

void HelloTriangleApplication::copyImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) {
	// for copying the texture image from one buffer back to itself, but changing it's layout?

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageSubresourceLayers subResource = {};
	subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subResource.baseArrayLayer = 0;
	subResource.mipLevel = 0;
	subResource.layerCount = 1;

	VkImageCopy region = {};
	region.srcSubresource = subResource;
	region.dstSubresource = subResource;
	region.srcOffset = {0, 0, 0};
	region.dstOffset = {0, 0, 0};
	region.extent.width = width;
	region.extent.height = height;
	region.extent.depth = 1;

	vkCmdCopyImage(
			commandBuffer,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region
			);

	endSingleTimeCommands(commandBuffer);
}

void HelloTriangleApplication::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	// to handle layout transitions before copying texture image to card

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	// need to sync before editting buffer, with VK_SHARING_MODE_EXCLUSIVE 

	// layouts example - VK_IMAGE_LAYOUT_UNDEFINED
	// src/dstQueueFamilyIndex used if transferring ownership of queue family
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED 
			&& newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED 
			&& newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
			&& newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
			&& newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		   	VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	} else {
		throw std::invalid_argument("unsupported layout transition!");
	}


	vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			0, // or VK_DEPENDECY_BY_REGION_BIT, if !0 can read from already written
			0, nullptr, // memory barriers
			0, nullptr, // buffer memory barriers
			1, &barrier // image memory barriers
			);

	endSingleTimeCommands(commandBuffer);

}

void HelloTriangleApplication::createImage(uint32_t width, uint32_t height, VkFormat format, 
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
		VDeleter<VkImage>& image, VDeleter<VkDeviceMemory>& imageMemory) {

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;

	imageInfo.format = format;
	imageInfo.tiling = tiling;
	// LINEAR here is row major order, need like this to access, cannot change
	// for final will use TILING_OPTIMAL
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	// layout either undefined - first transition discards texels
	// or preinitialized first transition preserve texels

	// transfer source, like before
	imageInfo.usage = usage;
	// only used in one queue family, one that supports transfer
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // for multisampling if >1
	imageInfo.flags = 0; // Optional

	// could error if gpu does not support format - unlikely with rgba 8 bit unsigned
	if (vkCreateImage(device, &imageInfo, nullptr, image.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	// just like vKGetBufferMemoryRequirements etc 
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, imageMemory.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

void HelloTriangleApplication::createTextureImage() {
	// should be called after create command pool as creates command buffers
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/statue.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	VDeleter<VkImage> stagingImage{device, vkDestroyImage};
	VDeleter<VkDeviceMemory> stagingImageMemory{device, vkFreeMemory};
	createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage, stagingImageMemory);

	void* data;
	// map memory of stagingImageMemory to data
	vkMapMemory(device, stagingImageMemory, 0, imageSize, 0, &data);

	// copy pixels (the image) to data (which is effectively stagingImageMemory space)
	memcpy(data, pixels, (size_t) imageSize);
	vkUnmapMemory(device, stagingImageMemory);

	// free memory of the original texture image
	stbi_image_free(pixels);

	// final image, note different params like TILING OPTIMAL
	// want to be able to sample from this, also destination bit
	// device local memory
	createImage(
			texWidth, texHeight,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			textureImage,
			textureImageMemory
			);

	transitionImageLayout(stagingImage, VK_FORMAT_R8G8B8A8_UNORM, 
			VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, 
			VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyImage(stagingImage, textureImage, texWidth, texHeight);

	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// woohoo, image now loaded into gpu memory as texture

}

void HelloTriangleApplication::createDescriptorSet() {

	// uses the class member 'descriptorPool' to create a descriptor set

	VkDescriptorSetLayout layouts[] = {descriptorSetLayout};
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	// alloc one descriptor set with one uniform buffer descriptor
	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	/*
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImageView;
	imageInfo.sampler = textureSampler;
	*/

	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	/*
	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;
	*/

	vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

}

void HelloTriangleApplication::createDescriptorPool() {
	// descriptor sets must be allocated from a pool

	// describe descriptor types our sets are going to contain
	// 0 for ubo, 1 for texture object thing
	
	std::array<VkDescriptorPoolSize, 1> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;
	//poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//poolSizes[1].descriptorCount = 1;

	// descriptor pool
	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = poolSizes.size();
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, descriptorPool.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void HelloTriangleApplication::updateUniformBuffer(
	const glm::vec3 position,
	const glm::vec3 lookingAt,
	const glm::vec3 up
		) {
	//auto timeNow = std::chrono::high_resolution_clock::now();
	//float timeAbs = std::chrono::duration_cast<std::chrono::nanoseconds>(timeNow - startOfApplicationTime).count() / 1e9f;

	//Thing* cam = world->object("Camera");

	UniformBufferObject ubo = {};
	// rotate rotate $1 by $2 about $3, in this case about z axis
	
	// rotate rotate $1 by $2 about $3, in this case about z axis
	float rotateAngleTest = (float) ((long)time_now() % (long)startOfApplicationTime);
	rotateAngleTest /= 1e8f;
	
	ubo.model = glm::rotate(glm::mat4(), glm::radians(rotateAngleTest), glm::vec3(0.0f, 0.0f, 1.0f));

	// eye position, center - what you're looking at, up
	//ubo.view = glm::lookAt(cameraPos, cameraPos + lookingAt, up);
	// this way looking at is always relative to cameraPos
	ubo.view = glm::lookAt(position, position + lookingAt, up);

	//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), 
	//		glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));


	// FOV, aspect ratio, near clip plane, far clip plane
	ubo.proj = glm::perspective(glm::radians(90.0f), 
			swapchainExtent.width / (float) swapchainExtent.height, 0.1f, 90.0f);

	// because glm was for opengl where y coord is flipped compared to vulkan
	ubo.proj[1][1] *= -1;

	// copy data from uniform buffer object to uniform staging buffer
	void* data;
	vkMapMemory(device, uniformStagingBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformStagingBufferMemory);

	// copy from staging buffer to actual buffer - not best way to do this
	copyBuffer(uniformStagingBuffer, uniformBuffer, sizeof(ubo));

	// look up push constants to pass frequently changing data to shaders
}

void HelloTriangleApplication::createDescriptorSetLayout() {
	// binding for uniform shader
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0; // binding used in shader
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1; // number of values in array
	// combo of VkShaderStage flags or all, where descriptor referenced
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	/*
	// descriptor type here indicates going to use combined image sampler in fragment shader
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	*/

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = bindings.size();
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, descriptorSetLayout.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

}

void HelloTriangleApplication::createIndexBuffer() {

	// now may be 16 bit * size if using uint16
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VDeleter<VkBuffer> stagingBuffer{device, vkDestroyBuffer};
	VDeleter<VkDeviceMemory> stagingBufferMemory{device, vkFreeMemory};
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t) bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	// index buffer
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	// get buffer from gpu that has best memory type/perf for our ops
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	// find memory types
	// if mem type suitable for buffer with properties we need, return index
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}


void HelloTriangleApplication::createBuffer(
		VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VDeleter<VkBuffer>& buffer, VDeleter<VkDeviceMemory>& bufferMemory) {
	createBuffer(size,usage,properties,buffer,bufferMemory,VK_SHARING_MODE_EXCLUSIVE);
}

void HelloTriangleApplication::createBuffer(
		VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
		VDeleter<VkBuffer>& buffer, VDeleter<VkDeviceMemory>& bufferMemory, VkSharingMode sharingMode) {

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	// bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // buffer usage - vertex
	bufferInfo.sharingMode = sharingMode; // like images in Swapchain

	if (vkCreateBuffer(device, &bufferInfo, nullptr, buffer.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex buffer!");
	}

	VkMemoryRequirements memRequirements; // struct with fields
	// size of required mem in bytes
	// alignment, depends on bufferInfo.{usage,flags}
	// memoryTypeBits
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	// coherent since when written to it will write straight to output
	// ie. don't want screen image cached
	// host visible means cpu can write directly to buffer to?
	// allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, bufferMemory.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}

	// 3rd arg is offset withon region of memory, should be aligned
	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

VkCommandBuffer HelloTriangleApplication::beginSingleTimeCommands() {
	// to copy from staging buffer to gpu buffer
	// may want command pool for command buffers as otherwise short lived
	// created to execute memory transfer command only
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = transferCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	// used to submit one command
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void HelloTriangleApplication::endSingleTimeCommands(VkCommandBuffer commandBuffer) {

	// only need copy command in this command buffer so stop recording
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// do the transfer
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);

	// either wait for a fence - could allow multiple transfer in simul
	// or wait till queue is idle then done
	vkQueueWaitIdle(transferQueue);

	vkFreeCommandBuffers(device, transferCommandPool, 1, &commandBuffer);
}


void HelloTriangleApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void HelloTriangleApplication::createVertexBuffer() {
	// now making this use staging buffer technique
	// now make program write to buffer in mem, then gpu reads this
	// buffer into gpu-only mem, faster

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	// main memory buffer
	// args to createBuffer, size, usage, properties
	VDeleter<VkBuffer> stagingBuffer{device, vkDestroyBuffer};
	VDeleter<VkDeviceMemory> stagingBufferMemory{device, vkFreeMemory};

	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	// allocate memory for buffer
	void* data;
	// args 0 to 5
	// 1 -> memory object to map
	// 2 -> byte offset
	// 3 -> size of range to map
	// 5 -> ptr to mapped memory
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);

	// write data to memory, copy vertex data to buffer
	// dst, src, size(+1)
	memcpy(data, vertices.data(), (size_t) bufferSize);

	// problems of caching rather than immediately writing to mapped mem solved by
	// using coherent mem heap, as set by HOST_COHERENT_BIT above
	vkUnmapMemory(device, stagingBufferMemory);

	// USAGE_TRANSFER_SRC/DST, buffer used as src/dest in mem op - stage buffer src
	// DEVICE_LOCAL_BIT, device (gpu) only mem, faster
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	// actually do the copy from staging (main mem) to vertex (gpu mem)
	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	// old one if not using staging buffer
	//createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer, vertexBufferMemory);	

}

/*
static void windowFocusCallback(GLFWwindow* window, int focused) {
	if (focused) {
		std::cout << "Window gained focus" << "\n";
		// The window gained input focus
	}
	else {
		// The window lost input focus
	}
}
*/

void HelloTriangleApplication::recreateSwapchain() {
	std::cout << "Recreating swapchain";
	vkDeviceWaitIdle(device);
	std::cout << " device not idle, remaking" << "\n";
	createSwapchain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFrameBuffers();
	createCommandBuffers();
}

void HelloTriangleApplication::createSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, imageAvailableSemaphore.replace()) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, renderFinishedSemaphore.replace()) != VK_SUCCESS) {

		throw std::runtime_error("failed to create semaphores!");
	}
}

void HelloTriangleApplication::drawFrame() {
	// getting image from swapchain
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);   

	// if couldn't acquire Swapchain for some reason
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	// wait writing colours to image until available
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	// which command buffer to submit for execution
	// specify one that binds swapchain image as color attachment
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	// which semaphores to signal once command buffer done
	VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// submit command buffer to graphics queue
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	// submit result back to swapchain
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	// wait on these before presentation
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	// which swapchain to present image to and index of image per swapchain
	VkSwapchainKHR swapchains[] = {swapchain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;

	// PRESENT THE IMAGE
	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapchain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

}

void HelloTriangleApplication::createCommandBuffers() {
	// if method re-called then likely Swapchain invalid so want to free command buffers first
	if (commandBuffers.size() > 0) {
		vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
	}

	commandBuffers.resize(swapchainFramebuffers.size());

	// primary cmd buff, can be submitted to a queue for execution
	// secondary cannot be submitted directly but called from primary
	// command buffers alloced/freed with their own functions, different from wrapper

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < commandBuffers.size(); i++) {

		// flags here says buffer can be resubmitted whilst
		// pending execution

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapchainFramebuffers[i];

		// render area is where shader loads/stored are
		// should match size of attachments
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapchainExtent;

		// LOAD_OP_CLEAR colour
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = {1.0f, 1.0f, 1.0f, 1.0f};
		clearValues[1].depthStencil = {1.0f, 0}; // view plane far, near values

		renderPassInfo.clearValueCount = clearValues.size();
		renderPassInfo.pClearValues = clearValues.data();

		// last option can be changed to execute commands from secondary buffers
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};

		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// binding descriptors for ubo, separate to this other stuff
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		// was this without index buffer
		// 1,2 -> offset, number of bindings specify vertex buffers for
		// 3,4 -> array of vertex buffers to bind, byte offsets to read vertex data from
		// vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

		// cmd buffer, vertexCount, instanceCount(1 if not instancing), firstVertex, firstInstance
		//vkCmdDraw(commandBuffers[i], vertices.size(), 1, 0, 0);

		// draw command using index buffer
		// command buff, number vertices to draw, number of instances,
		// base index within index buffer, value added to vertex index before index into
		// index buffer, instance id of first instance to draw
		vkCmdDrawIndexed(commandBuffers[i], indices.size(), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

	}

}

void HelloTriangleApplication::createCommandPool() {
	// command buffers executed by submitting them to a device queue
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	// flags are if rerecording command buffers frequently
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(device, &poolInfo, nullptr, commandPool.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}

	// flags are if rerecording command buffers frequently
	VkCommandPoolCreateInfo transferPoolInfo = {};
	transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily;
	transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	// Optional - unsure if should have transient bit flag for this
	// if command buffers alloced from pool short lived then should set flag

	if (vkCreateCommandPool(device, &transferPoolInfo, nullptr, transferCommandPool.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create transfer command pool!");
	}
}

void HelloTriangleApplication::createFrameBuffers() {
	// now must be be after depth resources created
	swapchainFramebuffers.resize(swapchainImageViews.size(), VDeleter<VkFramebuffer>{device, vkDestroyFramebuffer});

	// create framebuffer in list for every image in swapchain
	for (size_t i = 0; i < swapchainImageViews.size(); i++) {

		std::array<VkImageView, 2> attachments = {
			swapchainImageViews[i],
			depthImageView
		};
		// colour attachment differs per swapchain image, but depth image used by all
		// only a single subpass is running at the time due to semaphores

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, swapchainFramebuffers[i].replace()) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void HelloTriangleApplication::createRenderPass() {
	// specify number of colour/depth buffers, samples in each
	// and handling of contents

	// loadOp -> clear framebuffer to black before next frame
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// not using stencils
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// framebuffer contents before render
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// images to be presented in swap chain
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// for depth testing
	// format should be same as image itself
	// don't care about data after drawing is finished
	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// one render pass can contain subpasses indexed 0 -> n
	// just using one here for attachment==index = 0
	// optimal because vulkan can reorder subpasses to optimise memory etc.
	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// index of attachment in array is directly referenced from fragment shader with
	// layout(location = 0) out vec4 outColor directive
	// index added for subpass for depth testing
	VkSubpassDescription subPass = {};
	subPass.colorAttachmentCount = 1;
	subPass.pColorAttachments = &colorAttachmentRef;
	subPass.pDepthStencilAttachment = &depthAttachmentRef;

	// SUBPASS_EXTERNAL refers to implicit subpass before/after render pass
	// depending on if in src/dst where dst>src
	// swapchain must wait until done reading before access
	// colour attachment stage
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subPass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, renderPass.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void HelloTriangleApplication::createShaderModule(const std::vector<char>& code, VDeleter<VkShaderModule>& shaderModule) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = (uint32_t*) code.data();
	if (vkCreateShaderModule(device, &createInfo, nullptr, shaderModule.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
}

void HelloTriangleApplication::createGraphicsPipeline() {
	// func creates programmable pipeline
	auto vertShaderCode = readFile("vert.spv");
	auto fragShaderCode = readFile("frag.spv");

	// local so mem freed when this func ends
	VDeleter<VkShaderModule> vertShaderModule{device, vkDestroyShaderModule};
	VDeleter<VkShaderModule> fragShaderModule{device, vkDestroyShaderModule};

	createShaderModule(vertShaderCode, vertShaderModule);
	createShaderModule(fragShaderCode, fragShaderModule);

	// vertex shader
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // pipeline stage
	vertShaderStageInfo.module = vertShaderModule; // shader module containing code
	vertShaderStageInfo.pName = "main"; // shader func to invoke
	// pSpecializationInfo - field to allow specifying compile-time consts to shader

	// fragment shader, same stuff
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

	// fixed function pipeline
	// data for this in vertex shader
	//VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	//vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	//vertexInputInfo.vertexBindingDescriptionCount = 0;
	//vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	//vertexInputInfo.vertexAttributeDescriptionCount = 0;
	//vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	// taking input from Vertices struct
	// binding and attribs
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// what kind of geom - points, lines, triangles list/strip
	// primitive restart yes/no - change geom type on fly by using code
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// portion of framebuffer to render
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swapchainExtent.width;
	viewport.height = (float) swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// see link for scissor meaning vs viewport
	// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions#page_Viewports_and_scissors
	// don't cut anything, just normal
	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = swapchainExtent;

	// puts viewport and scissor object into this one
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	// rasterizer stuffz
	// if depthClampEnable
	// fragments that are beyond the near and far planes are clamped to them
	// as opposed to discarding them
	// polygonmode != FILL needs gpu feature, fill area of poly with fragments
	// cullMode - disable, front cull, back, or both
	// frontFace is vertex order for culling - here set clockwise
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f; // >1 needs GPU feature wideLines
	rasterizer.cullMode = VK_CULL_MODE_NONE; // usually back
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// MultiSampling, not being used
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// depthTestEnable - depth testing or not
	// depthWriteEnable - should actually write fragments that pass test to buffer
	// useful for drawing transparent objects
	// depthCompareOp is how to decide - COMPARE_OP_LESS is lower depth is closer
	// bounds allows only keeping fragments in certain bounds
	// not using stencils
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	// colour blending now
	// AttachmentState obj is per framebuffer
	// StateCreateInfo is global colour blend settings
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	// set false, bitwise colour blending, only need one of this or above
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	// can change some settings on the fly, else set this to nullptr
	// if want none
	//VkDynamicState dynamicStates[] = {
		//VK_DYNAMIC_STATE_VIEWPORT,
		//VK_DYNAMIC_STATE_LINE_WIDTH
	//};

	//VkPipelineDynamicStateCreateInfo dynamicState = {};
	//dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	//dynamicState.dynamicStateCount = 2;
	//dynamicState.pDynamicStates = dynamicStates;

	VkDescriptorSetLayout setLayouts[] = {descriptorSetLayout};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = setLayouts; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
				pipelineLayout.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	// put all the config into this struct
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	//pipelineInfo.pDynamicState = &dynamicState; // Optional -- if using dynamic state

	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	// could specify derived pipeline to reuse functionality
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1; // Optional

	// nullptr is to pipelinecache obj, for now nullptr
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphicsPipeline.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

}

void HelloTriangleApplication::createImageViews() {
	swapchainImageViews.resize(swapchainImages.size(), VDeleter<VkImageView>{device, vkDestroyImageView});
	for (uint32_t i = 0; i < swapchainImages.size(); i++) {
		createImageView(swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, swapchainImageViews[i]);
	}
}

void HelloTriangleApplication::createSwapchain() {
	SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice);
	// images in swap chain, res
	// pixel format, colour space
	// presentation modes
	// rgba colour model as format, stored in srgb colour space I think?
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
	//mailbox else fifo
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
	// res of stuff passed to swapchain
	VkExtent2D extent = chooseSwapExtent(swapchainSupport.capabilities);

	// imageCount =  max number of images that can be in swap chain
	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
	if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
		imageCount = swapchainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1; // 2 etc for 3d
	// rendering image -> swap chain direct, could be set different to post-process first
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = {(uint32_t) indices.graphicsFamily,
		(uint32_t) indices.presentFamily, (uint32_t) indices.transferFamily};

	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.pQueueFamilyIndices = queueFamilyIndices;

	// if queues are on different hardware (unlikely) do concurrent - images from multiple
	// queue families can be used from swap chain
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.queueFamilyIndexCount = 3;
	} else {
		createInfo.queueFamilyIndexCount = 2;
	}

	createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
	// don't want a transform on images in swap chain (eg 90deg rotate)
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE; // don't care about obscured pixel colour

	// indicate replacing old swap chain - ie on window resize, no longer valid
	VkSwapchainKHR oldSwapchain = swapchain;
	createInfo.oldSwapchain = oldSwapchain;

	// need both swapchains to be around at same time so this func can take the old
	// Swapchain into account
	VkSwapchainKHR newSwapchain;
	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &newSwapchain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	swapchain = newSwapchain;

	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;


}

//
VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	// if "surface size will be determined by the extent of a swapchain targeting the surface"
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent; 
	} else {
		VkExtent2D actualExtent = {(uint32_t)wide, (uint32_t)high};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

// look for best mode, immediate, fifo, fifo-relaxed, mailbox
// https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain#page_Presentation_mode
// this is selecting mailbox, else fifo(always available)
// NOTE TO SELF - BECAUSE I CHANGE THE FRAMERATE, THIS MUST NOT BE EITHER OF THE FIFO IE. VSYNC MODES
VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

// choose best swap chain settings for colour depth
// eg 8 bit unsigned int rgba = 32 bits per pixel (VK_FORMAT_B8G8R8A8_UNORM)
VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	// surface has no preference
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	}

	// try to get our preference of rgba 8bit unsigned, using srgb
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	// else just take first one available
	return availableFormats[0];

}

SwapchainSupportDetails HelloTriangleApplication::querySwapchainSupport(VkPhysicalDevice device) {
	SwapchainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}


void HelloTriangleApplication::createSurface() {
	if (glfwCreateWindowSurface(instance, window, nullptr, surface.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void HelloTriangleApplication::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily, indices.transferFamily};

	// set of size 1 or 2 (if it worked), usually will be of size 1, ie same device
	// can create scene and present it to user
	// create one for transfer queue too

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {}; // special features we want, rn need none

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = (uint32_t) queueCreateInfos.size();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, device.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
	vkGetDeviceQueue(device, indices.transferFamily, 0, &transferQueue);
}

void HelloTriangleApplication::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU! - AHHHHHHHHHHH");
	}

}

QueueFamilyIndices HelloTriangleApplication::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {

		if (queueFamily.queueCount > 0) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
				if (presentSupport) {
					indices.presentFamily = i;
				}
			} else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
				indices.transferFamily = i;
			}
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

bool HelloTriangleApplication::isDeviceSuitable(VkPhysicalDevice device) {
	//VkPhysicalDeviceProperties deviceProperties;
	//VkPhysicalDeviceFeatures deviceFeatures;
	//vkGetPhysicalDeviceProperties(device, &deviceProperties);
	//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapchainAdequate = false;
	if (extensionsSupported) {
		SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device);
		swapchainAdequate = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
	}

	// returns true if device has ability to render and present
	// and supports vulkan extensions we want
	// and has swap chain has supported image format(pixel mode etc) and present mode
	return indices.isComplete() && extensionsSupported && swapchainAdequate;
}

// returns true if have all extensions specified in deviceExtensions at top
bool HelloTriangleApplication::checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

void HelloTriangleApplication::createInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = validationLayers.size();
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&createInfo, nullptr, instance.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void HelloTriangleApplication::setupDebugCallback() {
	if (!enableValidationLayers) return;

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, callback.replace()) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}

std::vector<const char*> HelloTriangleApplication::getRequiredExtensions() {
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		extensions.push_back(glfwExtensions[i]);
	}

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

bool HelloTriangleApplication::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<char> HelloTriangleApplication::readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApplication::debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData) {
	std::cerr << "validation layer: " << msg << std::endl;

	return VK_FALSE;
}

