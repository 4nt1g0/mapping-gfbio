/*
 * cacheclient.h
 *
 *  Created on: 28.05.2015
 *      Author: mika
 */

#ifndef CLIENT_CLIENT_H_
#define CLIENT_CLIENT_H_

#include "cache/priv/transfer.h"
#include "datatypes/raster.h"
#include "cache/common.h"

#include <memory>

class CacheClient {
public:
	CacheClient( std::string index_host, uint32_t index_port );
	virtual ~CacheClient();
	std::unique_ptr<GenericRaster> get_raster(	const std::string &graph_json,
												const QueryRectangle &query,
												const GenericOperator::RasterQM query_mode = GenericOperator::RasterQM::EXACT );
private:
	DeliveryResponse read_index_response( SocketConnection &idx_con );
	std::unique_ptr<GenericRaster> fetch_raster( const DeliveryResponse &resp );
	std::string index_host;
	uint32_t index_port;
};

#endif /* CLIENT_CLIENT_H_ */
