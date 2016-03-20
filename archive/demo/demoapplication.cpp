#include "demoapplication.h"
#include "ui_demoapplication.h"
#include <iostream>
#include <math.h>
#include <QGLWidget>

DemoApplication::DemoApplication(QWidget *parent)
	 : QMainWindow(parent), ui(new Ui::DemoApplicationClass)
{
    ui->setupUi(this);
	 ui->progressBar->hide();
	features = NULL;
	 for(int c=0; c<10000; c++)
		palette.append(QColor(qrand()%256,qrand()%256,qrand()%256));

	 // Auto loads images for debugging purpouses
//    loadImagesFromDir("testImages/seq1");
//	 loadImagesFromDir("/home/andrea/Scrivania/test-2");
	 loadImagesFromDir("/home/andrea/Scrivania/test-images");
}

DemoApplication::~DemoApplication()
{
    delete ui;
}

void DemoApplication::on_actionQuit_triggered()
{
    exit(0);
}

void DemoApplication::clearData(){
	ui->verticalSlider->setValue(20);
	if(features != NULL){
		delete(features);
	}
	features = NULL;
}

void DemoApplication::loadImagesFromDir(QString directoryName)
{
	QDir myDir(directoryName);
	QStringList list = myDir.entryList();
	int increment = (ui->progressBar->maximum()-ui->progressBar->minimum())/list.size();
	statusBar()->showMessage("Loading images from: " + directoryName);
	ui->progressBar->setValue(ui->progressBar->minimum());
	ui->progressBar->show();
	QSize size(0,0);
	clearData();
	multi_view_stereo.clearImages();
	foreach (QString s, list){
		QImage loadedImage = QImage(myDir.absoluteFilePath(s)).scaledToWidth(640,Qt::SmoothTransformation);
		if(!loadedImage.isNull()){
			multi_view_stereo.addImage(loadedImage);
		}
		ui->progressBar->setValue(ui->progressBar->value()+increment);
	}
	ui->progressBar->hide();
	ui->sourceImagesView->setScene(&scene);
	ui->sourceImagesView->setRenderHint(QPainter::Antialiasing);
//	ui->sourceImagesView->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
	ui->sourceImagesView->activateWindow();
	displayImagesInScene();
}


void DemoApplication::displayImagesInScene(){
	scene.clear();
	sourcePositions.clear();
	QPointF position(0,0);
	QVector<QImage> sourceImages = multi_view_stereo.getImages();
	foreach (QImage image, sourceImages){
		QGraphicsPixmapItem* item = scene.addPixmap(QPixmap::fromImage(image));
		item->setOffset(position);
		sourcePositions.append(position);
		position.setX(position.x() + image.width());
	 }
	 pen.setWidth(2);
	 pen.setBrush(Qt::red);
	 pen.setCapStyle(Qt::RoundCap);
	 pen.setJoinStyle(Qt::RoundJoin);

	if(features != NULL){

		for(int image=0; image<features->size(); image++){
			const ImageFeatures& feature_points = features->at(image);
			for(int feature=0; feature<feature_points.size(); feature++){
				const FeaturePoint& feature_point = feature_points.at(feature);
				if(feature_point.quality >= 0){
					QPointF position = sourcePositions.at(image);
					int x = position.x() + feature_point.x;
					int y = position.y() + feature_point.y;
					pen.setColor(palette.at(feature));
					scene.addEllipse(x-4,y-4,8,8,pen)->setZValue(10);
				}
			}
		}

	}

}


void DemoApplication::on_actionOpen_images_directory_triggered()
{
    QString directoryName = QFileDialog::getExistingDirectory(this,"Select photo directory","", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    loadImagesFromDir(directoryName);
}

void DemoApplication::wheelEvent (QWheelEvent* event ){
    return;
	 if(ui->tabWidget->currentWidget()==ui->photosTab){
      qreal scaleFactor = pow((double)2, -event->delta() / 240.0);
      qreal factor = ui->sourceImagesView->matrix().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
      if (factor < 0.07 || factor > 100) return;
      ui->sourceImagesView->scale(scaleFactor, scaleFactor);
    }
}

void DemoApplication::on_verticalSlider_sliderMoved(int position)
{
    qreal scaleFactor = (position+10.0)/30.0;
    QMatrix transform = ui->sourceImagesView->matrix();
    transform.setMatrix(scaleFactor,0,0,scaleFactor,transform.dx(),transform.dy());
    ui->sourceImagesView->setMatrix(transform);
}

void DemoApplication::on_actionFind_feature_points_triggered()
{
	int features_number = 100;
	features = multi_view_stereo.getTrackedFeatures(features_number);
	displayImagesInScene();
}
