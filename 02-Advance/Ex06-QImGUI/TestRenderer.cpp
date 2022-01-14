#include "TestRenderer.h"

TestRenderer::TestRenderer(QVulkanWindow* window)
	: QImGUIRenderer(window)
{
}

void TestRenderer::initResources()
{
	QImGUIRenderer::initResources();
}

void TestRenderer::initSwapChainResources()
{
}

void TestRenderer::releaseSwapChainResources()
{
}

void TestRenderer::releaseResources()
{
	QImGUIRenderer::releaseResources();
}

void TestRenderer::startNextFrame()
{
	QImGUIRenderer::startNextFrame();

	ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
	ImGui::ShowDemoWindow();
	ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);

	// normally you wouldn't change the entire style each frame
	ImPlotStyle backup = ImPlot::GetStyle();
	if (ImPlot::BeginPlot("seaborn style")) {
		ImPlot::SetupAxes("x-axis", "y-axis");
		ImPlot::SetupAxesLimits(-0.5f, 9.5f, 0, 10);
		unsigned int lin[10] = { 8,8,9,7,8,8,8,9,7,8 };
		unsigned int bar[10] = { 1,2,5,3,4,1,2,5,3,4 };
		unsigned int dot[10] = { 7,6,6,7,8,5,6,5,8,7 };
		ImPlot::PlotBars("Bars", bar, 10, 0.5f);
		ImPlot::PlotLine("Line", lin, 10);
		ImPlot::NextColormapColor(); // skip green
		ImPlot::PlotScatter("Scatter", dot, 10);
		ImPlot::EndPlot();
	}
	ImPlot::GetStyle() = backup;
	ImPlot::ShowDemoWindow();

	frameReady();
	requestUpdate();
}