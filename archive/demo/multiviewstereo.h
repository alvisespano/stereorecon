#ifndef MULTIVIEWSTEREO_H
#define MULTIVIEWSTEREO_H

#include <QVector>
#include <QImage>
#include "stereomath.h"

/**
 * Class FeaturePoint contains the location and the quality level of a tracked feature
 */
class FeaturePoint{

public:
	FeaturePoint() {}
	FeaturePoint(double x_, double y_, double quality_):x(x_),y(y_),quality(quality_){}
	double x, y, quality;

};

/**
 * This vector represents all the features extracted from an image. If the feature i is not contained in an
 * image, then the position i of this object is NULL
 */
typedef QVector<FeaturePoint> ImageFeatures;

/**
 * Collection of all the features extracted from a sequence of images
 */
typedef QVector<ImageFeatures> TrackedFeatures;

/**
 * Class MultiViewStereo embeds the algorithmic toolkit needed in order to obtain a
 * full 3D textured surface reconstruction starting from a collection of uncalibrated
 * multiple view shots.
 */
class MultiViewStereo{

public:
	MultiViewStereo();

	void addImage(const QImage &image);
	QVector<QImage> getImages();
	void clearImages();

	TrackedFeatures* getTrackedFeatures(int features_per_image) const;

private:

	// Images to be used for the scene reconstruction
	QVector<QImage> images;

	// With and height of all the images
	int width, height;

	void image_to_unsigned_char(const QImage &image, unsigned char* data) const;
};

#endif // MULTIVIEWSTEREO_H
