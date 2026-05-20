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

#ifndef PSLoc_PHASECLUSTER_H
#define PSLoc_PHASECLUSTER_H

#include "datamodel.h"

#include <vector>
#include <set>
#include <map>
#include <string>


namespace PSLoc {


/**
 * DBSCAN-based phase clustering for seismic picks.
 *
 * This implements density-based spatial clustering similar to GEMPA's scanloc
 * module. Picks are clustered based on:
 * - Spatial proximity (station distances)
 * - Temporal coherence (residual consistency)
 * - Phase type compatibility
 *
 * The clustering helps to:
 * - Identify groups of picks belonging to the same event
 * - Filter out noise/outlier picks
 * - Improve location quality by selecting coherent pick sets
 * - Handle multiple simultaneous events
 */
class PhaseCluster {
	public:
		/// Cluster membership status for a pick
		enum Membership {
			Unclassified = 0,   // Not yet processed
			Core,               // Core point of a cluster
			Border,             // Border point of a cluster
			Noise               // Noise point (outlier)
		};

		/// A single cluster result
		struct Cluster {
			int                 id;              // Cluster ID (-1 for noise)
			std::vector<Pick*>  picks;           // Member picks
			double              centroidTime;    // Mean origin time of cluster
			double              centroidLat;     // Mean latitude (from stations)
			double              centroidLon;     // Mean longitude (from stations)
			double              meanResidual;    // Mean residual within cluster
			double              rmsResidual;     // RMS of residuals
			size_t              pCount;          // Number of P phases
			size_t              sCount;          // Number of S phases
			double              maxStaDist;      // Maximum inter-station distance
			double              azigap;          // Azimuthal gap of stations

			Cluster() : id(-1), centroidTime(0), centroidLat(0), centroidLon(0),
			            meanResidual(0), rmsResidual(0), pCount(0), sCount(0),
			            maxStaDist(0), azigap(360) {}

			/// Check if cluster has enough picks to be considered valid
			bool isValid(size_t minPhases = 4) const {
				return picks.size() >= minPhases && (pCount + sCount) >= minPhases;
			}
		};

		/// Configuration for clustering
		struct Config {
			double epsTime{5.0};         // Time epsilon for clustering (seconds)
			double epsDist{100.0};       // Distance epsilon for clustering (km)
			int    minPts{4};            // Minimum picks to form a cluster
			double timeWeight{1.0};      // Weight for time dimension in distance
			double distWeight{1.0};      // Weight for spatial dimension
			bool   usePhaseType{true};   // Require compatible phase types
			bool   useTravelTime{true};  // Use travel-time corrected time
			double maxClusterTime{60.0}; // Max time span within a cluster (s)
			size_t minPhases{4};         // Minimum phases for valid cluster
			bool   enabled{false};       // Enable clustering
		};

	public:
		PhaseCluster();
		~PhaseCluster();

		/// Configure the clusterer
		void setConfig(const Config& cfg);

		/// Run DBSCAN clustering on a set of picks
		/// Returns clusters sorted by quality (score descending)
		std::vector<Cluster> cluster(const std::vector<Pick*>& picks);

		/// Run DBSCAN on picks from the pick pool
		std::vector<Cluster> clusterFromPool(const std::map<std::string, PickCPtr>& pool);

		/// Get cluster membership for a pick (after clustering)
		Membership getMembership(const Pick* pick) const;

		/// Get cluster ID for a pick (after clustering)
		int getClusterID(const Pick* pick) const;

		/// Clear all clustering results
		void clear();

		/// Compute a quality score for a cluster
		double clusterScore(const Cluster& c) const;

	private:
		/// DBSCAN core algorithm
		void dbscan();

		/// Expand cluster from a core point
		void expandCluster(int clusterId, size_t pointIdx, std::vector<size_t>& neighbors);

		/// Find neighbors within epsilon distance
		std::vector<size_t> regionQuery(size_t pointIdx) const;

		/// Compute combined distance between two picks
		double pickDistance(const Pick* a, const Pick* b) const;

		/// Compute spatial distance between two pick stations (km)
		double spatialDistance(const Pick* a, const Pick* b) const;

		/// Compute temporal distance (residual-based) between two picks
		double temporalDistance(const Pick* a, const Pick* b) const;

		/// Check if two phase types are compatible for clustering
		bool phaseCompatible(const std::string& phaseA, const std::string& phaseB) const;

		/// Get family of a phase (P, S, depth, etc.)
		std::string phaseFamily(const std::string& phase) const;

		/// Compute cluster centroid and statistics
		void computeClusterStats(Cluster& c);

		/// Haversine distance between two lat/lon points (km)
		static double haversine(double lat1, double lon1, double lat2, double lon2);

		/// Great circle distance in degrees
		static double greatCircleDistance(double lat1, double lon1, double lat2, double lon2);

	private:
		Config                          _config;
		std::vector<Pick*>              _points;           // All picks being clustered
		std::vector<Membership>         _membership;       // Membership status per point
		std::vector<int>                _clusterId;        // Cluster assignment per point
		std::vector<Cluster>            _clusters;         // Result clusters
		std::map<const Pick*, int>      _pickToCluster;    // Quick lookup
	};


} // namespace PSLoc

#endif // PSLoc_PHASECLUSTER_H
