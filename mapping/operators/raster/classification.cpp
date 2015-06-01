#include "datatypes/raster.h"
#include "datatypes/raster/typejuggling.h"
#include "raster/opencl.h"
#include "operators/operator.h"

#include <memory>
#include <cmath>
#include <json/json.h>
#include <algorithm>


class ClassificationOperator : public GenericOperator {
	public:
		ClassificationOperator(int sourcecounts[], GenericOperator *sources[], Json::Value &params);
		virtual ~ClassificationOperator();

		virtual std::unique_ptr<GenericRaster> getRaster(const QueryRectangle &rect, QueryProfiler &profiler);
	protected:
		void writeSemanticParameters(std::ostringstream& stream);
	private:
		std::vector<float> classification_lower_border{}, classification_upper_border{};
		std::vector<int> classification_classes{};
		bool reclassNoData{false};
		int noDataClass{0};
};



ClassificationOperator::ClassificationOperator(int sourcecounts[], GenericOperator *sources[], Json::Value &params) : GenericOperator(sourcecounts, sources) {
	assumeSources(1);

	//get the easy things first
	reclassNoData = params.get("reclassNoData", false).asBool();
	noDataClass = params.get("noDataClass", 0).asInt();

	//now get the classification list. This should be an array of arrays where the inner arrays contain exact 3 elements. Example: [[lowerBorder, upperBorder, class], [1,5,1],[5,7,2]]
	Json::Value classification_list = params["RemapRange"];
	if(!classification_list.isArray())
		throw OperatorException("Classification: \"classification_list\" is no array! ->" + classification_list.asString());

	if ( (classification_list.size() <= 0)  & (!reclassNoData))
		throw OperatorException("Classification: result will only contain NoData values!");

	for (Json::Value::ArrayIndex i=0;i<classification_list.size();i++) {
		Json::Value classification_case = classification_list[i];
		if(!classification_case.isArray())
			throw OperatorException("Classification: \"classification_case\" on position "+std::to_string(i)+" is no array! ->" + classification_case.asString());
		if(classification_case.size() != 3)
			throw OperatorException("Classification: \"classification_case\" on position "+std::to_string(i)+" is to short/long! Expected: size() == 3 ->" + classification_case.asString());
		//now get the values and push them into the vectors
		Json::Value value = classification_case[0];
		if(!value.isConvertibleTo(Json::ValueType::realValue))
			throw OperatorException("Classification: \"lower_border\" on position "+std::to_string(i)+" is not convertible to a real value ->" + classification_case.asString());
		classification_lower_border.push_back(static_cast<float>(value.asDouble()));

		value = classification_case[1];
		if(!value.isConvertibleTo(Json::ValueType::realValue))
			throw OperatorException("Classification: \"upper_border\" on position "+std::to_string(i)+" is not convertible to a real value ->" + classification_case.asString());
		classification_upper_border.push_back(static_cast<float>(value.asDouble()));

		value = classification_case[2];
		if(!value.isConvertibleTo(Json::ValueType::intValue))
			throw OperatorException("Classification: \"upper_border\" on position "+std::to_string(i)+" is not convertible to an int value ->" + classification_case.asString());
		classification_classes.push_back(value.asInt());
	}
}
ClassificationOperator::~ClassificationOperator() {
}

REGISTER_OPERATOR(ClassificationOperator, "reclass");

void ClassificationOperator::writeSemanticParameters(std::ostringstream& stream) {
	if(!(classification_lower_border.size() == classification_upper_border.size() && classification_upper_border.size() == classification_classes.size()))
		throw OperatorException("ClassificationOperator::writeSemanticParameters: unequal parameter vector sizes!");

	const size_t size =classification_classes.size();

	stream << "\"RemapRange:\":[";
	if(size >= 1){
		stream << "[" << classification_lower_border.at(0) <<"," <<classification_upper_border.at(0) <<","<< classification_classes.at(0) << "]";
	}
	for(size_t i = 1; i < size; i++){
		stream << ",[" << classification_lower_border.at(i) <<"," <<classification_upper_border.at(i) <<","<< classification_classes.at(i) << "]";
	}
	stream << "],\"reclassNoData\":" << reclassNoData <<",\"noDataClass\":" << noDataClass << "]";
}


#include "operators/raster/classification_kernels.cl.h"

std::unique_ptr<GenericRaster> ClassificationOperator::getRaster(const QueryRectangle &rect, QueryProfiler &profiler) {
	const auto raster_in = getRasterFromSource(0, rect, profiler);

	RasterOpenCL::init();
	raster_in->setRepresentation(GenericRaster::Representation::OPENCL);

	const auto min_max_classes = std::minmax_element(classification_classes.begin(), classification_classes.end());
	const double min = std::min(*min_max_classes.first, noDataClass);
	const double max = std::max(*min_max_classes.second, noDataClass);


	DataDescription out_data_description{GDT_Int32, min, max};
	out_data_description.addNoData();

	const int new_nodata_class = (reclassNoData)?noDataClass:static_cast<int>(out_data_description.no_data);

	auto raster_out = GenericRaster::create(out_data_description, *raster_in, GenericRaster::Representation::OPENCL);

	RasterOpenCL::CLProgram prog;
	prog.setProfiler(profiler);
	prog.addOutRaster(raster_out.get());
	prog.addInRaster(raster_in.get());
	prog.compile(operators_raster_classification_kernels, "classificationByRangeKernel");
	prog.addArg(classification_lower_border);
	prog.addArg(classification_upper_border);
	prog.addArg(classification_classes);
	prog.addArg(static_cast<int>(classification_classes.size()));
	prog.addArg(new_nodata_class);
	prog.run();

	return (raster_out);
}
