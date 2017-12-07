#ifndef VULKANWINDOW_H
#define VULKANWINDOW_H

#include <QObject>
#include <QWindow>

#include "Vulkan.h"

class VulkanWindow : public QWindow
{
	Q_OBJECT
		signals :
	void init_signal();
	void destroy_signal();

	public slots:
	void start_show();
	void end_show();

public:
	VulkanWindow(QWindow *parent = nullptr);

private:
	Vulkan _vulkan_app;
	bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // VULKANWINDOW_H
