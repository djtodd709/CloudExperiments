#include "renderer.h"

int main(void)
{
	Renderer* renderer = new Renderer();

	while (renderer->isRunning()){
		renderer->UpdateScene();
		renderer->RenderScene();
	}

	delete renderer;

	return 0;
}