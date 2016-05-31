#ifndef ALGORITHMTESTER_H
#define ALGORITHMTESTER_H

#include <QtWidgets/QMainWindow>
#include "ui_algorithmtester.h"

class AlgorithmTester : public QMainWindow
{
	Q_OBJECT

public:
	AlgorithmTester(QWidget *parent = 0);
	~AlgorithmTester();

private:
	Ui::AlgorithmTesterClass ui;
};

#endif // ALGORITHMTESTER_H
