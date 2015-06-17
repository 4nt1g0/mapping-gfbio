/*
 * util.cpp
 *
 *  Created on: 18.05.2015
 *      Author: mika
 */

#include "test/unittests/cache/util.h"

time_t parseIso8601DateTime(std::string dateTimeString) {
	const std::string dateTimeFormat{"%Y-%m-%dT%H:%M:%S"}; //TODO: we should allow millisec -> "%Y-%m-%dT%H:%M:%S.SSSZ" std::get_time and the tm struct dont have them.

	//std::stringstream dateTimeStream{dateTimeString}; //TODO: use this with gcc >5.0
	tm queryDateTime{0};
	//std::get_time(&queryDateTime, dateTimeFormat); //TODO: use this with gcc >5.0
	strptime(dateTimeString.c_str(), dateTimeFormat.c_str(), &queryDateTime); //TODO: remove this with gcc >5.0
	time_t queryTimestamp = timegm(&queryDateTime); //TODO: is there a c++ version for timegm?

	//TODO: parse millisec

	return (queryTimestamp);
}

void parseBBOX(double *bbox, const std::string bbox_str, epsg_t epsg, bool allow_infinite) {
	// &BBOX=0,0,10018754.171394622,10018754.171394622
	for(int i=0;i<4;i++)
		bbox[i] = NAN;

	// Figure out if we know the extent of the CRS
	// WebMercator, http://www.easywms.com/easywms/?q=en/node/3592
	//                               minx          miny         maxx         maxy
	double extent_webmercator[4] {-20037508.34 , -20037508.34 , 20037508.34 , 20037508.34};
	double extent_latlon[4]      {     -180    ,       -90    ,      180    ,       90   };
	double extent_msg[4]         { -5568748.276,  -5568748.276, 5568748.276,  5568748.276};

	double *extent = nullptr;
	if (epsg == EPSG_WEBMERCATOR)
		extent = extent_webmercator;
	else if (epsg == EPSG_LATLON)
		extent = extent_latlon;
	else if (epsg == EPSG_GEOSMSG)
		extent = extent_msg;


	std::string delimiters = " ,";
	size_t current, next = -1;
	int element = 0;
	do {
		current = next + 1;
		next = bbox_str.find_first_of(delimiters, current);
		std::string stringValue = bbox_str.substr(current, next - current);
		double value = 0;

		if (stringValue == "Infinity") {
			if (!allow_infinite)
				throw ArgumentException("cannot process BBOX with Infinity");
			if (!extent)
				throw ArgumentException("cannot process BBOX with Infinity and unknown CRS");
			value = std::max(extent[element], extent[(element+2)%4]);
		}
		else if (stringValue == "-Infinity") {
			if (!allow_infinite)
				throw ArgumentException("cannot process BBOX with Infinity");
			if (!extent)
				throw ArgumentException("cannot process BBOX with Infinity and unknown CRS");
			value = std::min(extent[element], extent[(element+2)%4]);
		}
		else {
			value = std::stod(stringValue);
			if (!std::isfinite(value))
				throw ArgumentException("BBOX contains entry that is not a finite number");
		}

		bbox[element++] = value;
	} while (element < 4 && next != std::string::npos);

	if (element != 4)
		throw ArgumentException("Could not parse BBOX parameter");

	/*
	 * OpenLayers insists on sending latitude in x and longitude in y.
	 * The MAPPING code (including gdal's projection classes) don't agree: east/west should be in x.
	 * The simple solution is to swap the x and y coordinates.
	 * OpenLayers 3 uses the axis orientation of the projection to determine the bbox axis order. https://github.com/openlayers/ol3/blob/master/src/ol/source/imagewmssource.js ~ line 317.
	 */
	if (epsg == EPSG_LATLON) {
		std::swap(bbox[0], bbox[1]);
		std::swap(bbox[2], bbox[3]);
	}

	// If no extent is known, just trust the client.
	if (extent) {
		double bbox_normalized[4];
		for (int i=0;i<4;i+=2) {
			bbox_normalized[i  ] = (bbox[i  ] - extent[0]) / (extent[2]-extent[0]);
			bbox_normalized[i+1] = (bbox[i+1] - extent[1]) / (extent[3]-extent[1]);
		}

		// Koordinaten können leicht ausserhalb liegen, z.B.
		// 20037508.342789, 20037508.342789
		for (int i=0;i<4;i++) {
			if (bbox_normalized[i] < 0.0 && bbox_normalized[i] > -0.001)
				bbox_normalized[i] = 0.0;
			else if (bbox_normalized[i] > 1.0 && bbox_normalized[i] < 1.001)
				bbox_normalized[i] = 1.0;
		}

		for (int i=0;i<4;i++) {
			if (bbox_normalized[i] < 0.0 || bbox_normalized[i] > 1.0)
				throw ArgumentException("BBOX exceeds extent");
		}
	}

	//bbox_normalized[1] = 1.0 - bbox_normalized[1];
	//bbox_normalized[3] = 1.0 - bbox_normalized[3];
}
