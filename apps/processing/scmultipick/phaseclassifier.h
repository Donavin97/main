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


#ifndef SEISCOMP_APPLICATIONS_MULTIPICK_PHASECLASSIFIER
#define SEISCOMP_APPLICATIONS_MULTIPICK_PHASECLASSIFIER


#include <string>
#include <vector>
#include <map>


namespace Seiscomp {
namespace Applications {
namespace MultiPick {


// Phase classification result
struct PhaseResult {
	std::string phase;           // Classified phase name (P, S, Pg, Sg, etc.)
	double confidence;           // Confidence score (0.0 - 1.0)
	double onsetTime;            // Picked onset time
	double snr;                  // Signal-to-noise ratio
	double amplitude;            // Peak amplitude
	double period;               // Dominant period
	std::string method;          // Classification method used
};


// Phase type enumeration
enum class PhaseType {
	P_FAMILY,       // P, Pg, Pn, Pb
	S_FAMILY,       // S, Sg, Sn, Sb
	PKP_FAMILY,     // PKP, PKPdf, PKPab, PKiKP
	SKS_FAMILY,     // SKS, SKKS
	DEPTH_PHASES,   // pP, sP, sS, pwP
	REFLECTIONS,    // PcP, ScP, ScS
	MULTIPLES,      // PP, SS
	UNKNOWN
};


// Phase classifier using multiple methods
class PhaseClassifier {
	public:
		PhaseClassifier();
		~PhaseClassifier();

		// Initialize classifier with configuration
		bool init();

		// Classify a phase based on waveform characteristics
		PhaseResult classify(
			const std::vector<double>& waveform,
			double sampleRate,
			double pickTime,
			const std::string& component,
			double backAzimuth = -1.0,
			double distance = -1.0
		);

		// Set classifier method
		void setMethod(const std::string& method) { _method = method; }

		// Enable/disable specific phase types
		void enablePhaseType(PhaseType type, bool enable);
		bool isPhaseTypeEnabled(PhaseType type) const;

		// Configuration
		void setMinConfidence(double minConf) { _minConfidence = minConf; }
		void setUsePolarization(bool use) { _usePolarization = use; }
		void setUseSpectralAnalysis(bool use) { _useSpectralAnalysis = use; }
		void setUseMachineLearning(bool use) { _useML = use; }

	private:
		// Classification methods
		PhaseResult classifyByPolarization(
			const std::vector<double>& waveform,
			double sampleRate,
			const std::string& component
		);

		PhaseResult classifyBySpectralAnalysis(
			const std::vector<double>& waveform,
			double sampleRate
		);

		PhaseResult classifyByML(
			const std::vector<double>& waveform,
			double sampleRate,
			double pickTime,
			double backAzimuth,
			double distance
		);

		PhaseResult classifyByHeuristics(
			const std::vector<double>& waveform,
			double sampleRate,
			const std::string& component,
			double backAzimuth,
			double distance
		);

		// Feature extraction
		std::map<std::string, double> extractFeatures(
			const std::vector<double>& waveform,
			double sampleRate
		);

		// Phase identification helpers
		PhaseType identifyPhaseFamily(double snr, double period, const std::string& component);
		std::string determinePhaseName(PhaseType family, const std::string& component, double distance);

		// Configuration
		std::string _method;                    // Classification method
		double _minConfidence;                  // Minimum confidence threshold
		bool _usePolarization;                  // Use polarization analysis
		bool _useSpectralAnalysis;              // Use spectral analysis
		bool _useML;                            // Use machine learning
		std::map<PhaseType, bool> _enabledTypes; // Enabled phase types

		// ML model (placeholder for future integration)
		void* _mlModel;                         // ML model pointer
};


}
}
}


#endif
