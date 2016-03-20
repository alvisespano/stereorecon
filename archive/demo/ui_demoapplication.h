/********************************************************************************
** Form generated from reading ui file 'demoapplication.ui'
**
** Created: Wed Jun 17 10:20:15 2009
**      by: Qt User Interface Compiler version 4.5.0
**
** WARNING! All changes made in this file will be lost when recompiling ui file!
********************************************************************************/

#ifndef UI_DEMOAPPLICATION_H
#define UI_DEMOAPPLICATION_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGraphicsView>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QProgressBar>
#include <QtGui/QSlider>
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DemoApplicationClass
{
public:
    QAction *actionOpen_images_directory;
    QAction *actionFind_feature_points;
    QAction *actionQuit;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout_3;
    QTabWidget *tabWidget;
    QWidget *photosTab;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QSlider *verticalSlider;
    QGraphicsView *sourceImagesView;
    QWidget *featuresTab;
    QVBoxLayout *verticalLayout_2;
    QProgressBar *progressBar;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *DemoApplicationClass)
    {
        if (DemoApplicationClass->objectName().isEmpty())
            DemoApplicationClass->setObjectName(QString::fromUtf8("DemoApplicationClass"));
        DemoApplicationClass->resize(539, 403);
        actionOpen_images_directory = new QAction(DemoApplicationClass);
        actionOpen_images_directory->setObjectName(QString::fromUtf8("actionOpen_images_directory"));
        QIcon icon;
        icon.addPixmap(QPixmap(QString::fromUtf8(":/icons/resources/insert-image.png")), QIcon::Normal, QIcon::Off);
        actionOpen_images_directory->setIcon(icon);
        actionFind_feature_points = new QAction(DemoApplicationClass);
        actionFind_feature_points->setObjectName(QString::fromUtf8("actionFind_feature_points"));
        QIcon icon1;
        icon1.addPixmap(QPixmap(QString::fromUtf8(":/icons/resources/pin2.png")), QIcon::Normal, QIcon::Off);
        actionFind_feature_points->setIcon(icon1);
        actionQuit = new QAction(DemoApplicationClass);
        actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
        QIcon icon2;
        icon2.addPixmap(QPixmap(QString::fromUtf8(":/icons/resources/process-stop.png")), QIcon::Normal, QIcon::Off);
        actionQuit->setIcon(icon2);
        centralWidget = new QWidget(DemoApplicationClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
        centralWidget->setSizePolicy(sizePolicy);
        verticalLayout_3 = new QVBoxLayout(centralWidget);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setMargin(0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        tabWidget = new QTabWidget(centralWidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        photosTab = new QWidget();
        photosTab->setObjectName(QString::fromUtf8("photosTab"));
        sizePolicy.setHeightForWidth(photosTab->sizePolicy().hasHeightForWidth());
        photosTab->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(photosTab);
        verticalLayout->setSpacing(4);
        verticalLayout->setMargin(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setMargin(4);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        verticalSlider = new QSlider(photosTab);
        verticalSlider->setObjectName(QString::fromUtf8("verticalSlider"));
        verticalSlider->setOrientation(Qt::Vertical);

        horizontalLayout->addWidget(verticalSlider);

        sourceImagesView = new QGraphicsView(photosTab);
        sourceImagesView->setObjectName(QString::fromUtf8("sourceImagesView"));
        sourceImagesView->setSceneRect(QRectF(0, 0, 0, 0));
        sourceImagesView->setDragMode(QGraphicsView::ScrollHandDrag);

        horizontalLayout->addWidget(sourceImagesView);


        verticalLayout->addLayout(horizontalLayout);

        tabWidget->addTab(photosTab, QString());
        featuresTab = new QWidget();
        featuresTab->setObjectName(QString::fromUtf8("featuresTab"));
        verticalLayout_2 = new QVBoxLayout(featuresTab);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setMargin(11);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        tabWidget->addTab(featuresTab, QString());

        verticalLayout_3->addWidget(tabWidget);

        progressBar = new QProgressBar(centralWidget);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setEnabled(true);
        progressBar->setMaximum(1000);
        progressBar->setValue(0);

        verticalLayout_3->addWidget(progressBar);

        DemoApplicationClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(DemoApplicationClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 539, 25));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        DemoApplicationClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(DemoApplicationClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        DemoApplicationClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(DemoApplicationClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        DemoApplicationClass->setStatusBar(statusBar);

        menuBar->addAction(menuFile->menuAction());
        menuFile->addAction(actionOpen_images_directory);
        menuFile->addAction(actionFind_feature_points);
        menuFile->addSeparator();
        menuFile->addAction(actionQuit);
        mainToolBar->addAction(actionOpen_images_directory);
        mainToolBar->addAction(actionFind_feature_points);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actionQuit);

        retranslateUi(DemoApplicationClass);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(DemoApplicationClass);
    } // setupUi

    void retranslateUi(QMainWindow *DemoApplicationClass)
    {
        DemoApplicationClass->setWindowTitle(QApplication::translate("DemoApplicationClass", "DemoApplication", 0, QApplication::UnicodeUTF8));
        actionOpen_images_directory->setText(QApplication::translate("DemoApplicationClass", "Open images directory", 0, QApplication::UnicodeUTF8));
        actionFind_feature_points->setText(QApplication::translate("DemoApplicationClass", "Find feature points", 0, QApplication::UnicodeUTF8));
        actionQuit->setText(QApplication::translate("DemoApplicationClass", "Quit", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(photosTab), QApplication::translate("DemoApplicationClass", "Photos", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(featuresTab), QApplication::translate("DemoApplicationClass", "Features", 0, QApplication::UnicodeUTF8));
        menuFile->setTitle(QApplication::translate("DemoApplicationClass", "File", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class DemoApplicationClass: public Ui_DemoApplicationClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DEMOAPPLICATION_H
