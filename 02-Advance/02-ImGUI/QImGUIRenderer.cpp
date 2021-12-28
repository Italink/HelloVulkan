#include <QFile>
#include "QImGUIRenderer.h"
#include <QApplication>
#include <QClipboard>
#include <QMouseEvent>
#include "QDateTime"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

const int64_t IMGUI_BUFFER_SIZE = 50000;

static inline vk::DeviceSize aligned(vk::DeviceSize v, vk::DeviceSize byteAlign) {
	return (v + byteAlign - 1) & ~(byteAlign - 1);
}

const QHash<int, ImGuiKey> keyMap = {
	{ Qt::Key_Tab, ImGuiKey_Tab },
	{ Qt::Key_Left, ImGuiKey_LeftArrow },
	{ Qt::Key_Right, ImGuiKey_RightArrow },
	{ Qt::Key_Up, ImGuiKey_UpArrow },
	{ Qt::Key_Down, ImGuiKey_DownArrow },
	{ Qt::Key_PageUp, ImGuiKey_PageUp },
	{ Qt::Key_PageDown, ImGuiKey_PageDown },
	{ Qt::Key_Home, ImGuiKey_Home },
	{ Qt::Key_End, ImGuiKey_End },
	{ Qt::Key_Insert, ImGuiKey_Insert },
	{ Qt::Key_Delete, ImGuiKey_Delete },
	{ Qt::Key_Backspace, ImGuiKey_Backspace },
	{ Qt::Key_Space, ImGuiKey_Space },
	{ Qt::Key_Enter, ImGuiKey_Enter },
	{ Qt::Key_Return, ImGuiKey_Enter },
	{ Qt::Key_Escape, ImGuiKey_Escape },
	{ Qt::Key_A, ImGuiKey_A },
	{ Qt::Key_C, ImGuiKey_C },
	{ Qt::Key_V, ImGuiKey_V },
	{ Qt::Key_X, ImGuiKey_X },
	{ Qt::Key_Y, ImGuiKey_Y },
	{ Qt::Key_Z, ImGuiKey_Z },
	{ Qt::MiddleButton, ImGuiMouseButton_Middle }
};

const QHash<ImGuiMouseCursor, Qt::CursorShape> cursorMap = {
	{ ImGuiMouseCursor_Arrow,      Qt::CursorShape::ArrowCursor },
	{ ImGuiMouseCursor_TextInput,  Qt::CursorShape::IBeamCursor },
	{ ImGuiMouseCursor_ResizeAll,  Qt::CursorShape::SizeAllCursor },
	{ ImGuiMouseCursor_ResizeNS,   Qt::CursorShape::SizeVerCursor },
	{ ImGuiMouseCursor_ResizeEW,   Qt::CursorShape::SizeHorCursor },
	{ ImGuiMouseCursor_ResizeNESW, Qt::CursorShape::SizeBDiagCursor },
	{ ImGuiMouseCursor_ResizeNWSE, Qt::CursorShape::SizeFDiagCursor },
	{ ImGuiMouseCursor_Hand,       Qt::CursorShape::PointingHandCursor },
	{ ImGuiMouseCursor_NotAllowed, Qt::CursorShape::ForbiddenCursor },
};

QByteArray g_currentClipboardText;

QImGUIRenderer::QImGUIRenderer(QVulkanWindow* window)
	:window_(window)
{
	QList<int> sampleCounts = window->supportedSampleCounts();
	if (!sampleCounts.isEmpty()) {
		window->setSampleCount(sampleCounts.back());
	}

	imGuiContext = ImGui::CreateContext();
	imPlotContext = ImPlot::CreateContext();
	ImGui::SetCurrentContext(imGuiContext);

	ImGuiIO& io = ImGui::GetIO();

	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;  // We can honor io.WantSetMousePos requests (optional, rarely used)
	io.BackendPlatformName = "Qt ImGUI";

	for (ImGuiKey key : keyMap.values()) {
		io.KeyMap[key] = key;
	}

	io.SetClipboardTextFn = [](void* user_data, const char* text) {
		Q_UNUSED(user_data);
		QGuiApplication::clipboard()->setText(text);
	};

	io.GetClipboardTextFn = [](void* user_data) {
		Q_UNUSED(user_data);
		g_currentClipboardText = QGuiApplication::clipboard()->text().toUtf8();
		return (const char*)g_currentClipboardText.data();
	};
	window_->installEventFilter(this);
}

