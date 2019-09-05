#include "pch.h"
#include "vulkan/ModelApp.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	const int WindowWidth = 1280;
	const int WindowHeight = 720;
	const char* AppTitle = "Hello Vulkan";

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, 0);
	auto window = glfwCreateWindow(WindowWidth, WindowHeight, AppTitle, nullptr, nullptr);


	// Vulkan������
	ModelApp theApp;
	theApp.initialize(window, AppTitle);
	while (glfwWindowShouldClose(window) == GLFW_FALSE)
	{
		glfwPollEvents();
		theApp.render();
	}
	
	// Vulkan�I��
	theApp.terminate();
	glfwTerminate();

	return 0;
}