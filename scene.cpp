#include "Scene.h"
#include "Property.h"
#include "animator.h"

extern AnimatorApp* myApp;

Scene::Scene() {
	currentShape = NULL;
	currentTime = 0.f;

	bgColor = new ColorProperty(Vec3u(255, 255, 255), "Background Color", myApp->panelViewport);
	bgColor->CreateWidgets(0);
	//myApp->panelViewport_sizer->Layout();
};