QImGUIRenderer::~QImGUIRenderer()
{
	ImPlot::DestroyContext(imPlotContext);
	ImGui::DestroyContext(imGuiContext);
}

bool QImGUIRenderer::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == window_) {
		switch (event->type()) {
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease: {
			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			g_MousePressed[0] = mouseEvent->buttons() & Qt::LeftButton;
			g_MousePressed[1] = mouseEvent->buttons() & Qt::RightButton;
			g_MousePressed[2] = mouseEvent->buttons() & Qt::MiddleButton;
			break;
		}
		case QEvent::Wheel: {
			QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
			ImGui::SetCurrentContext(imGuiContext);
			if (wheelEvent->pixelDelta().x() != 0) {
				g_MouseWheelH += wheelEvent->pixelDelta().x() / (ImGui::GetTextLineHeight());
			}
			else {
				g_MouseWheelH += wheelEvent->angleDelta().x() / 120;
			}
			if (wheelEvent->pixelDelta().y() != 0) {
				g_MouseWheel += wheelEvent->pixelDelta().y() / (5.0 * ImGui::GetTextLineHeight());
			}
			else {
				g_MouseWheel += wheelEvent->angleDelta().y() / 120;
			}
			break;
		}
		case QEvent::KeyPress:
		case QEvent::KeyRelease: {
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			ImGui::SetCurrentContext(imGuiContext);
			ImGuiIO& io = ImGui::GetIO();
			const bool key_pressed = (event->type() == QEvent::KeyPress);
			const auto key_it = keyMap.constFind(keyEvent->key());
			if (key_it != keyMap.constEnd()) {
				const int imgui_key = *(key_it);
				io.KeysDown[imgui_key] = key_pressed;
			}

			if (key_pressed) {
				const QString text = keyEvent->text();
				if (text.size() == 1) {
					io.AddInputCharacter(text.at(0).unicode());
				}
			}
#ifdef Q_OS_MAC
			io.KeyCtrl = keyEvent->modifiers() & Qt::MetaModifier;
			io.KeyShift = keyEvent->modifiers() & Qt::ShiftModifier;
			io.KeyAlt = keyEvent->modifiers() & Qt::AltModifier;
			io.KeySuper = keyEvent->modifiers() & Qt::ControlModifier;
#else
			io.KeyCtrl = keyEvent->modifiers() & Qt::ControlModifier;
			io.KeyShift = keyEvent->modifiers() & Qt::ShiftModifier;
			io.KeyAlt = keyEvent->modifiers() & Qt::AltModifier;
			io.KeySuper = keyEvent->modifiers() & Qt::MetaModifier;
#endif
			break;
		}
		default:
			break;
		}
	}
	return QObject::eventFilter(watched, event);
}

