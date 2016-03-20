#ifndef DEMOAPPLICATION_H
#define DEMOAPPLICATION_H

#include <QtGui/QMainWindow>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QFileDialog>
#include <QWheelEvent>

#include "multiviewstereo.h"

namespace Ui
{
    class DemoApplicationClass;
}

class DemoApplication : public QMainWindow
{
    Q_OBJECT

public:
    DemoApplication(QWidget *parent = 0);
    ~DemoApplication();

private:
	Ui::DemoApplicationClass *ui;
	QGraphicsScene scene;
	QPen pen;
	QVector<QColor> palette;

	MultiViewStereo multi_view_stereo;
	TrackedFeatures* features;

	 // These are the source images loaded by the user
	QList<QPointF> sourcePositions;

    // This method clears all the image data
    void clearData();

    void loadImagesFromDir(QString directoryName);

	void displayImagesInScene();

    void wheelEvent ( QWheelEvent * event );

private slots:
	 void on_actionFind_feature_points_triggered();
	 void on_verticalSlider_sliderMoved(int position);
    void on_actionOpen_images_directory_triggered();
    void on_actionQuit_triggered();
};

#endif // DEMOAPPLICATION_H
