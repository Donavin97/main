/***************************************************************************
 * Copyright (C) GFZ Potsdam                                               *
 * All rights reserved.                                                    *
 *                                                                         *
 * GNU Affero General Public License Usage                                 *
 * This file may be used under the terms of the GNU Affero                 *
 * Public License version 3.0 as published by the Free Software Foundation *
 * and appearing in the file LICENSE included in the packaging of this     *
 * file. Please review the following information to ensure the GNU Affero  *
 * Public License version 3.0 requirements will be met:                    *
 * https://www.gnu.org/licenses/agpl-3.0.html.                             *
 ***************************************************************************/

#define SEISCOMP_COMPONENT PSLoc

#include <seiscomp/logging/log.h>
#include <seiscomp/math/geo.h>

#include "phasecluster.h"

#include <cmath>
#include <algorithm>
#include <numeric>
#include <queue>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
constexpr double EARTH_RADIUS_KM = 6371.0;

// Extract phase type from pick label
// Labels typically contain patterns like "-A-P-" or "-A-S-"
std::string extractPhaseFromLabel(const std::string& label) {
	// Look for phase indicator in label
	size_t pos = label.find("-A-P-");
	if (pos != std::string::npos) return "P";
	
	pos = label.find("-A-S-");
	if (pos != std::string::npos) return "S";
	
	// Check for other phase types
	if (label.find("PKP") != std::string::npos) return "PKP";
	if (label.find("SKS") != std::string::npos) return "SKS";
	if (label.find("Pg") != std::string::npos) return "Pg";
	if (label.find("Sg") != std::string::npos) return "Sg";
	if (label.find("Pn") != std::string::npos) return "Pn";
	if (label.find("Sn") != std::string::npos) return "Sn";
	
	return "P";  // Default to P
}
}


