
#include "datatypes/raster.h"
#include "datatypes/raster/raster_priv.h"
#include "operators/operator.h"
#include "datatypes/simplefeaturecollections/geosgeomutil.h"

#include "util/make_unique.h"

#include <geos/geom/GeometryFactory.h>
#include <geos/geom/CoordinateSequenceFactory.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/Point.h>
#include <geos/geom/PrecisionModel.h>
#include <geos/geom/prep/PreparedGeometryFactory.h>

#include <string>
#include <sstream>
#include "datatypes/pointcollection.h"
#include "datatypes/polygoncollection.h"

/**
 * Filter simple pointcollection by a polygoncollection
 */
class FilterPointsByGeometry : public GenericOperator {
	public:
		FilterPointsByGeometry(int sourcecounts[], GenericOperator *sources[], Json::Value &params);
		virtual ~FilterPointsByGeometry();

		virtual std::unique_ptr<PointCollection> getPointCollection(const QueryRectangle &rect, QueryProfiler &profiler);
};




FilterPointsByGeometry::FilterPointsByGeometry(int sourcecounts[], GenericOperator *sources[], Json::Value &params) : GenericOperator(sourcecounts, sources) {
	assumeSources(2);
}

FilterPointsByGeometry::~FilterPointsByGeometry() {
}
REGISTER_OPERATOR(FilterPointsByGeometry, "filterpointsbygeometry");

std::unique_ptr<PointCollection> FilterPointsByGeometry::getPointCollection(const QueryRectangle &rect, QueryProfiler &profiler) {
	//TODO: check projection
	const geos::geom::PrecisionModel pm;
	geos::geom::GeometryFactory gf = geos::geom::GeometryFactory(&pm, 4326);
	geos::geom::GeometryFactory* geometryFactory = &gf;

	auto points = getPointCollectionFromSource(0, rect, profiler, FeatureCollectionQM::SINGLE_ELEMENT_FEATURES);

	auto multiPolygons = getPolygonCollectionFromSource(0, rect, profiler, FeatureCollectionQM::ANY_FEATURE);

	auto geometry = GeosGeomUtil::createGeosPolygonCollection(*multiPolygons);
	//fprintf(stderr, "getGeom >> %f", geometry->getArea());

	size_t points_count = points->getFeatureCount();
	std::vector<bool> keep(points_count, false);

	auto prep = geos::geom::prep::PreparedGeometryFactory();

	size_t numgeom = geometry->getNumGeometries();
	for (size_t i=0; i< numgeom; i++){

		auto preparedGeometry = prep.prepare(geometry->getGeometryN(i));
		for (size_t idx=0;idx<points_count;idx++) {
			Coordinate &p = points->coordinates[idx];
			double x = p.x, y = p.y;

			const geos::geom::Coordinate coordinate(x, y);
			geos::geom::Point* pointGeom = geometryFactory->createPoint(coordinate);

			if (preparedGeometry->contains(pointGeom))
				keep[idx] = true;

			geometryFactory->destroyGeometry(pointGeom);
		}

		prep.destroy(preparedGeometry);
	}

	return points->filter(keep);
}