void QImGUIRenderer::initResources()
{
	vk::Device device = window_->device();
	ImGui::SetCurrentContext(imGuiContext);
	ImGuiIO& io = ImGui::GetIO();
	vk::PhysicalDeviceLimits limits = window_->physicalDeviceProperties()->limits;

	vk::BufferCreateInfo vertexBufferInfo;
	vertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	vertexBufferInfo.size = sizeof(ImDrawVert) * IMGUI_BUFFER_SIZE;
	vertexBuffer_ = device.createBuffer(vertexBufferInfo);
	vk::MemoryRequirements vertexMemReq = device.getBufferMemoryRequirements(vertexBuffer_);
	vk::MemoryAllocateInfo vertexMemAllocInfo(vertexMemReq.size, window_->hostVisibleMemoryIndex());
	vertexDevMemory_ = device.allocateMemory(vertexMemAllocInfo);
	device.bindBufferMemory(vertexBuffer_, vertexDevMemory_, 0);

	vk::BufferCreateInfo indexBufferInfo;
	indexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
	indexBufferInfo.size = sizeof(ImDrawIdx) * IMGUI_BUFFER_SIZE;
	indexBuffer_ = device.createBuffer(indexBufferInfo);
	vk::MemoryRequirements indexMemReq = device.getBufferMemoryRequirements(indexBuffer_);
	vk::MemoryAllocateInfo indexMemInfo(indexMemReq.size, window_->hostVisibleMemoryIndex());
	indexDevMemory_ = device.allocateMemory(indexMemInfo);
	device.bindBufferMemory(indexBuffer_, indexDevMemory_, 0);

	vk::SamplerCreateInfo samplerInfo;
	samplerInfo.magFilter = vk::Filter::eNearest;
	samplerInfo.minFilter = vk::Filter::eNearest;
	samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
	samplerInfo.maxAnisotropy = 1.0f;
	sampler_ = device.createSampler(samplerInfo);

	vk::PhysicalDevice physicalDevice = window_->physicalDevice();

	vk::FormatProperties formatProps = physicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
	auto canSampleLinear = (formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);
	auto canSampleOptimal = (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);
	if (!canSampleLinear && !canSampleOptimal) {
		qWarning("Neither linear nor optimal image sampling is supported for RGBA8");
	}

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.format = vk::Format::eR8G8B8A8Unorm;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.tiling = vk::ImageTiling::eLinear;
	imageInfo.usage = vk::ImageUsageFlagBits::eSampled;
	imageInfo.initialLayout = vk::ImageLayout::ePreinitialized;

	fontTexture_ = device.createImage(imageInfo);

	vk::MemoryRequirements texMemReq = device.getImageMemoryRequirements(fontTexture_);
	vk::MemoryAllocateInfo allocInfo(texMemReq.size, window_->hostVisibleMemoryIndex());
	fontTexMemory_ = device.allocateMemory(allocInfo);
	device.bindImageMemory(fontTexture_, fontTexMemory_, 0);

	vk::ImageSubresource subres(vk::ImageAspectFlagBits::eColor, 0, 0/*imageInfo.mipLevels, imageInfo.arrayLayers*/);
	vk::SubresourceLayout subresLayout = device.getImageSubresourceLayout(fontTexture_, subres);
	uint8_t* texMemPtr = (uint8_t*)device.mapMemory(fontTexMemory_, subresLayout.offset, subresLayout.size);
	memcpy(texMemPtr, pixels, subresLayout.size);
	device.unmapMemory(fontTexMemory_);

	vk::ImageViewCreateInfo imageViewInfo;
	imageViewInfo.image = fontTexture_;
	imageViewInfo.viewType = vk::ImageViewType::e2D;
	imageViewInfo.format = vk::Format::eR8G8B8A8Unorm;
	imageViewInfo.components.r = vk::ComponentSwizzle::eR;
	imageViewInfo.components.g = vk::ComponentSwizzle::eG;
	imageViewInfo.components.b = vk::ComponentSwizzle::eB;
	imageViewInfo.components.a = vk::ComponentSwizzle::eA;
	imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imageViewInfo.subresourceRange.levelCount = imageViewInfo.subresourceRange.layerCount = 1;
	fontTexView_ = device.createImageView(imageViewInfo);

	vk::CommandBufferAllocateInfo cmdBufferAllocInfo;
	cmdBufferAllocInfo.commandBufferCount = 1;
	cmdBufferAllocInfo.commandPool = window_->graphicsCommandPool();
	cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	vk::CommandBuffer cmdBuffer = device.allocateCommandBuffers(cmdBufferAllocInfo).front();
	vk::CommandBufferBeginInfo cmdBufferBeginInfo;
	cmdBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	cmdBuffer.begin(cmdBufferBeginInfo);

	vk::ImageMemoryBarrier barrier;
	barrier.image = fontTexture_;
	barrier.oldLayout = vk::ImageLayout::ePreinitialized;
	barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.layerCount = barrier.subresourceRange.levelCount = 1;
	cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
	cmdBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	vk::Queue queue = window_->graphicsQueue();
	queue.submit(submitInfo);
	queue.waitIdle();

	vk::DescriptorPoolSize descPoolSize = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, (uint32_t)1)
	};

	vk::DescriptorPoolCreateInfo descPoolInfo;
	descPoolInfo.maxSets = 1;
	descPoolInfo.poolSizeCount = 1;
	descPoolInfo.pPoolSizes = &descPoolSize;
	descPool_ = device.createDescriptorPool(descPoolInfo);

	vk::DescriptorSetLayoutBinding layoutBinding = { 0, vk::DescriptorType::eCombinedImageSampler,1,vk::ShaderStageFlagBits::eFragment };

	vk::DescriptorSetLayoutCreateInfo descLayoutInfo;
	descLayoutInfo.pNext = nullptr;
	descLayoutInfo.flags = {};
	descLayoutInfo.bindingCount = 1;
	descLayoutInfo.pBindings = &layoutBinding;

	descSetLayout_ = device.createDescriptorSetLayout(descLayoutInfo);

	vk::DescriptorSetAllocateInfo descSetAllocInfo(descPool_, 1, &descSetLayout_);
	descSet_ = device.allocateDescriptorSets(descSetAllocInfo).front();
	vk::WriteDescriptorSet descWrite;
	vk::DescriptorImageInfo descImageInfo(sampler_, fontTexView_, vk::ImageLayout::eShaderReadOnlyOptimal);
	descWrite.dstSet = descSet_;
	descWrite.dstBinding = 0;
	descWrite.descriptorCount = 1;
	descWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
	descWrite.pImageInfo = &descImageInfo;
	device.updateDescriptorSets(1, &descWrite, 0, nullptr);

	vk::GraphicsPipelineCreateInfo piplineInfo;
	piplineInfo.stageCount = 2;

	vk::ShaderModule vertShader = loadShader("./imgui_vert.spv");
	vk::ShaderModule fragShader = loadShader("./imgui_frag.spv");

	vk::PipelineShaderStageCreateInfo piplineShaderStage[2];
	piplineShaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
	piplineShaderStage[0].module = vertShader;
	piplineShaderStage[0].pName = "main";
	piplineShaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
	piplineShaderStage[1].module = fragShader;
	piplineShaderStage[1].pName = "main";
	piplineInfo.pStages = piplineShaderStage;

	vk::VertexInputBindingDescription vertexBindingDesc;
	vertexBindingDesc.binding = 0;
	vertexBindingDesc.stride = sizeof(ImDrawVert);
	vertexBindingDesc.inputRate = vk::VertexInputRate::eVertex;

	vk::VertexInputAttributeDescription vertexAttrDesc[3];
	vertexAttrDesc[0].binding = 0;
	vertexAttrDesc[0].location = 0;
	vertexAttrDesc[0].format = vk::Format::eR32G32Sfloat;
	vertexAttrDesc[0].offset = offsetof(ImDrawVert, pos);

	vertexAttrDesc[1].binding = 0;
	vertexAttrDesc[1].location = 1;
	vertexAttrDesc[1].format = vk::Format::eR32G32Sfloat;
	vertexAttrDesc[1].offset = offsetof(ImDrawVert, uv);

	vertexAttrDesc[2].binding = 0;
	vertexAttrDesc[2].location = 2;
	vertexAttrDesc[2].format = vk::Format::eR8G8B8A8Unorm;
	vertexAttrDesc[2].offset = offsetof(ImDrawVert, col);

	vk::PipelineVertexInputStateCreateInfo vertexInputState({}, 1, &vertexBindingDesc, 3, vertexAttrDesc);
	piplineInfo.pVertexInputState = &vertexInputState;

	vk::PipelineInputAssemblyStateCreateInfo vertexAssemblyState({}, vk::PrimitiveTopology::eTriangleList);
	piplineInfo.pInputAssemblyState = &vertexAssemblyState;

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	piplineInfo.pViewportState = &viewportState;

	vk::PipelineRasterizationStateCreateInfo rasterizationState;
	rasterizationState.polygonMode = vk::PolygonMode::eFill;
	rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
	rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;
	rasterizationState.lineWidth = 1.0f;
	piplineInfo.pRasterizationState = &rasterizationState;

	vk::PipelineMultisampleStateCreateInfo MSState;
	MSState.rasterizationSamples = (VULKAN_HPP_NAMESPACE::SampleCountFlagBits)window_->sampleCountFlagBits();
	piplineInfo.pMultisampleState = &MSState;

	vk::PipelineDepthStencilStateCreateInfo DSState;
	DSState.depthTestEnable = false;
	DSState.depthWriteEnable = false;
	DSState.stencilTestEnable = false;
	DSState.depthCompareOp = vk::CompareOp::eLessOrEqual;
	piplineInfo.pDepthStencilState = &DSState;

	vk::PipelineColorBlendStateCreateInfo colorBlendState;
	colorBlendState.attachmentCount = 1;
	vk::PipelineColorBlendAttachmentState colorBlendAttachmentState;
	colorBlendAttachmentState.blendEnable = true;
	colorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	colorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	colorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
	colorBlendState.pAttachments = &colorBlendAttachmentState;
	piplineInfo.pColorBlendState = &colorBlendState;

	vk::PipelineDynamicStateCreateInfo dynamicState;
	vk::DynamicState dynamicEnables[] = { vk::DynamicState::eViewport ,vk::DynamicState::eScissor };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicEnables;
	piplineInfo.pDynamicState = &dynamicState;

	matMVP_.size = sizeof(float) * 16;
	matMVP_.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::PipelineLayoutCreateInfo piplineLayoutInfo;
	piplineLayoutInfo.setLayoutCount = 1;
	piplineLayoutInfo.pSetLayouts = &descSetLayout_;
	piplineLayoutInfo.pushConstantRangeCount = 1;
	piplineLayoutInfo.pPushConstantRanges = &matMVP_;
	piplineLayout_ = device.createPipelineLayout(piplineLayoutInfo);
	piplineInfo.layout = piplineLayout_;

	piplineInfo.renderPass = window_->defaultRenderPass();

	piplineCache_ = device.createPipelineCache(vk::PipelineCacheCreateInfo());
	pipline_ = device.createGraphicsPipeline(piplineCache_, piplineInfo).value;

	device.destroyShaderModule(vertShader);
	device.destroyShaderModule(fragShader);
}

