#if defined(_MSC_VER) && defined(DETECT_MEM_LEAK) && defined(_DEBUG)
#include <vld.h>
#endif

#include <QApplication>
#include "common.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
//  qInstallMessageHandler(myMessageOutput);
	QApplication app(argc, argv);

	// Set application info
	app.setApplicationName(QString(APPLICATION_NAME));
	app.setApplicationVersion(QString::fromUtf8(VersionString(APPLICATION_VERSION).c_str()));

	// Set legal info
	app.setOrganizationName(QString(ORGANIZATION_NAME));

	MainWindow w;
	w.show();

	return app.exec();
}
