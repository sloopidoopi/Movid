/***********************************************************************
 ** Copyright (C) 2010 Movid Authors.  All rights reserved.
 **
 ** This file is part of the Movid Software.
 **
 ** This file may be distributed under the terms of the Q Public License
 ** as defined by Trolltech AS of Norway and appearing in the file
 ** LICENSE included in the packaging of this file.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** Contact info@movid.org if any conditions of this licensing are
 ** not clear to you.
 **
 **********************************************************************/


#include <assert.h>

#include "../moLog.h"
#include "../moModule.h"
#include "../moDataStream.h"
#include "moCombineModule.h"

MODULE_DECLARE(Combine, "native", "Take the maximum color from 2 image");

moCombineModule::moCombineModule() : moModule(MO_MODULE_INPUT|MO_MODULE_OUTPUT, 2, 1) {
	MODULE_INIT();

	this->input1 = NULL;
	this->input2 = NULL;
	this->output = new moDataStream("IplImage");
	this->output_buffer = NULL;

	// declare outputs
	this->input_infos[0] = new moDataStreamInfo(
			"image1", "IplImage", "Background image");
	this->input_infos[1] = new moDataStreamInfo(
			"image2", "IplImage", "Image to combine to background (black is transparent)");
	this->output_infos[0] = new moDataStreamInfo(
			"combine", "IplImage", "Result of the combine");
}

moCombineModule::~moCombineModule() {
}

void moCombineModule::notifyData(moDataStream *input) {
	IplImage* src = static_cast<IplImage*>(input->getData());
	assert( input->getFormat() == "IplImage" );
	if ( src == NULL )
		return;

	if ( this->output_buffer == NULL ) {
		this->output_buffer = cvCreateImage(cvGetSize(src), src->depth, src->nChannels);
		this->split = cvCreateImage(cvGetSize(src), src->depth, 1);
	} else {
		if ( this->output_buffer->width != src->width ||
			 this->output_buffer->height != src->height ) {
			LOG(MO_CRITICAL, "cannot combine image with different size");
		}
	}

	this->notifyUpdate();
}

void moCombineModule::update() {
	IplImage *d1 = NULL, *d2 = NULL;
	if ( this->input1 == NULL || this->output_buffer == NULL )
		return;

	this->input1->lock();
	d1 = (IplImage *)this->input1->getData();
	if ( d1 == NULL ) {
		this->input1->unlock();
		return;
	}

	d1 = cvCloneImage(d1);
	this->input1->unlock();

	if ( this->input2 == NULL ) {
		cvCopy(d1, this->output_buffer);
	} else {
		this->input2->lock();
		d2 = (IplImage *)this->input2->getData();
		if ( d2 == NULL ) {
			this->input2->unlock();
			cvReleaseImage(&d1);
			return;
		}

		d2 = cvCloneImage(d2);
		this->input2->unlock();

		cvSplit(d2, this->split, NULL, NULL, NULL);
		cvCopy(d1, this->output_buffer);
		cvCopy(d2, this->output_buffer, this->split);
	}

	this->output->push(this->output_buffer);

	cvReleaseImage(&d1);
	cvReleaseImage(&d2);
}

void moCombineModule::setInput(moDataStream *stream, int n) {
	assert( n == 0 || n == 1 );

	if ( n == 0 ) {
		if ( this->input1 != NULL )
			this->input1->removeObserver(this);
		this->input1 = stream;
		if ( stream->getFormat() != "IplImage" ) {
			this->setError("Input 0 accept only IplImage");
			this->input1 = NULL;
			return;
		}
	} else {
		if ( this->input2 != NULL )
			this->input2->removeObserver(this);
		this->input2 = stream;
		if ( stream->getFormat() != "IplImage" ) {
			this->setError("Input 1 accept only IplImage");
			this->input2 = NULL;
			return;
		}
	}

	if ( stream != NULL )
		stream->addObserver(this);
}

moDataStream *moCombineModule::getInput(int n) {
	if ( n == 0 )
		return this->input1;
	if ( n == 1 )
		return this->input2;

	this->setError("Invalid input index");
	return NULL;
}

moDataStream *moCombineModule::getOutput(int n) {
	if ( n != 0 ) {
		this->setError("Invalid output index");
		return NULL;
	}
	return this->output;
}
