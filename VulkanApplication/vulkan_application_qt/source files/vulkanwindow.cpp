#include "vulkanwindow.h"

#include <QDebug>
#include <QTimer>
#include <thread>

void VulkanWindow::start_show()
{
	this->_vulkan_app.init_window(reinterpret_cast<HWND>(this->winId()),
		GetModuleHandle(nullptr), this->width(), this->height());
	qDebug() << "start show !" << reinterpret_cast<HWND>(this->winId()) << endl;
	this->_vulkan_app.init_vulkan();
}

void VulkanWindow::end_show()
{
	qDebug() << "end show !" << reinterpret_cast<HWND>(this->winId()) << endl;
	this->_vulkan_app.clear_vulkan();
}

VulkanWindow::VulkanWindow(QWindow *parent) : QWindow(parent)
{
	this->installEventFilter(this);
}

static int is_init = 0;

bool VulkanWindow::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::Expose)
	{
		if (is_init == 0)
		{
			this->start_show();
			this->_vulkan_app.start_run();
			is_init = 1;
		}
		else if (is_init == 1)
		{
			this->_vulkan_app.end_run();
			this->end_show();
			is_init = -1;
		}
	}
	return false;
}