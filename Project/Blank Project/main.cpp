#include "../NCLGL/window.h"
#include "Renderer.h"


int main() {
	Window w("Coursework!", 1500, 1000, false); //This is all boring win32 window creation stuff!
	if (!w.HasInitialised()) {
		return -1;
	}

	Renderer renderer(w); //This handles all the boring OGL 3.2 initialisation stuff, and sets up our tutorial!
	if (!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
		renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
		//std::cout << "Updated\n";
		//while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_D));
		renderer.RenderScene();
		//std::cout << "Rendered\n";
		//while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_S));
		renderer.SwapBuffers();
		//std::cout << "Swapped\n";
		//while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_F));
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
	}

	

	return 0;
}