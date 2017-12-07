#include <QGuiApplication>
#include "vulkanwindow.h"

int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication app(argc, argv);

	QScopedPointer<VulkanWindow> vkwin;
	vkwin.reset(new VulkanWindow);
	vkwin->resize(1024, 768);
	vkwin->show();

	return app.exec();
}