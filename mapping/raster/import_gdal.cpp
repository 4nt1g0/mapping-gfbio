
#include "raster/raster_priv.h"

#include <stdint.h>
#include <mutex>
#include <sstream>


static std::mutex gdal_init_mutex;
static bool gdal_is_initialized = false;

void gdal_init() {
	if (gdal_is_initialized)
		return;
	std::lock_guard<std::mutex> guard(gdal_init_mutex);
	if (gdal_is_initialized)
		return;
	gdal_is_initialized = true;
	GDALAllRegister();
	//GetGDALDriverManager()->AutoLoadDrivers();
}


static GenericRaster *GDALImporter_loadRaster(GDALDataset *dataset, int rasteridx, double origin_x, double origin_y, double scale_x, double scale_y, epsg_t default_epsg) {
	GDALRasterBand  *poBand;
	int             nBlockXSize, nBlockYSize;
	int             bGotMin, bGotMax;
	double          adfMinMax[2];

	poBand = dataset->GetRasterBand( rasteridx );
	poBand->GetBlockSize( &nBlockXSize, &nBlockYSize );

	GDALDataType type = poBand->GetRasterDataType();

/*
	printf( "Block=%dx%d Type=%s, ColorInterp=%s\n",
		nBlockXSize, nBlockYSize,
		GDALGetDataTypeName(poBand->GetRasterDataType()),
		GDALGetColorInterpretationName(
				poBand->GetColorInterpretation()) );
*/
	adfMinMax[0] = poBand->GetMinimum( &bGotMin );
	adfMinMax[1] = poBand->GetMaximum( &bGotMax );
	if( ! (bGotMin && bGotMax) )
		GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);

	int hasnodata = true;
	int success;
	double nodata = poBand->GetNoDataValue(&success);
	if (!success /*|| nodata < adfMinMax[0] || nodata > adfMinMax[1]*/) {
		hasnodata = false;
		nodata = 0;
	}
	/*
	if (nodata < adfMinMax[0] || nodata > adfMinMax[1]) {

	}
	*/
/*
	printf( "Min=%.3g, Max=%.3g\n", adfMinMax[0], adfMinMax[1] );

	if( poBand->GetOverviewCount() > 0 )
		printf( "Band has %d overviews.\n", poBand->GetOverviewCount() );

	if( poBand->GetColorTable() != NULL )
		printf( "Band has a color table with %d entries.\n",
			poBand->GetColorTable()->GetColorEntryCount() );
*/
	// Read Pixel data
	int   nXSize = poBand->GetXSize();
	int   nYSize = poBand->GetYSize();

	epsg_t epsg = default_epsg; //EPSG_UNKNOWN;

	RasterMetadata rastermeta(epsg, nXSize, nYSize, origin_x, origin_y, scale_x, scale_y);
	// TODO: Workaround für .rst mit falschen max-werten!
	double maxvalue = adfMinMax[1];
	if (maxvalue == 255 && type == GDT_Int16) maxvalue = 1023;
	if (type == GDT_Byte) maxvalue = 255;
	ValueMetadata valuemeta(type, adfMinMax[0], maxvalue, hasnodata, nodata);
	//printf("loading raster with %g -> %g valuerange\n", adfMinMax[0], adfMinMax[1]);

	GenericRaster *raster = GenericRaster::create(rastermeta, valuemeta);
	void *buffer = raster->getDataForWriting();
	//int bpp = raster->getBPP();

	/*
CPLErr GDALRasterBand::RasterIO( GDALRWFlag eRWFlag,
                                 int nXOff, int nYOff, int nXSize, int nYSize,
                                 void * pData, int nBufXSize, int nBufYSize,
                                 GDALDataType eBufType,
                                 int nPixelSpace,
                                 int nLineSpace )
	*/

	poBand->RasterIO( GF_Read, 0, 0, nXSize, nYSize,
		buffer, nXSize, nYSize, type, 0, 0);


	return raster;
}

