#include "algorithmtester.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	AlgorithmTester w;
	w.show();
	return a.exec();
}
