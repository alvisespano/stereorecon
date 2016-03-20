#include <stdexcept>
#include "multiviewstereo.h"
#include "klt/klt.h"
#include "cv.h"

MultiViewStereo::MultiViewStereo(){
	clearImages();
}

/**
 * This method adds a new image to the list of shots to be processed.
 * In order to obtain best results it is recommended to supply images
 * in the same order of the motion of the camera. In fact this way the feature
 * tracker will be able to find more common reference points among
 * pairs of images.
 *
 * @param image The image to add to the list
 */
void MultiViewStereo::addImage(const QImage &image){

	if(!(width || height)){
		width = image.width();
		height = image.height();
	}else if((image.width()!=width)||(image.height()!=height)){
		throw std::invalid_argument(("Unable to add image of size " +  QString::number(image.width()) + "x" +  QString::number(image.height()) + " into a collection of size " +  QString::number(width) + "x" +  QString::number(height)).toStdString());
	}

	images.append(image);

}


QVector<QImage> MultiViewStereo::getImages(){

	return images;

}


void MultiViewStereo::clearImages(){

	width = 0;
	height = 0;
	images.clear();

}


/**
 * Dumps a QImage to a (monochrome) unsigned char buffer
 *
 * @param image The image to dump
 * @param data The buffer that will hold the monochrome data
 */
void MultiViewStereo::image_to_unsigned_char(const QImage &image, unsigned char* data) const{

	for(int y=0; y<image.height(); y++)
		for(int x=0; x<image.width(); x++)
			*data++ = qGray(image.pixel(x,y));

}


/**
 * Obtains a complete list of tracked features through all the images
 *
 * @param features_per_image The maximal number of features originate by each image
 */
TrackedFeatures* MultiViewStereo::getTrackedFeatures(int features_per_image) const{

	if(images.isEmpty()){
		throw std::runtime_error("Unable to track features on an empty image list");
	}

	// The structure that will contain all the tracked features
	TrackedFeatures* result = new TrackedFeatures();
	for(int image=0; image<images.size(); image++)
		result->append(ImageFeatures(0));

	// Creates the structures for tracking and the support memory areas for the greyscale images
	KLT_TrackingContext tracking_context = KLTCreateTrackingContext();

	tracking_context->mindist = 30;
	tracking_context->window_width = 30;
	tracking_context->window_height = 30;
//	tracking_context->lighting_insensitive = 1;
	tracking_context->max_residue = 10;
	tracking_context->max_iterations = 1000;
	tracking_context->min_displacement = 200;

	KLTChangeTCPyramid(tracking_context, 15);
	KLTUpdateTCBorder(tracking_context);

	KLT_FeatureList features_list = KLTCreateFeatureList(features_per_image);
	unsigned char* source_data = new unsigned char[width*height];
	unsigned char* destination_data = new unsigned char[width*height];


	for(int source=0; source<images.size(); source++){

		image_to_unsigned_char(images.at(source),source_data);

		for(int destination=0; destination<images.size(); destination++){

			image_to_unsigned_char(images.at(destination),destination_data);

			KLTSelectGoodFeatures(tracking_context, source_data, width, height, features_list);

			if(source != destination){
				KLTTrackFeatures(tracking_context, source_data, destination_data, width, height, features_list);
			}

			for(int i=0; i<features_list->nFeatures; i++) {
				FeaturePoint fp(features_list->feature[i]->x,features_list->feature[i]->y,features_list->feature[i]->val);
				(*result)[destination].append(fp);
			}

		}
		break;
	}

	// Releases all the resources allocated
	KLTFreeTrackingContext(tracking_context);
	KLTFreeFeatureList(features_list);
	delete[] source_data;
	delete[] destination_data;

	return result;
}