namespace PSLoc {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseCluster::PhaseCluster() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseCluster::~PhaseCluster() {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PhaseCluster::setConfig(const Config& cfg) {
	_config = cfg;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PhaseCluster::clear() {
	_points.clear();
	_membership.clear();
	_clusterId.clear();
	_clusters.clear();
	_pickToCluster.clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::vector<PhaseCluster::Cluster> PhaseCluster::cluster(const std::vector<Pick*>& picks) {
	clear();

	if ( picks.empty() || !_config.enabled ) {
		return _clusters;
	}

	// Initialize points
	_points = picks;
	size_t n = _points.size();
	_membership.assign(n, Unclassified);
	_clusterId.assign(n, -1);

	SEISCOMP_DEBUG("PhaseCluster: running DBSCAN on %zu picks "
	               "(epsTime=%.1f, epsDist=%.1f, minPts=%d)",
	               n, _config.epsTime, _config.epsDist, _config.minPts);

	// Run DBSCAN
	dbscan();

	// Build cluster objects
	std::map<int, Cluster> clusterMap;
	for ( size_t i = 0; i < n; ++i ) {
		int cid = _clusterId[i];
		if ( cid < 0 ) continue;  // Skip noise

		if ( clusterMap.find(cid) == clusterMap.end() ) {
			Cluster c;
			c.id = cid;
			clusterMap[cid] = c;
		}
		clusterMap[cid].picks.push_back(_points[i]);
	}

	// Compute statistics for each cluster
	for ( auto& pair : clusterMap ) {
		computeClusterStats(pair.second);
		_clusters.push_back(pair.second);
	}

	// Sort clusters by score (descending)
	std::sort(_clusters.begin(), _clusters.end(),
	          [this](const Cluster& a, const Cluster& b) {
		          return clusterScore(a) > clusterScore(b);
	          });

	// Build quick lookup
	for ( const auto& c : _clusters ) {
		for ( const Pick* p : c.picks ) {
			_pickToCluster[p] = c.id;
		}
	}

	SEISCOMP_INFO("PhaseCluster: found %zu clusters (%zu noise picks)",
	              _clusters.size(),
	              std::count(_clusterId.begin(), _clusterId.end(), -1));

	for ( size_t i = 0; i < _clusters.size(); ++i ) {
		const Cluster& c = _clusters[i];
		SEISCOMP_DEBUG("  Cluster %d: %zu picks (P=%zu, S=%zu), "
		               "score=%.1f, RMS=%.2f, azigap=%.0f",
		               c.id, c.picks.size(), c.pCount, c.sCount,
		               clusterScore(c), c.rmsResidual, c.azigap);
	}

	return _clusters;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::vector<PhaseCluster::Cluster>
PhaseCluster::clusterFromPool(const std::map<std::string, PickCPtr>& pool) {
	std::vector<Pick*> picks;
	picks.reserve(pool.size());
	for ( const auto& pair : pool ) {
		picks.push_back(const_cast<Pick*>(pair.second.get()));
	}
	return cluster(picks);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseCluster::Membership PhaseCluster::getMembership(const Pick* pick) const {
	for ( size_t i = 0; i < _points.size(); ++i ) {
		if ( _points[i] == pick ) {
			return _membership[i];
		}
	}
	return Unclassified;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int PhaseCluster::getClusterID(const Pick* pick) const {
	auto it = _pickToCluster.find(pick);
	if ( it != _pickToCluster.end() ) {
		return it->second;
	}
	return -1;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double PhaseCluster::clusterScore(const Cluster& c) const {
	if ( !c.isValid(_config.minPhases) ) {
		return 0.0;
	}

	double score = 0.0;

	// Phase count component (logarithmic scaling)
	size_t totalPhases = c.pCount + c.sCount;
	score += 2.0 * std::log10(std::max(1.0, (double)totalPhases));

	// P/S balance bonus (having both P and S is valuable)
	if ( c.pCount > 0 && c.sCount > 0 ) {
		double ratio = std::min((double)c.sCount / c.pCount,
		                        (double)c.pCount / c.sCount);
		score += 1.0 * ratio;  // Up to +1.0 for balanced P/S
	}

	// Residual quality (lower RMS is better)
	if ( c.rmsResidual > 0 ) {
		double rmsScore = std::max(0.0, 5.0 - c.rmsResidual);
		score += rmsScore;
	}
	else {
		score += 5.0;
	}

	// Azimuthal coverage bonus (lower gap is better)
	if ( c.azigap < 360 ) {
		double aziScore = std::max(0.0, 3.0 * (1.0 - c.azigap / 360.0));
		score += aziScore;
	}

	// Station distance spread bonus
	if ( c.maxStaDist > 0 ) {
		double distScore = std::min(2.0, c.maxStaDist / 50.0);
		score += distScore;
	}

	return score;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PhaseCluster::dbscan() {
	int clusterId = 0;

	for ( size_t i = 0; i < _points.size(); ++i ) {
		if ( _membership[i] != Unclassified ) {
			continue;
		}

		std::vector<size_t> neighbors = regionQuery(i);

		if ( (int)neighbors.size() < _config.minPts ) {
			// Mark as noise (may become border point later)
			_membership[i] = Noise;
			continue;
		}

		// Create new cluster
		_clusterId[i] = clusterId;
		_membership[i] = Core;

		expandCluster(clusterId, i, neighbors);
		clusterId++;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PhaseCluster::expandCluster(int clusterId, size_t pointIdx,
                                 std::vector<size_t>& neighbors) {
	std::queue<size_t> toProcess;
	for ( size_t n : neighbors ) {
		toProcess.push(n);
	}

	while ( !toProcess.empty() ) {
		size_t q = toProcess.front();
		toProcess.pop();

		if ( _membership[q] == Noise ) {
			// Promote noise to border point
			_membership[q] = Border;
			_clusterId[q] = clusterId;
			continue;
		}

		if ( _membership[q] != Unclassified ) {
			continue;
		}

		// Assign to cluster
		_membership[q] = Core;
		_clusterId[q] = clusterId;

		// Find neighbors of q
		std::vector<size_t> qNeighbors = regionQuery(q);

		if ( (int)qNeighbors.size() >= _config.minPts ) {
			for ( size_t n : qNeighbors ) {
				if ( _membership[n] == Unclassified || _membership[n] == Noise ) {
					toProcess.push(n);
				}
			}
		}
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::vector<size_t> PhaseCluster::regionQuery(size_t pointIdx) const {
	std::vector<size_t> neighbors;
	neighbors.reserve(_points.size());

	const Pick* p = _points[pointIdx];

	for ( size_t j = 0; j < _points.size(); ++j ) {
		if ( j == pointIdx ) continue;

		const Pick* q = _points[j];

		// Check phase compatibility first (fast filter)
		if ( _config.usePhaseType ) {
			std::string phaseA = extractPhaseFromLabel(p->label);
			std::string phaseB = extractPhaseFromLabel(q->label);
			if ( !phaseCompatible(phaseA, phaseB) ) {
				continue;
			}
		}

		// DBSCAN uses a combined epsilon threshold
		// Normalized distance: sqrt((timeDist/epsTime)^2 + (spatialDist/epsDist)^2) <= 1
		double timeDist = temporalDistance(p, q);
		double spatialDist = spatialDistance(p, q);

		double normTime = (_config.epsTime > 0) ? timeDist / _config.epsTime : 999.0;
		double normDist = (_config.epsDist > 0) ? spatialDist / _config.epsDist : 999.0;

		// Weighted Euclidean combination
		double combined = std::sqrt(
			_config.timeWeight * normTime * normTime +
			_config.distWeight * normDist * normDist
		);

		SEISCOMP_DEBUG("    %s vs %s: t=%.1fs (norm=%.3f), d=%.0fkm (norm=%.3f), combined=%.3f %s",
		               p->label.c_str(), q->label.c_str(),
		               timeDist, normTime, spatialDist, normDist, combined,
		               combined <= 1.0 ? "NEIGHBOR" : "too far");

		if ( combined <= 1.0 ) {
			neighbors.push_back(j);
		}
	}

	return neighbors;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double PhaseCluster::pickDistance(const Pick* a, const Pick* b) const {
	double timeDist = temporalDistance(a, b);
	double spatialDist = spatialDistance(a, b);

	double normTime = (_config.epsTime > 0) ? timeDist / _config.epsTime : 999.0;
	double normDist = (_config.epsDist > 0) ? spatialDist / _config.epsDist : 999.0;

	return std::sqrt(
		_config.timeWeight * normTime * normTime +
		_config.distWeight * normDist * normDist
	);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double PhaseCluster::spatialDistance(const Pick* a, const Pick* b) const {
	if ( !a->station() || !b->station() ) {
		return 999999.0;
	}

	return haversine(a->station()->lat, a->station()->lon,
	                 b->station()->lat, b->station()->lon);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double PhaseCluster::temporalDistance(const Pick* a, const Pick* b) const {
	// Simple temporal distance: absolute time difference
	// Picks from the same event should arrive within a reasonable time window
	// regardless of station distance (travel time differences are accounted for
	// by using a sufficiently large epsTime, typically 30-60 seconds)
	double dt = std::fabs(a->time - b->time);

	return dt;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PhaseCluster::phaseCompatible(const std::string& phaseA,
                                    const std::string& phaseB) const {
	std::string familyA = phaseFamily(phaseA);
	std::string familyB = phaseFamily(phaseB);

	// Same family is always compatible
	if ( familyA == familyB ) {
		return true;
	}

	// P and S families can cluster together (they come from the same event)
	if ( (familyA == "P" && familyB == "S") ||
	     (familyA == "S" && familyB == "P") ) {
		return true;
	}

	// Depth phases (pP, sP) are compatible with P
	if ( familyA == "P" && familyB == "depth" ) return true;
	if ( familyA == "depth" && familyB == "P" ) return true;

	// Core phases (PKP, SKS) are less compatible with regional phases
	if ( familyA == "core" || familyB == "core" ) {
		return familyA == familyB;
	}

	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string PhaseCluster::phaseFamily(const std::string& phase) const {
	if ( phase.empty() ) return "unknown";

	// P family
	if ( phase == "P" || phase == "Pg" || phase == "Pn" || phase == "Pb" ) {
		return "P";
	}

	// S family
	if ( phase == "S" || phase == "Sg" || phase == "Sn" || phase == "Sb" ) {
		return "S";
	}

	// Core phases
	if ( phase == "PKP" || phase == "PKIKP" || phase == "PKiKP" ||
	     phase == "SKS" || phase == "SKKS" ) {
		return "core";
	}

	// Depth phases
	if ( phase == "pP" || phase == "sP" || phase == "pwP" || phase == "sS" ) {
		return "depth";
	}

	// Reflections
	if ( phase == "PcP" || phase == "ScP" || phase == "ScS" ) {
		return "reflection";
	}

	// Multiples
	if ( phase == "PP" || phase == "SS" || phase == "PPP" ) {
		return "multiple";
	}

	return "unknown";
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PhaseCluster::computeClusterStats(Cluster& c) {
	if ( c.picks.empty() ) return;

	// Count P and S phases
	c.pCount = 0;
	c.sCount = 0;
	for ( const Pick* p : c.picks ) {
		std::string phase = extractPhaseFromLabel(p->label);
		std::string fam = phaseFamily(phase);
		if ( fam == "P" || fam == "depth" ) c.pCount++;
		else if ( fam == "S" ) c.sCount++;
	}

	// Centroid time
	double sumTime = 0;
	for ( const Pick* p : c.picks ) {
		sumTime += p->time;
	}
	c.centroidTime = sumTime / c.picks.size();

	// Centroid location (weighted by SNR if available)
	double sumLat = 0, sumLon = 0, sumWeight = 0;
	for ( const Pick* p : c.picks ) {
		if ( p->station() ) {
			double w = p->snr > 0 ? p->snr : 1.0;
			sumLat += p->station()->lat * w;
			sumLon += p->station()->lon * w;
			sumWeight += w;
		}
	}
	if ( sumWeight > 0 ) {
		c.centroidLat = sumLat / sumWeight;
		c.centroidLon = sumLon / sumWeight;
	}

	// Residuals (relative to centroid time)
	std::vector<double> residuals;
	residuals.reserve(c.picks.size());
	for ( const Pick* p : c.picks ) {
		// Approximate residual: pick time - centroid time
		// A proper residual would need travel time, but this gives coherence measure
		residuals.push_back(p->time - c.centroidTime);
	}

	// Mean and RMS of residuals
	double sumRes = 0;
	for ( double r : residuals ) sumRes += r;
	c.meanResidual = sumRes / residuals.size();

	double sumSqRes = 0;
	for ( double r : residuals ) {
		double diff = r - c.meanResidual;
		sumSqRes += diff * diff;
	}
	c.rmsResidual = std::sqrt(sumSqRes / residuals.size());

	// Maximum inter-station distance and azimuthal gap
	c.maxStaDist = 0;
	std::vector<double> azimuths;
	for ( size_t i = 0; i < c.picks.size(); ++i ) {
		if ( !c.picks[i]->station() ) continue;
		for ( size_t j = i + 1; j < c.picks.size(); ++j ) {
			if ( !c.picks[j]->station() ) continue;
			double d = haversine(c.picks[i]->station()->lat, c.picks[i]->station()->lon,
			                     c.picks[j]->station()->lat, c.picks[j]->station()->lon);
			if ( d > c.maxStaDist ) c.maxStaDist = d;

			// Azimuth from centroid to station (simplified computation)
			double dlat = c.picks[i]->station()->lat - c.centroidLat;
			double dlon = c.picks[i]->station()->lon - c.centroidLon;
			double azi = std::atan2(dlon, dlat) * 180.0 / M_PI;
			if (azi < 0) azi += 360.0;
			azimuths.push_back(azi);
		}
	}

	// Compute azimuthal gap
	if ( azimuths.size() >= 2 ) {
		std::sort(azimuths.begin(), azimuths.end());
		double maxGap = 0;
		for ( size_t i = 1; i < azimuths.size(); ++i ) {
			double gap = azimuths[i] - azimuths[i-1];
			if ( gap > maxGap ) maxGap = gap;
		}
		// Wrap-around gap
	 double wrapGap = 360.0 - azimuths.back() + azimuths.front();
		if ( wrapGap > maxGap ) maxGap = wrapGap;
		c.azigap = maxGap;
	}
	else {
		c.azigap = 360.0;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double PhaseCluster::haversine(double lat1, double lon1, double lat2, double lon2) {
	double dlat = (lat2 - lat1) * M_PI / 180.0;
	double dlon = (lon2 - lon1) * M_PI / 180.0;
	double a = std::sin(dlat / 2) * std::sin(dlat / 2) +
	           std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
	           std::sin(dlon / 2) * std::sin(dlon / 2);
	double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
	return EARTH_RADIUS_KM * c;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
double PhaseCluster::greatCircleDistance(double lat1, double lon1,
                                          double lat2, double lon2) {
	return haversine(lat1, lon1, lat2, lon2);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




} // namespace PSLoc
