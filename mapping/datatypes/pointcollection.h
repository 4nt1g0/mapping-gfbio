#ifndef DATATYPES_POINTCOLLECTION_H_
#define DATATYPES_POINTCOLLECTION_H_

#include "datatypes/simplefeaturecollection.h"
#include <memory>


/**
 * This collection contains Point-Features, each feature consisting of one or more points
 */
class PointCollection : public SimpleFeatureCollection {
private:
	template<typename C> class PointFeatureReference;
public:
	PointCollection(BinaryStream &stream);
	PointCollection(const SpatioTemporalReference &stref) : SimpleFeatureCollection(stref) {
		start_feature.push_back(0); //end of first feature
	}

	typedef SimpleFeatureIterator<PointCollection, PointFeatureReference> iterator;
	typedef SimpleFeatureIterator<const PointCollection, PointFeatureReference> const_iterator;

    inline iterator begin() {
    	return iterator(*this, 0);
    }
    inline iterator end() {
    	return iterator(*this, getFeatureCount());
    }

    inline const_iterator begin() const {
    	return const_iterator(*this, 0);
    }
    inline const_iterator end() const {
    	return const_iterator(*this, getFeatureCount());
    }

	//starting index of individual features in the points vector, last entry indicates first index out of bounds of coordinates
	//thus iterating over features has to stop at start_feature.size() -2
	std::vector<uint32_t> start_feature;

	void toStream(BinaryStream &stream);

	//add a new coordinate, to a new feature. After adding all coordinates, finishFeature() has to be called
	void addCoordinate(double x, double y);
	//finishes the definition of the new feature, returns new feature index
	size_t finishFeature();
	//add a new feature consisting of a single coordinate, returns new feature index
	size_t addSinglePointFeature(const Coordinate coordinate);

	std::unique_ptr<PointCollection> filter(const std::vector<bool> &keep);
	std::unique_ptr<PointCollection> filter(const std::vector<char> &keep);

	std::string hash();

	virtual std::string toGeoJSON(bool displayMetadata) const;
	virtual std::string toCSV() const;

	virtual bool isSimple() const final;

	virtual size_t getFeatureCount() const final {
		return start_feature.size() - 1;
	}

	std::string getAsString();

	virtual ~PointCollection(){};
private:
	/*
	 * Finally, implement the helper classes for iteration
	 */
	template<typename C>
	class PointFeatureReference {
		public:
			PointFeatureReference(C &pc, size_t idx) : pc(pc), idx(idx) {};

			typedef decltype(std::declval<C>().coordinates.begin()) iterator;
			typedef decltype(std::declval<C>().coordinates.cbegin()) const_iterator;

			iterator begin() {
		    	return std::next(pc.coordinates.begin(), pc.start_feature[idx]);
		    }
			iterator end() {
		    	return std::next(pc.coordinates.begin(), pc.start_feature[idx+1]);
		    }

		    const_iterator begin() const {
		    	return std::next(pc.coordinates.cbegin(), pc.start_feature[idx]);
		    }
		    const_iterator end() const {
		    	return std::next(pc.coordinates.cbegin(), pc.start_feature[idx+1]);
		    }

		    size_t size() const {
		    	return pc.start_feature[idx+1] - pc.start_feature[idx];
		    }

		    operator size_t() const {
		    	return idx;
		    }
		private:
			C &pc;
			const size_t idx;
	};
};


#endif