GenericRaster *GenericRaster::fromGDAL(const char *filename, int rasterid, epsg_t epsg) {
	gdal_init();

	GDALDataset *dataset = (GDALDataset *) GDALOpen(filename, GA_ReadOnly);

	if (dataset == NULL)
		throw ImporterException("Could not open dataset");

/*
	printf( "Driver: %s/%s\n",
		dataset->GetDriver()->GetDescription(),
		dataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );

	printf( "Size is %dx%dx%d\n",
		dataset->GetRasterXSize(), dataset->GetRasterYSize(),
		dataset->GetRasterCount() );

	if( dataset->GetProjectionRef()  != NULL )
			printf( "Projection is `%s'\n", dataset->GetProjectionRef() );
*/
	double adfGeoTransform[6];

	// http://www.gdal.org/classGDALDataset.html#af9593cc241e7d140f5f3c4798a43a668
	if( dataset->GetGeoTransform( adfGeoTransform ) != CE_None ) {
		GDALClose(dataset);
		throw ImporterException("no GeoTransform information in raster");
	}
/*
	if (adfGeoTransform[2] != 0 || adfGeoTransform[4] != 0) {
		GDALClose(dataset);
		std::stringstream ss;
		ss << "Raster is using an unsupported GeoTransform: (" << adfGeoTransform[0] << "/" << adfGeoTransform[1] << "/" << adfGeoTransform[2] << "/" << adfGeoTransform[3] << ")";
		throw ImporterException(ss.str());
	}
*/
/*
	{
		printf( "Origin = (%.6f,%.6f)\n",
			adfGeoTransform[0], adfGeoTransform[3] );

		printf( "Pixel Size = (%.6f,%.6f)\n",
			adfGeoTransform[1], adfGeoTransform[5] );
	}
*/

	int rastercount = dataset->GetRasterCount();

	if (rasterid < 1 || rasterid > rastercount) {
		GDALClose(dataset);
		throw ImporterException("rasterid not found");
	}

	GenericRaster *raster = GDALImporter_loadRaster(dataset, rasterid, adfGeoTransform[0], adfGeoTransform[3], adfGeoTransform[1], adfGeoTransform[5], epsg);

	/*
	char** md = dataset->GetMetadataDomainList();
	if (md) {
		char** iter = md;
		int count = 0;
		while (iter != nullptr && *iter != nullptr) {
			printf("Metadata domain: %s\n", *iter);
			count++;
		}
		CSLDestroy(md);
		printf("Found %d Metadata domains\n", count);
	}
	*/

	GDALClose(dataset);

	return raster;
}



template<typename T> void Raster2D<T>::toGDAL(const char *, const char *) {

}
#if 0
template<typename T> void Raster2D<T>::toGDAL(const char *filename, const char *drivername) {
	gdal_init();


	GDALDriver *poDriver;
	char **papszMetadata;


	int count = GetGDALDriverManager()->GetDriverCount();
	printf("GDAL has %d drivers\n", count);
	for (int i=0;i<count;i++) {
		poDriver = GetGDALDriverManager()->GetDriver(i);

		if( poDriver == NULL ) {
			continue;
		}

		papszMetadata = poDriver->GetMetadata();
		if( CSLFetchBoolean( papszMetadata, GDAL_DCAP_CREATE, FALSE ) )
			printf( "Driver %d supports Create() method.\n", i );
		if( CSLFetchBoolean( papszMetadata, GDAL_DCAP_CREATECOPY, FALSE ) )
			printf( "Driver %d supports CreateCopy() method.\n", i );

	}

	poDriver = GetGDALDriverManager()->GetDriverByName(drivername);

	if( poDriver == NULL ) {
		printf( "Driver %s not found.\n", drivername);
		return;
	}

	papszMetadata = poDriver->GetMetadata();
	if( CSLFetchBoolean( papszMetadata, GDAL_DCAP_CREATE, FALSE ) )
		printf( "Driver %s supports Create() method.\n", drivername );
	if( CSLFetchBoolean( papszMetadata, GDAL_DCAP_CREATECOPY, FALSE ) )
		printf( "Driver %s supports CreateCopy() method.\n", drivername );

	// ...
}
#endif

RASTER_PRIV_INSTANTIATE_ALL
