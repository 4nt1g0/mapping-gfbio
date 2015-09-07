/*
 * reorg_strategy.h
 *
 *  Created on: 03.08.2015
 *      Author: mika
 */

#ifndef REORG_STRATEGY_H_
#define REORG_STRATEGY_H_

#include "cache/index/index_cache.h"
#include "cache/priv/redistribution.h"
#include "util/gdal.h"
#include <map>
#include <unordered_map>
#include <memory>

// Reorg-strategies manage the reconfiguration of the cache
// on imbalance

class Node;

//
// Describes the reorganization-tasks for a specific node
//
class NodeReorgDescription : public ReorgDescription {
public:
	NodeReorgDescription( std::shared_ptr<Node> node );
	const std::shared_ptr<Node> node;
};

//
// Tells the index cache if and how to reorganize
// its entries in order to get a balanced usage
// across all nodes
//
class ReorgStrategy {
public:
	static std::unique_ptr<ReorgStrategy> by_name(const std::string &name);
	ReorgStrategy(double target_usage);
	virtual ~ReorgStrategy();
	virtual bool requires_reorg(const IndexCache &cache, const std::map<uint32_t,std::shared_ptr<Node>> &nodes ) const;
	virtual void reorganize(const IndexCache &cache, std::map<uint32_t,NodeReorgDescription> &result ) const = 0 ;

	static bool entry_less(const std::shared_ptr<IndexCacheEntry> &a, const std::shared_ptr<IndexCacheEntry> &b);
	static bool entry_greater(const std::shared_ptr<IndexCacheEntry> &a, const std::shared_ptr<IndexCacheEntry> &b);
	static double get_score( const IndexCacheEntry &entry );
protected:
	double get_target_usage( const IndexCache &cache, const std::map<uint32_t,NodeReorgDescription> &result ) const;
private:
	const double target_usage;
};

//
// This strategy never triggers reorganization
//
class NeverReorgStrategy : public ReorgStrategy {
public:
	NeverReorgStrategy();
	virtual ~NeverReorgStrategy();
	virtual bool requires_reorg( const IndexCache &cache, const std::map<uint32_t,std::shared_ptr<Node>> &nodes ) const;
	virtual void reorganize( const IndexCache &cache, std::map<uint32_t,NodeReorgDescription> &result ) const;
};

//
// This strategy simply redistributes entries
// to achieve approx. the same memory usage across
// all nodes
//
class CapacityReorgStrategy : public ReorgStrategy {
public:
	CapacityReorgStrategy(double target_usage);
	virtual ~CapacityReorgStrategy();
	virtual void reorganize( const IndexCache &cache, std::map<uint32_t,NodeReorgDescription> &result ) const;
};


//
// This strategy calculates the center of mass of over all entries
// and clusters nearby entries at a single node
//
class GeographicReorgStrategy : public ReorgStrategy {
	friend class NodePos;
public:
	GeographicReorgStrategy(double target_usage);
	virtual ~GeographicReorgStrategy();
	virtual void reorganize( const IndexCache &cache, std::map<uint32_t,NodeReorgDescription> &result ) const;
private:
	static GDAL::CRSTransformer geosmsg_trans;
	static GDAL::CRSTransformer webmercator_trans;
};


//
// This strategy calculates clusters cache-entries
// by similarity of their operator-graphs
//
class GraphReorgStrategy : public ReorgStrategy {
	friend class NodePos;
public:
	GraphReorgStrategy(double target_usage);
	virtual ~GraphReorgStrategy();
	virtual void reorganize( const IndexCache &cache, std::map<uint32_t,NodeReorgDescription> &result ) const;
};



#endif /* REORG_STRATEGY_H_ */