void QImGUIRenderer::initSwapChainResources()
{
}

void QImGUIRenderer::releaseSwapChainResources()
{
}

void QImGUIRenderer::releaseResources() {
	vk::Device device = window_->device();
	device.destroyPipeline(pipline_);
	device.destroyPipelineCache(piplineCache_);
	device.destroyPipelineLayout(piplineLayout_);
	device.destroyDescriptorSetLayout(descSetLayout_);
	device.destroyDescriptorPool(descPool_);
	device.destroyBuffer(vertexBuffer_);
	device.freeMemory(vertexDevMemory_);
	device.destroyBuffer(indexBuffer_);
	device.freeMemory(indexDevMemory_);
	device.destroySampler(sampler_);
	device.destroyImage(fontTexture_);
	device.freeMemory(fontTexMemory_);
	device.destroyImageView(fontTexView_);
}

void QImGUIRenderer::startNextFrame() {
	ImGui::SetCurrentContext(imGuiContext);
	ImPlot::SetCurrentContext(imPlotContext);
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(window_->width(), window_->height());
	io.DisplayFramebufferScale = ImVec2(window_->devicePixelRatio(), window_->devicePixelRatio());

	double current_time = QDateTime::currentMSecsSinceEpoch() / double(1000);
	io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
	g_Time = current_time;
	if (io.WantSetMousePos) {
		const QPoint global_pos = window_->mapToGlobal(QPoint{ (int)io.MousePos.x, (int)io.MousePos.y });
		QCursor cursor = window_->cursor();
		cursor.setPos(global_pos);
		window_->setCursor(cursor);
	}
	if (window_->isActive()) {
		const QPoint pos = window_->mapFromGlobal(QCursor::pos());
		io.MousePos = ImVec2(pos.x(), pos.y());   // Mouse position in screen coordinates (set to -1,-1 if no mouse / on another screen, etc.)
	}
	else {
		io.MousePos = ImVec2(-1, -1);
	}
	for (int i = 0; i < 3; i++) {
		io.MouseDown[i] = g_MousePressed[i];
	}
	io.MouseWheelH = g_MouseWheelH;
	io.MouseWheel = g_MouseWheel;
	g_MouseWheelH = 0;
	g_MouseWheel = 0;
	if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
		return;

	const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
	if (io.MouseDrawCursor || (imgui_cursor == ImGuiMouseCursor_None)) {
		window_->setCursor(Qt::CursorShape::BlankCursor);
	}
	else {
		const auto cursor_it = cursorMap.constFind(imgui_cursor);
		if (cursor_it != cursorMap.constEnd()) {
			const Qt::CursorShape qt_cursor_shape = *(cursor_it);
			window_->setCursor(qt_cursor_shape);
		}
		else {
			window_->setCursor(Qt::CursorShape::ArrowCursor);
		}
	}
	ImGui::NewFrame();
}

