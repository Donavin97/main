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


#define SEISCOMP_COMPONENT Multipick
#include <seiscomp/logging/log.h>

#include "phaseclassifier.h"

#include <cmath>
#include <algorithm>
#include <numeric>


namespace Seiscomp {
namespace Applications {
namespace MultiPick {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseClassifier::PhaseClassifier()
: _method("auto")
, _minConfidence(0.5)
, _usePolarization(false)
, _useSpectralAnalysis(false)
, _useML(false)
, _mlModel(nullptr)
{
	// Enable all phase types by default
	_enabledTypes[PhaseType::P_FAMILY] = true;
	_enabledTypes[PhaseType::S_FAMILY] = true;
	_enabledTypes[PhaseType::PKP_FAMILY] = false;
	_enabledTypes[PhaseType::SKS_FAMILY] = false;
	_enabledTypes[PhaseType::DEPTH_PHASES] = false;
	_enabledTypes[PhaseType::REFLECTIONS] = false;
	_enabledTypes[PhaseType::MULTIPLES] = false;
	_enabledTypes[PhaseType::UNKNOWN] = true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseClassifier::~PhaseClassifier() {
	// Clean up ML model if allocated
	if ( _mlModel ) {
		// TODO: Free ML model resources
		_mlModel = nullptr;
	}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PhaseClassifier::init() {
	SEISCOMP_INFO("PhaseClassifier initialized with method: %s", _method.c_str());
	return true;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void PhaseClassifier::enablePhaseType(PhaseType type, bool enable) {
	_enabledTypes[type] = enable;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool PhaseClassifier::isPhaseTypeEnabled(PhaseType type) const {
	auto it = _enabledTypes.find(type);
	if ( it != _enabledTypes.end() ) {
		return it->second;
	}
	return false;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseResult PhaseClassifier::classify(
	const std::vector<double>& waveform,
	double sampleRate,
	double pickTime,
	const std::string& component,
	double backAzimuth,
	double distance
) {
	PhaseResult result;
	result.onsetTime = pickTime;
	result.snr = 0.0;
	result.amplitude = 0.0;
	result.period = 0.0;
	result.confidence = 0.0;
	result.method = _method;

	// Extract features from waveform
	auto features = extractFeatures(waveform, sampleRate);
	result.snr = features.count("snr") ? features["snr"] : 0.0;
	result.amplitude = features.count("amplitude") ? features["amplitude"] : 0.0;
	result.period = features.count("period") ? features["period"] : 0.0;

	// Classify based on configured method
	if ( _method == "component" ) {
		// Simple component-based classification
		// BHZ -> P family, BHN/BHE -> S family
		if ( component.size() >= 3 ) {
			char compChar = component.back();
			if ( compChar == 'Z' || compChar == 'z' ) {
				result.phase = "P";
				result.confidence = 0.8;
			}
			else if ( compChar == 'N' || compChar == 'n' ||
			          compChar == 'E' || compChar == 'e' ) {
				result.phase = "S";
				result.confidence = 0.8;
			}
			else {
				result.phase = "P";
				result.confidence = 0.5;
			}
		}
		else {
			result.phase = "P";
			result.confidence = 0.5;
		}
	}
	else if ( _method == "polarization" && _usePolarization ) {
		result = classifyByPolarization(waveform, sampleRate, component);
	}
	else if ( _method == "ml" && _useML ) {
		result = classifyByML(waveform, sampleRate, pickTime, backAzimuth, distance);
	}
	else {
		// Default: auto mode using heuristics
		result = classifyByHeuristics(waveform, sampleRate, component, backAzimuth, distance);
	}

	// Apply phase name refinement based on distance if available
	if ( distance > 0 && result.confidence >= _minConfidence ) {
		PhaseType family = identifyPhaseFamily(result.snr, result.period, component);
		result.phase = determinePhaseName(family, component, distance);
	}

	return result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseResult PhaseClassifier::classifyByPolarization(
	const std::vector<double>& waveform,
	double sampleRate,
	const std::string& component
) {
	PhaseResult result;
	result.method = "polarization";

	// Extract features
	auto features = extractFeatures(waveform, sampleRate);
	result.snr = features.count("snr") ? features["snr"] : 0.0;
	result.amplitude = features.count("amplitude") ? features["amplitude"] : 0.0;
	result.period = features.count("period") ? features["period"] : 0.0;

	// Simple polarization-based classification
	// P-waves: predominantly vertical motion
	// S-waves: predominantly horizontal motion
	if ( component.size() >= 3 ) {
		char compChar = component.back();
		if ( compChar == 'Z' || compChar == 'z' ) {
			result.phase = "P";
			result.confidence = 0.85;
		}
		else {
			result.phase = "S";
			result.confidence = 0.85;
		}
	}
	else {
		result.phase = "P";
		result.confidence = 0.6;
	}

	return result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseResult PhaseClassifier::classifyBySpectralAnalysis(
	const std::vector<double>& waveform,
	double sampleRate
) {
	PhaseResult result;
	result.method = "spectral";

	// Extract features including spectral content
	auto features = extractFeatures(waveform, sampleRate);
	result.snr = features.count("snr") ? features["snr"] : 0.0;
	result.amplitude = features.count("amplitude") ? features["amplitude"] : 0.0;
	result.period = features.count("period") ? features["period"] : 0.0;

	// P-waves typically have higher frequency content than S-waves
	// This is a simplified approach - real implementation would use FFT
	if ( result.period > 0 && result.period < 0.5 ) {
		result.phase = "P";
		result.confidence = 0.7;
	}
	else if ( result.period > 0.5 ) {
		result.phase = "S";
		result.confidence = 0.65;
	}
	else {
		result.phase = "P";
		result.confidence = 0.5;
	}

	return result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseResult PhaseClassifier::classifyByML(
	const std::vector<double>& waveform,
	double sampleRate,
	double pickTime,
	double backAzimuth,
	double distance
) {
	PhaseResult result;
	result.method = "ml";

	// Placeholder for ML-based classification
	// In a real implementation, this would:
	// 1. Extract feature vector from waveform
	// 2. Pass through trained neural network or other ML model
	// 3. Return predicted phase with confidence

	auto features = extractFeatures(waveform, sampleRate);
	result.snr = features.count("snr") ? features["snr"] : 0.0;
	result.amplitude = features.count("amplitude") ? features["amplitude"] : 0.0;
	result.period = features.count("period") ? features["period"] : 0.0;

	// Fallback to heuristics since ML model is not implemented
	SEISCOMP_WARNING("ML classification not available, falling back to heuristics");
	return classifyByHeuristics(waveform, sampleRate, "", backAzimuth, distance);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseResult PhaseClassifier::classifyByHeuristics(
	const std::vector<double>& waveform,
	double sampleRate,
	const std::string& component,
	double backAzimuth,
	double distance
) {
	PhaseResult result;
	result.method = "heuristic";

	// Extract features
	auto features = extractFeatures(waveform, sampleRate);
	result.snr = features.count("snr") ? features["snr"] : 0.0;
	result.amplitude = features.count("amplitude") ? features["amplitude"] : 0.0;
	result.period = features.count("period") ? features["period"] : 0.0;

	// Phase identification based on component
	if ( component.size() >= 3 ) {
		char compChar = component.back();

		if ( compChar == 'Z' || compChar == 'z' ) {
			// Vertical component - likely P-phase (first arriving)
			result.phase = "P";
			result.confidence = 0.8;

			// Refine based on distance if available
			if ( distance > 0 ) {
				if ( distance < 20.0 && _enabledTypes[PhaseType::P_FAMILY] ) {
					result.phase = "Pg";
					result.confidence = 0.75;
				}
				else if ( distance > 30.0 && _enabledTypes[PhaseType::P_FAMILY] ) {
					result.phase = "Pn";
					result.confidence = 0.7;
				}
			}
		}
		else if ( compChar == 'N' || compChar == 'n' ||
		          compChar == 'E' || compChar == 'e' ) {
			// Horizontal component - likely S-phase (first arriving)
			result.phase = "S";
			result.confidence = 0.75;

			// Refine based on distance if available
			if ( distance > 0 ) {
				if ( distance < 20.0 && _enabledTypes[PhaseType::S_FAMILY] ) {
					result.phase = "Sg";
					result.confidence = 0.7;
				}
				else if ( distance > 30.0 && _enabledTypes[PhaseType::S_FAMILY] ) {
					result.phase = "Sn";
					result.confidence = 0.65;
				}
			}
		}
		else {
			// Unknown component - default to P (first arriving)
			result.phase = "P";
			result.confidence = 0.5;
		}
	}
	else {
		// No component info - default to P (first arriving)
		result.phase = "P";
		result.confidence = 0.5;
	}

	// Check if phase type is enabled
	PhaseType family = identifyPhaseFamily(result.snr, result.period, component);
	if ( !isPhaseTypeEnabled(family) ) {
		// Fall back to most common enabled phase
		if ( isPhaseTypeEnabled(PhaseType::P_FAMILY) ) {
			result.phase = "P";
		}
		else if ( isPhaseTypeEnabled(PhaseType::S_FAMILY) ) {
			result.phase = "S";
		}
		result.confidence *= 0.5;
	}

	return result;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::map<std::string, double> PhaseClassifier::extractFeatures(
	const std::vector<double>& waveform,
	double sampleRate
) {
	std::map<std::string, double> features;

	if ( waveform.empty() || sampleRate <= 0 ) {
		return features;
	}

	size_t n = waveform.size();

	// Calculate RMS amplitude
	double rms = 0.0;
	for ( size_t i = 0; i < n; ++i ) {
		rms += waveform[i] * waveform[i];
	}
	rms = std::sqrt(rms / n);
	features["amplitude"] = rms;

	// Calculate peak amplitude
	double peak = 0.0;
	for ( size_t i = 0; i < n; ++i ) {
		double amp = std::fabs(waveform[i]);
		if ( amp > peak ) {
			peak = amp;
		}
	}
	features["peak"] = peak;

	// Estimate dominant period using zero-crossing rate
	size_t zeroCrossings = 0;
	for ( size_t i = 1; i < n; ++i ) {
		if ( (waveform[i-1] >= 0 && waveform[i] < 0) ||
		     (waveform[i-1] < 0 && waveform[i] >= 0) ) {
			++zeroCrossings;
		}
	}

	if ( zeroCrossings > 0 ) {
		double duration = static_cast<double>(n) / sampleRate;
		features["period"] = (2.0 * duration) / static_cast<double>(zeroCrossings);
	}
	else {
		features["period"] = 0.0;
	}

	// Estimate SNR using first/last half ratio (simplified)
	double noiseRMS = 0.0, signalRMS = 0.0;
	size_t halfN = n / 2;

	for ( size_t i = 0; i < halfN; ++i ) {
		noiseRMS += waveform[i] * waveform[i];
	}
	noiseRMS = std::sqrt(noiseRMS / halfN);

	for ( size_t i = halfN; i < n; ++i ) {
		signalRMS += waveform[i] * waveform[i];
	}
	signalRMS = std::sqrt(signalRMS / (n - halfN));

	if ( noiseRMS > 0 ) {
		features["snr"] = signalRMS / noiseRMS;
	}
	else {
		features["snr"] = 0.0;
	}

	// Calculate mean and std for additional features
	double mean = 0.0;
	for ( double v : waveform ) {
		mean += v;
	}
	mean /= static_cast<double>(n);

	double variance = 0.0;
	for ( double v : waveform ) {
		double diff = v - mean;
		variance += diff * diff;
	}
	variance /= static_cast<double>(n);

	features["mean"] = mean;
	features["std"] = std::sqrt(variance);

	return features;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
PhaseType PhaseClassifier::identifyPhaseFamily(double snr, double period, const std::string& component) {
	// Simple heuristic for phase family identification
	if ( component.size() >= 3 ) {
		char compChar = component.back();
		if ( compChar == 'Z' || compChar == 'z' ) {
			return PhaseType::P_FAMILY;
		}
		else if ( compChar == 'N' || compChar == 'n' ||
		          compChar == 'E' || compChar == 'e' ) {
			return PhaseType::S_FAMILY;
		}
	}

	// Use period as additional indicator
	// P-waves typically have shorter periods (higher frequency)
	if ( period > 0 && period < 0.5 ) {
		return PhaseType::P_FAMILY;
	}
	else if ( period > 0.5 ) {
		return PhaseType::S_FAMILY;
	}

	return PhaseType::UNKNOWN;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
std::string PhaseClassifier::determinePhaseName(PhaseType family, const std::string& component, double distance) {
	std::string phase;

	switch ( family ) {
		case PhaseType::P_FAMILY:
			if ( distance > 0 ) {
				if ( distance < 20.0 ) {
					phase = "Pg";  // Regional P
				}
				else if ( distance > 30.0 ) {
					phase = "Pn";  // Teleseismic P
				}
				else {
					phase = "P";   // Intermediate
				}
			}
			else {
				phase = "P";
			}
			break;

		case PhaseType::S_FAMILY:
			if ( distance > 0 ) {
				if ( distance < 20.0 ) {
					phase = "Sg";  // Regional S
				}
				else if ( distance > 30.0 ) {
					phase = "Sn";  // Teleseismic S
				}
				else {
					phase = "S";   // Intermediate
				}
			}
			else {
				phase = "S";
			}
			break;

		case PhaseType::PKP_FAMILY:
			phase = "PKP";
			break;

		case PhaseType::SKS_FAMILY:
			phase = "SKS";
			break;

		case PhaseType::DEPTH_PHASES:
			// Determine based on component
			if ( component.size() >= 3 ) {
				char compChar = component.back();
				if ( compChar == 'Z' || compChar == 'z' ) {
					phase = "pP";
				}
				else {
					phase = "sP";
				}
			}
			else {
				phase = "pP";
			}
			break;

		case PhaseType::REFLECTIONS:
			phase = "PcP";
			break;

		case PhaseType::MULTIPLES:
			phase = "PP";
			break;

		default:
			phase = "P";
			break;
	}

	return phase;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
}
