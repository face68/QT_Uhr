#include "ClockWidget.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a( argc, argv );

	QCoreApplication::setOrganizationName( "3D-Pixel" );
	QCoreApplication::setApplicationName( "QT_Uhr" );

	ClockWidget w;
	w.show();
	return a.exec();
}
