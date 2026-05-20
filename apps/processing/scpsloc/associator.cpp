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




#define SEISCOMP_COMPONENT Autoloc
#include <seiscomp/logging/log.h>

#include <algorithm>
#include <cmath>
#include <seiscomp/seismology/ttt.h>

#include "util.h"
#include "sc3adapters.h"
#include "associator.h"

using namespace std;

namespace PSLoc {

#define AFFMIN 0.1

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Associator::Associator()
{
	_origins = nullptr;
	_stations = nullptr;
	_minScoreSPhase = 5.0;
	_minScoreOtherSPhases = 6.0;

	// The order of the phases is essential!
	
	// P-phase family (teleseismic & regional)
	_phases.push_back( Phase("P",      0, 180) );
	_phases.push_back( Phase("Pg",     0,  15) );  // Regional P
	_phases.push_back( Phase("Pn",     0,  18) );  // Refracted P
	_phases.push_back( Phase("Pb",     0,  12) );  // Conrad interface P
	_phases.push_back( Phase("PcP",   25,  55) );  // Core reflection
	_phases.push_back( Phase("PKP", 140, 180) );  // Core phase
	_phases.push_back( Phase("PKiKP", 30, 120) ); // Inner core reflection
	_phases.push_back( Phase("PKKP",  80, 130) ); // Outer core multiple

	// S-phase family (teleseismic & regional) - NEW for advanced P/S locator
	_phases.push_back( Phase("S",      0, 180) );
	_phases.push_back( Phase("Sg",     0,  15) );  // Regional S
	_phases.push_back( Phase("Sn",     0,  18) );  // Refracted S
	_phases.push_back( Phase("Sb",     0,  12) );  // Conrad interface S
	_phases.push_back( Phase("ScS",   25,  85) );  // Core reflection
	_phases.push_back( Phase("SKS",   80, 150) );  // Core phase
	_phases.push_back( Phase("SKKP", 110, 152) ); // Outer core multiple

	// Depth phases - NEW for improved depth resolution
	_phases.push_back( Phase("pP",    30,  90) );  // Depth phase (P up then P down)
	_phases.push_back( Phase("sP",    30,  90) );  // Depth phase (S up then P down)
	_phases.push_back( Phase("sS",    30,  90) );  // Depth phase (S up then S down)
	_phases.push_back( Phase("pwP",   30,  80) );  // Water surface multiple

	// Other phases
	_phases.push_back( Phase("PP",    60, 160) );  // Surface multiple
	_phases.push_back( Phase("ScP",   25,  55) );  // S to P conversion
	_phases.push_back( Phase("SKP",  120, 150) );  // Core conversion

	// TODO: make the phase set configurable
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Associator::setStations(const StationMap *stations)
{
	_stations = stations;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Associator::setOrigins(const OriginVector *origins)
{
	_origins = origins;
}

void
Associator::setMinScoreSPhase(double score)
{
	_minScoreSPhase = score;
}

void
Associator::setMinScoreOtherSPhases(double score)
{
	_minScoreOtherSPhases = score;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Associator::reset()
{
	_associations.clear();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void
Associator::shutdown()
{
	reset();
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool
Associator::feed(const Pick* pick)
{
	_associations.clear();

	if ( ! _origins)
		return false;

	static Seiscomp::TravelTimeTable ttt;

	int count = 0;

	for (const OriginPtr& _origin : *_origins) {

		const Origin  *origin = _origin.get();
		const Station *station = pick->station();

		const Hypocenter& hypo{origin->hypocenter};
		double delta, az, baz;
		delazi(&hypo, station, delta, az, baz);

		Seiscomp::TravelTimeList *ttlist {nullptr};

		try {
			ttlist = ttt.compute(hypo.lat, hypo.lon, std::max(hypo.dep, 0.01),
			                     station->lat, station->lon, 0);
		}
		catch ( std::out_of_range & ) {
			continue;
		}
		if ( ! ttlist)
			continue;

		// An imported origin is treated as if it had a very high
		// score. => Anything can be associated with it.
		// Preliminary origins (single-station cluster origins) are also treated
		// as having high score to encourage pick accumulation (scanloc approach)
		double origin_score = origin->imported ? 1000 : (origin->preliminary ? 100 : origin->score);

		// If the pick has a phase hint from the picker (e.g., Pg, Sg, Pn, Sn),
		// try that phase first before falling back to the generic phase order.
		// This respects the picker's phase identification while still allowing
		// fallback if the hinted phase doesn't match.
		bool associated = false;
		std::string pickPhase = pick->phase;

		// Helper lambda to try associating a specific phase
		auto tryPhase = [&](const std::string& phaseCode) -> bool {
			// Find the phase definition
			const Phase* phaseDef = nullptr;
			for (const Phase& ph : _phases) {
				if (ph.code == phaseCode) {
					phaseDef = &ph;
					break;
				}
			}
			if (!phaseDef) return false;

			// Check minimum score requirement
			double min_score = 20;
			if (phaseDef->code == "P") min_score = 20;
			else if (phaseDef->code == "S") min_score = _minScoreSPhase;
			else if (phaseDef->code.substr(0,1) == "S") min_score = _minScoreOtherSPhases;
			else min_score = 50;

			if (origin_score < min_score) {
				return false;
			}

			double delta_check = delta;
			if (delta_check < phaseDef->dmin || delta_check > phaseDef->dmax)
				return false;

			double ttime = -1, x = 1;

			// Handle P family
			if (phaseDef->code == "P" || phaseDef->code == "Pg" || phaseDef->code == "Pn" ||
			    phaseDef->code == "Pb" || phaseDef->code == "PcP") {
				for (const auto& tt: *ttlist) {
					if (phaseDef->code == "P") {
						if (delta_check < 114) {
							ttime = tt.time;
							break;
						}
						if (tt.phase.substr(0,2) != "PK")
							continue;
						ttime = tt.time;
						break;
					}
					else {
						if (tt.phase == phaseDef->code || tt.phase.substr(0, phaseDef->code.size()) == phaseDef->code) {
							ttime = tt.time;
							break;
						}
					}
				}
				x = 1 + 0.6*exp(-0.003*delta_check*delta_check) + 0.5*exp(-0.03*(15-delta_check)*(15-delta_check));
			}
			// Handle S family
			else if (phaseDef->code == "S" || phaseDef->code == "Sg" || phaseDef->code == "Sn" ||
			         phaseDef->code == "Sb" || phaseDef->code == "ScS") {
				for (const auto& tt: *ttlist) {
					if (phaseDef->code == "S") {
						if (delta_check < 114) {
							// Accept teleseismic S and regional S phases (Sg, Sn, Sb)
							if (tt.phase == "S" || tt.phase == "S1" ||
							    tt.phase == "Sg" || tt.phase == "Sn" || tt.phase == "Sb") {
								ttime = tt.time;
								break;
							}
						}
						else {
							if (tt.phase.substr(0,3) == "SKS") {
								ttime = tt.time;
								break;
							}
						}
					}
					else {
						if (tt.phase == phaseDef->code || tt.phase.substr(0, phaseDef->code.size()) == phaseDef->code) {
							ttime = tt.time;
							break;
						}
					}
				}
				// S phases have larger picking uncertainty
				x = 1 + 1.2*exp(-0.003*delta_check*delta_check) + 0.8*exp(-0.03*(15-delta_check)*(15-delta_check));
			}
			// Handle other phases (depth, core, multiples, etc.)
			else {
				for (const auto& tt: *ttlist) {
					if (tt.phase == phaseDef->code || tt.phase.substr(0, phaseDef->code.size()) == phaseDef->code) {
						ttime = tt.time;
						break;
					}
				}
				x = 1.0;
			}

			if (ttime == -1) return false;

			// Compute affinity
			double affinity = 0;
			double residual = double(pick->time - (origin->time + ttime));
			if ( origin->imported ) {
				if (residual < -20 || residual > 30)
					return false;
				affinity = 1;
			}
			else {
				residual = residual/x;
				residual /= 10;
				affinity = avgfn(residual);
				if (affinity < AFFMIN)
					return false;
			}

			string phcode = phaseDef->code;
			if (phcode=="P" && ttime > 960) phcode = "PKP";
			if (phcode=="S" && ttime > 960) phcode = "SKS";

			Association asso(origin, pick, phcode, residual, affinity);
			asso.distance = delta;
			asso.azimuth = az;
			_associations.push_back(asso);
			count++;
			return true;
		};

		// Step 1: Try the pick's hinted phase first (if specific and not generic P/S)
		if (!pickPhase.empty() && pickPhase != "P" && pickPhase != "S") {
			if (tryPhase(pickPhase)) {
				associated = true;
			}
		}

		// Step 2: If pick phase hint didn't match, try generic family
		if (!associated && !pickPhase.empty()) {
			if (pickPhase == "P" || pickPhase == "Pg" || pickPhase == "Pn" || pickPhase == "Pb") {
				associated = tryPhase("P");
			}
			else if (pickPhase == "S" || pickPhase == "Sg" || pickPhase == "Sn" || pickPhase == "Sb") {
				associated = tryPhase("S");
			}
		}

		// Step 3: If no phase hint or it didn't match, fall back to normal phase iteration
		if (!associated) {
			for (const Phase &phase : _phases) {

			// Lower threshold for S phases to encourage S-phase association
			double min_score = 20;
			if (phase.code == "P") min_score = 20;
			else if (phase.code == "S") min_score = _minScoreSPhase;
			else if (phase.code.substr(0,1) == "S") min_score = _minScoreOtherSPhases;
			else min_score = 50;

				if (origin_score < min_score) {
					continue;
				}

				if (delta < phase.dmin || delta > phase.dmax)
					continue;

			double ttime = -1, x = 1;

			// Handle P family (P, Pg, Pn, Pb, PcP)
			if (phase.code == "P" || phase.code == "Pg" || phase.code == "Pn" || 
			    phase.code == "Pb" || phase.code == "PcP") {
				for (const auto& tt: *ttlist) {
					if (phase.code == "P") {
						if (delta < 114) {
							// for delta < 114, always take 1st arrival
							ttime = tt.time;
							break;
						}
						if (tt.phase.substr(0,2) != "PK")
							continue;
						ttime = tt.time;
						break;
					}
					else {
						// For specific phases (Pg, Pn, Pb, PcP)
						if (tt.phase == phase.code || tt.phase.substr(0, phase.code.size()) == phase.code) {
							ttime = tt.time;
							break;
						}
					}
				}
				// Weight residuals at regional distances
				x = 1 + 0.6*exp(-0.003*delta*delta) + 0.5*exp(-0.03*(15-delta)*(15-delta));
			}
			// Handle S family (S, Sg, Sn, Sb, ScS)
			else if (phase.code == "S" || phase.code == "Sg" || phase.code == "Sn" || 
			         phase.code == "Sb" || phase.code == "ScS") {
				SEISCOMP_DEBUG("Associator: Looking for %s phase travel time, delta=%.1f", phase.code.c_str(), delta);
				for (const auto& tt: *ttlist) {
					if (phase.code == "S") {
						if (delta < 114) {
							if (tt.phase == "S" || tt.phase == "S1") {
								ttime = tt.time;
								SEISCOMP_DEBUG("Associator: Found S phase tt=%.2f", ttime);
								break;
							}
						}
						else {
							if (tt.phase.substr(0,3) == "SKS") {
								ttime = tt.time;
								SEISCOMP_DEBUG("Associator: Found SKS phase tt=%.2f", ttime);
								break;
							}
						}
					}
					else {
						// For specific phases (Sg, Sn, Sb, ScS)
						if (tt.phase == phase.code || tt.phase.substr(0, phase.code.size()) == phase.code) {
							ttime = tt.time;
							SEISCOMP_DEBUG("Associator: Found %s phase tt=%.2f", phase.code.c_str(), ttime);
							break;
						}
					}
				}
				if (ttime == -1) {
					SEISCOMP_DEBUG("Associator: %s phase travel time NOT found for delta=%.1f", phase.code.c_str(), delta);
				}
				// S phases have larger uncertainty
				x = 1.5 + 0.8*exp(-0.002*delta*delta);
			}
			// Handle PKP family
			else if (phase.code.substr(0,3) == "PKP" || phase.code.substr(0,4) == "PKiK" || phase.code.substr(0,4) == "PKKP") {
				for (const auto& tt: *ttlist) {
					if (tt.phase.substr(0, phase.code.size()) == phase.code) {
						ttime = tt.time;
						break;
					}
				}
				x = 1.2;  // Core phases get moderate tolerance
			}
			// Handle SKS family
			else if (phase.code.substr(0,3) == "SKS" || phase.code.substr(0,4) == "SKKS") {
				for (const auto& tt: *ttlist) {
					if (tt.phase.substr(0, phase.code.size()) == phase.code) {
						ttime = tt.time;
						break;
					}
				}
				x = 1.5;  // S-core phases get more tolerance
			}
			// Handle depth phases (pP, sP, sS, pwP)
			else if (phase.code == "pP" || phase.code == "sP" || phase.code == "sS" || phase.code == "pwP") {
				for (const auto& tt: *ttlist) {
					if (tt.phase == phase.code) {
						ttime = tt.time;
						break;
					}
				}
				x = 0.8;  // Depth phases need tighter match
			}
			// Handle other phases (PP, ScP, SKP, etc.)
			else {
				for (const auto& tt: *ttlist) {
					if (tt.phase.substr(0, phase.code.size()) == phase.code) {
						ttime = tt.time;
						break;
					}
				}
				x = 1.0;
			}

			if (ttime == -1) // phase not found
				continue;

			// compute "affinity" based on distance and residual
			double affinity = 0;
			double residual = double(pick->time - (origin->time + ttime));
			if ( origin->imported ) {
				// If the pick is within the interval,
				// associate it with affinity 1, otherwise skip
				//
				// TODO: Make this configurable
				if (residual < -20 || residual > 30)
					continue;
				affinity = 1;
			}
			else {
				residual = residual/x;
				residual /= 10;       // normalize residual

				affinity = avgfn(residual); // test if exp(-residual**2) if better
				if (affinity < AFFMIN)
					continue;
			}
			string phcode = phase.code;
			// Rename P -> PKP for teleseismic distances
			if (phcode=="P" && ttime > 960)
				phcode = "PKP";
			// NEW: Rename S -> SKS for teleseismic distances
			if (phcode=="S" && ttime > 960)
				phcode = "SKS";
			Association asso(origin, pick, phcode, residual, affinity);
			asso.distance = delta;
			asso.azimuth = az;
			_associations.push_back(asso);
			count++;

			// ensure no more than one association per origin
			break;
		}
		}  // end if (!associated)

		delete ttlist;
	}

	return (_associations.size() > 0);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
const AssociationVector &
Associator::associations() const
{
	return _associations;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Association*
Associator::associate(Origin *origin, const Pick *pick, const string &phase)
{
	return nullptr;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
Associator::Phase::Phase(const string &code, double dmin, double dmax)
	: code(code), dmin(dmin), dmax(dmax) {}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


}  // namespace PSLoc