void QImGUIRenderer::frameReady()
{
	submitImGui();
	window_->frameReady();
}

void QImGUIRenderer::requestUpdate()
{
	window_->requestUpdate();
}

void QImGUIRenderer::submitImGui()
{
	ImGui::SetCurrentContext(imGuiContext);
	vk::Device device = window_->device();
	vk::CommandBuffer cmdBuffer = window_->currentCommandBuffer();
	const QSize size = window_->swapChainImageSize();
	vk::ClearValue clearValues[3] = {
		vk::ClearColorValue(std::array<float,4>{0.0f,0.0f,0.0f,1.0f }),
		vk::ClearDepthStencilValue(1.0f,0),
		vk::ClearColorValue(std::array<float,4>{ 0.0f,0.0f,0.0f,1.0f }),
	};

	vk::RenderPassBeginInfo beginInfo;
	beginInfo.renderPass = window_->defaultRenderPass();
	beginInfo.framebuffer = window_->currentFramebuffer();
	beginInfo.renderArea.extent.width = size.width();
	beginInfo.renderArea.extent.height = size.height();
	beginInfo.clearValueCount = window_->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
	beginInfo.pClearValues = clearValues;

	cmdBuffer.beginRenderPass(beginInfo, vk::SubpassContents::eInline);

	vk::Viewport viewport;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = size.width();
	viewport.height = size.height();
	viewport.minDepth = 0;
	viewport.maxDepth = 1;
	cmdBuffer.setViewport(0, viewport);

	cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipline_);

	cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, piplineLayout_, 0, 1, &descSet_, 0, nullptr);

	QMatrix4x4 mat = window_->clipCorrectionMatrix();
	QRect rect(0, 0, size.width(), size.height());
	mat.ortho(rect);
	cmdBuffer.pushConstants(piplineLayout_, vk::ShaderStageFlagBits::eVertex, 0, 16 * sizeof(float), mat.constData());

	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();

	int64_t vertexBufferOffset = 0;
	int64_t indexBufferOffset = 0;

	for (int i = 0; i < draw_data->CmdListsCount; i++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[i];
		int b = cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);

		uint8_t* vertexBufferMemPtr = (uint8_t*)device.mapMemory(vertexDevMemory_, vertexBufferOffset, cmd_list->VtxBuffer.size_in_bytes());
		memcpy(vertexBufferMemPtr, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.size_in_bytes());
		device.unmapMemory(vertexDevMemory_);

		uint8_t* indexBufferMemPtr = (uint8_t*)device.mapMemory(indexDevMemory_, indexBufferOffset, cmd_list->IdxBuffer.size_in_bytes());
		memcpy(indexBufferMemPtr, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		device.unmapMemory(indexDevMemory_);

		cmdBuffer.bindVertexBuffers(0, vertexBuffer_, vertexBufferOffset);
		cmdBuffer.bindIndexBuffer(indexBuffer_, indexBufferOffset, sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);

		vertexBufferOffset += cmd_list->VtxBuffer.size_in_bytes();
		indexBufferOffset += cmd_list->IdxBuffer.size_in_bytes();

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback) {
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else {
				vk::Rect2D scissor;
				scissor.offset.x = pcmd->ClipRect.x;
				scissor.offset.y = pcmd->ClipRect.y;
				scissor.extent.width = (pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissor.extent.height = (pcmd->ClipRect.w - pcmd->ClipRect.y);
				cmdBuffer.setScissor(0, scissor);
				cmdBuffer.drawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset, pcmd->VtxOffset, 0);
			}
		}
	}
	cmdBuffer.endRenderPass();
}

vk::ShaderModule QImGUIRenderer::loadShader(const QString& name)
{
	QFile file(name);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning("Failed to read shader %s", qPrintable(name));
		return nullptr;
	}
	QByteArray blob = file.readAll();
	file.close();
	vk::ShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = blob.size();
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(blob.constData());
	vk::Device device = window_->device();
	return device.createShaderModule(shaderInfo);
}