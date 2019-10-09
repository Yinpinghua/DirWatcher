#include "DirWatcher.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	DirWatcher w;
	w.show();
	return a.exec();
}
