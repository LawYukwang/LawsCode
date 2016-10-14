/********************************************************************************
** Form generated from reading UI file 'algorithmtester.ui'
**
** Created by: Qt User Interface Compiler version 5.2.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ALGORITHMTESTER_H
#define UI_ALGORITHMTESTER_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AlgorithmTesterClass
{
public:
    QAction *actionOpen;
    QAction *actionSave;
    QAction *actionAccuracy;
    QAction *actionAdjust;
    QAction *actionManual;
    QAction *actionSave_as;
    QAction *actionExit;
    QAction *actionRect;
    QAction *actionCircle;
    QAction *actionSelect_all;
    QAction *actionAdd_defect;
    QWidget *centralWidget;
    QMenuBar *menuBar;
    QMenu *file;
    QMenu *edit;
    QMenu *menuSelect_region;
    QMenu *function;
    QMenu *help;
    QStatusBar *statusBar;
    QToolBar *mainToolBar;

    void setupUi(QMainWindow *AlgorithmTesterClass)
    {
        if (AlgorithmTesterClass->objectName().isEmpty())
            AlgorithmTesterClass->setObjectName(QStringLiteral("AlgorithmTesterClass"));
        AlgorithmTesterClass->resize(934, 676);
        actionOpen = new QAction(AlgorithmTesterClass);
        actionOpen->setObjectName(QStringLiteral("actionOpen"));
        actionSave = new QAction(AlgorithmTesterClass);
        actionSave->setObjectName(QStringLiteral("actionSave"));
        actionSave->setMenuRole(QAction::TextHeuristicRole);
        actionAccuracy = new QAction(AlgorithmTesterClass);
        actionAccuracy->setObjectName(QStringLiteral("actionAccuracy"));
        actionAdjust = new QAction(AlgorithmTesterClass);
        actionAdjust->setObjectName(QStringLiteral("actionAdjust"));
        actionManual = new QAction(AlgorithmTesterClass);
        actionManual->setObjectName(QStringLiteral("actionManual"));
        actionSave_as = new QAction(AlgorithmTesterClass);
        actionSave_as->setObjectName(QStringLiteral("actionSave_as"));
        actionExit = new QAction(AlgorithmTesterClass);
        actionExit->setObjectName(QStringLiteral("actionExit"));
        actionRect = new QAction(AlgorithmTesterClass);
        actionRect->setObjectName(QStringLiteral("actionRect"));
        actionCircle = new QAction(AlgorithmTesterClass);
        actionCircle->setObjectName(QStringLiteral("actionCircle"));
        actionSelect_all = new QAction(AlgorithmTesterClass);
        actionSelect_all->setObjectName(QStringLiteral("actionSelect_all"));
        actionAdd_defect = new QAction(AlgorithmTesterClass);
        actionAdd_defect->setObjectName(QStringLiteral("actionAdd_defect"));
        centralWidget = new QWidget(AlgorithmTesterClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        AlgorithmTesterClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(AlgorithmTesterClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 934, 23));
        file = new QMenu(menuBar);
        file->setObjectName(QStringLiteral("file"));
        edit = new QMenu(menuBar);
        edit->setObjectName(QStringLiteral("edit"));
        menuSelect_region = new QMenu(edit);
        menuSelect_region->setObjectName(QStringLiteral("menuSelect_region"));
        function = new QMenu(menuBar);
        function->setObjectName(QStringLiteral("function"));
        help = new QMenu(menuBar);
        help->setObjectName(QStringLiteral("help"));
        AlgorithmTesterClass->setMenuBar(menuBar);
        statusBar = new QStatusBar(AlgorithmTesterClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        AlgorithmTesterClass->setStatusBar(statusBar);
        mainToolBar = new QToolBar(AlgorithmTesterClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        AlgorithmTesterClass->addToolBar(Qt::TopToolBarArea, mainToolBar);

        menuBar->addAction(file->menuAction());
        menuBar->addAction(edit->menuAction());
        menuBar->addAction(function->menuAction());
        menuBar->addAction(help->menuAction());
        file->addAction(actionOpen);
        file->addAction(actionSave);
        file->addAction(actionSave_as);
        file->addSeparator();
        file->addAction(actionExit);
        edit->addAction(menuSelect_region->menuAction());
        edit->addAction(actionSelect_all);
        menuSelect_region->addAction(actionRect);
        menuSelect_region->addAction(actionCircle);
        function->addAction(actionAccuracy);
        function->addAction(actionAdjust);
        function->addAction(actionAdd_defect);
        help->addAction(actionManual);

        retranslateUi(AlgorithmTesterClass);

        QMetaObject::connectSlotsByName(AlgorithmTesterClass);
    } // setupUi

    void retranslateUi(QMainWindow *AlgorithmTesterClass)
    {
        AlgorithmTesterClass->setWindowTitle(QApplication::translate("AlgorithmTesterClass", "AlgorithmTester", 0));
        actionOpen->setText(QApplication::translate("AlgorithmTesterClass", "\346\211\223\345\274\200", 0));
        actionSave->setText(QApplication::translate("AlgorithmTesterClass", "\344\277\235\345\255\230", 0));
        actionAccuracy->setText(QApplication::translate("AlgorithmTesterClass", "\345\207\206\347\241\256\347\216\207\350\256\241\347\256\227", 0));
        actionAdjust->setText(QApplication::translate("AlgorithmTesterClass", "\345\217\202\346\225\260\350\260\203\350\212\202", 0));
        actionManual->setText(QApplication::translate("AlgorithmTesterClass", "\344\275\277\347\224\250\350\257\264\346\230\216", 0));
        actionSave_as->setText(QApplication::translate("AlgorithmTesterClass", "\345\217\246\345\255\230\344\270\272", 0));
        actionExit->setText(QApplication::translate("AlgorithmTesterClass", "\351\200\200\345\207\272", 0));
        actionRect->setText(QApplication::translate("AlgorithmTesterClass", "\346\226\271\345\275\242", 0));
        actionCircle->setText(QApplication::translate("AlgorithmTesterClass", "\345\234\206\345\275\242", 0));
        actionSelect_all->setText(QApplication::translate("AlgorithmTesterClass", "\351\200\211\345\217\226\346\225\264\344\275\223", 0));
        actionAdd_defect->setText(QApplication::translate("AlgorithmTesterClass", "\347\221\225\347\226\265\346\250\241\346\213\237", 0));
        file->setTitle(QApplication::translate("AlgorithmTesterClass", "\346\226\207\344\273\266", 0));
        edit->setTitle(QApplication::translate("AlgorithmTesterClass", "\347\274\226\350\276\221", 0));
        menuSelect_region->setTitle(QApplication::translate("AlgorithmTesterClass", "\351\200\211\345\217\226", 0));
        function->setTitle(QApplication::translate("AlgorithmTesterClass", "\345\212\237\350\203\275", 0));
        help->setTitle(QApplication::translate("AlgorithmTesterClass", "\345\270\256\345\212\251", 0));
    } // retranslateUi

};

namespace Ui {
    class AlgorithmTesterClass: public Ui_AlgorithmTesterClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ALGORITHMTESTER_H
