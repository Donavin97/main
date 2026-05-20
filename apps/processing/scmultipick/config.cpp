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

#include <stdio.h>

#include <seiscomp/logging/log.h>
#include <seiscomp/client/application.h>

#include "config.h"


namespace Seiscomp {
namespace Applications {
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MultiPick::Config::init(const Client::Application *app) {
	try { amplitudeGroup = app->configGetString("connection.amplitudeGroup"); }
	catch ( ... ) {}

	try { phaseHint = app->configGetString("phaseHint"); }
	catch ( ... ) {}

	try { secondaryPhaseHint = app->configGetString("secondaryPhaseHint"); }
	catch ( ... ) {}

	try { commentID = app->configGetString("comment.ID"); }
	catch ( ... ) {}

	try { commentText = app->configGetString("comment.text"); }
	catch ( ... ) {}

	try { calculateAmplitudes = app->configGetBool("calculateAmplitudes"); }
	catch (...) {}

	try { defaultFilter = app->configGetString("filter"); }
	catch (...) {}

	try { defaultSFilter = app->configGetString("sFilter"); }
	catch (...) {}

	try { useAllStreams = app->configGetBool("useAllStreams"); }
	catch (...) {}

	// Sensitivity correction: convert raw counts to physical units (m/s)
	// This is critical for proper SNR estimation and amplitude computation
	try { sensitivityCorrection = app->configGetBool("sensitivityCorrection"); }
	catch (...) {}

	try { defaultTimeCorrection = app->configGetDouble("timeCorrection"); }
	catch (...) {}
	try { ringBufferSize = app->configGetDouble("ringBufferSize"); }
	catch (...) {}
	try { leadTime = app->configGetDouble("leadTime"); }
	catch (...) {}
	try { initTime = app->configGetDouble("initTime"); }
	catch (...) {}
	try { interpolateGaps = app->configGetBool("gapInterpolation"); }
	catch (...) {}

	try { defaultTriggerOnThreshold = app->configGetDouble("thresholds.triggerOn"); }
	catch (...) {}
	try { defaultTriggerOffThreshold = app->configGetDouble("thresholds.triggerOff"); }
	catch (...) {}
	try { maxGapLength = app->configGetDouble("thresholds.maxGapLength"); }
	catch (...) {}
	try { triggerDeadTime = app->configGetDouble("thresholds.deadTime"); }
	catch (...) {}
	try { minDuration = app->configGetDouble("thresholds.minDuration"); }
	catch (...) {}
	try { maxDuration = app->configGetDouble("thresholds.maxDuration"); }
	catch (...) {}

	try { amplitudeMaxTimeWindow = app->configGetDouble("thresholds.amplMaxTimeWindow"); }
	catch (...) {}
	try { amplitudeMinOffset = app->configGetDouble("thresholds.minAmplOffset"); }
	catch (...) {}

	try { minSNR = app->configGetDouble("minSNR"); }
	catch (...) {}

	try {
		std::vector<std::string> amplitudes = app->configGetStrings("amplitudes");
		amplitudeList.clear();
		amplitudeList.insert(amplitudes.begin(), amplitudes.end());
	}
	catch (...) {}

	try {
		std::vector<std::string> amplitudes = app->configGetStrings("amplitudes.enableUpdate");
		amplitudeUpdateList.clear();
		amplitudeUpdateList.insert(amplitudes.begin(), amplitudes.end());
	}
	catch (...) {}

	try { pickerType = app->configGetString("picker"); }
	catch ( ... ) {}

	// Accept pickerType as alias for picker (backwards compatibility)
	if ( pickerType.empty() ) {
		try { pickerType = app->configGetString("pickerType"); }
		catch ( ... ) {}
	}
	// Also accept multipick.pPickerType
	if ( pickerType.empty() ) {
		try { pickerType = app->configGetString("multipick.pPickerType"); }
		catch ( ... ) {}
	}

	// Load secondary picker type (S-phase picker)
	// Try multiple config key names for backwards compatibility
	try { secondaryPickerType = app->configGetString("secondaryPickerType"); }
	catch ( ... ) {}
	if ( secondaryPickerType.empty() ) {
		try { secondaryPickerType = app->configGetString("spicker"); }
		catch ( ... ) {}
	}
	if ( secondaryPickerType.empty() ) {
		try { secondaryPickerType = app->configGetString("spickerType"); }
		catch ( ... ) {}
	}
	if ( secondaryPickerType.empty() ) {
		try { secondaryPickerType = app->configGetString("multipick.sPickerType"); }
		catch ( ... ) {}
	}
	if ( secondaryPickerType.empty() ) {
		try { secondaryPickerType = app->configGetString("multipick.sPicker"); }
		catch ( ... ) {}
	}

	try { featureExtractionType = app->configGetString("fx"); }
	catch ( ... ) {}

	try { killPendingSecondaryProcessors = app->configGetBool("killPendingSPickers"); }
	catch ( ... ) {}

	try { sendDetections = app->configGetBool("sendDetections"); }
	catch ( ... ) {}

	try { extraPickComments = app->configGetBool("extraPickComments"); }
	catch ( ... ) {}

	try { playback = app->configGetBool("playback"); }
	catch ( ... ) {}

	try { generateSimplifiedIDs = app->configGetBool("simplifiedIDs"); }
	catch ( ... ) {}

	// ============================================================
	// Multi-phase configuration options
	// ============================================================

	try { enablePPhases = app->configGetBool("multipick.enablePPhases"); }
	catch ( ... ) {}

	try { enableSPhases = app->configGetBool("multipick.enableSPhases"); }
	catch ( ... ) {}

	try { enableRegionalPhases = app->configGetBool("multipick.enableRegionalPhases"); }
	catch ( ... ) {}

	try { enableCorePhases = app->configGetBool("multipick.enableCorePhases"); }
	catch ( ... ) {}

	try { phaseClassificationMethod = app->configGetString("multipick.phaseClassificationMethod"); }
	catch ( ... ) {}

	try { minSPhaseSNR = app->configGetDouble("multipick.minSPhaseSNR"); }
	catch ( ... ) {}

	try { minPPhaseSNR = app->configGetDouble("multipick.minPPhaseSNR"); }
	catch ( ... ) {}

	try { sPhaseSearchWindow = app->configGetDouble("multipick.sPhaseSearchWindow"); }
	catch ( ... ) {}

	try { minPSTimeDiff = app->configGetDouble("multipick.minPSTimeDiff"); }
	catch ( ... ) {}

	try { autoPhaseByComponent = app->configGetBool("multipick.autoPhaseByComponent"); }
	catch ( ... ) {}

	try { useAmplitudeRatio = app->configGetBool("multipick.useAmplitudeRatio"); }
	catch ( ... ) {}

	try { vhRatioThreshold = app->configGetDouble("multipick.vhRatioThreshold"); }
	catch ( ... ) {}

	try { usePolarization = app->configGetBool("multipick.usePolarization"); }
	catch ( ... ) {}

	try { maxRegionalDistance = app->configGetDouble("multipick.maxRegionalDistance"); }
	catch ( ... ) {}

	try { minTeleseismicDistance = app->configGetDouble("multipick.minTeleseismicDistance"); }
	catch ( ... ) {}

	// S-phase specific filter and thresholds
	try { sPhaseFilter = app->configGetString("multipick.sFilter"); }
	catch ( ... ) {}

	// Accept both naming conventions for backwards compatibility
	try {
		double val = app->configGetDouble("multipick.sTriggerOnThreshold");
		sTriggerOnThreshold = val;
	}
	catch ( ... ) {
		try {
			double val = app->configGetDouble("multipick.sTriggerOn");
			sTriggerOnThreshold = val;
		}
		catch ( ... ) {}
	}

	try {
		double val = app->configGetDouble("multipick.sTriggerOffThreshold");
		sTriggerOffThreshold = val;
	}
	catch ( ... ) {
		try {
			double val = app->configGetDouble("multipick.sTriggerOff");
			sTriggerOffThreshold = val;
		}
		catch ( ... ) {}
	}

	try {
		double val = app->configGetDouble("multipick.sTimeCorrection");
		sTimeCorrection = val;
	}
	catch ( ... ) {}

	// Accept both naming conventions
	try { sPickerType = app->configGetString("multipick.sPickerType"); }
	catch ( ... ) {
		try { sPickerType = app->configGetString("multipick.sPicker"); }
		catch ( ... ) {}
	}

	// Accept both naming conventions
	try { sTriggerDeadTime = app->configGetDouble("multipick.sTriggerDeadTime"); }
	catch ( ... ) {
		try { sTriggerDeadTime = app->configGetDouble("multipick.sDeadTime"); }
		catch ( ... ) {}
	}

	// P-phase trigger thresholds (also accept top-level minSNR as trigger threshold)
	try { defaultTriggerOnThreshold = app->configGetDouble("multipick.pTriggerOnThreshold"); }
	catch ( ... ) {
		try { defaultTriggerOnThreshold = app->configGetDouble("multipick.pTriggerOn"); }
		catch ( ... ) {
			try { defaultTriggerOnThreshold = app->configGetDouble("minSNR"); }
			catch ( ... ) {}
		}
	}

	try { defaultTriggerOffThreshold = app->configGetDouble("multipick.pTriggerOffThreshold"); }
	catch ( ... ) {
		try { defaultTriggerOffThreshold = app->configGetDouble("multipick.pTriggerOff"); }
		catch ( ... ) {}
	}

	try { sPhaseSearchWindow = app->configGetDouble("multipick.sPhaseSearchWindow"); }
	catch ( ... ) {}

	try { minPSTimeDiff = app->configGetDouble("multipick.minPSTimeDiff"); }
	catch ( ... ) {}

	// ============================================================
	// Enhanced AIC picker configuration
	// ============================================================
	try { useEnhancedAICPicker = app->configGetBool("multipick.useEnhancedAICPicker"); }
	catch ( ... ) {}

	try { aicMarginMin = app->configGetInt("multipick.aicMarginMin"); }
	catch ( ... ) {}

	try { aicMarginMax = app->configGetInt("multipick.aicMarginMax"); }
	catch ( ... ) {}

	try { aicConfidenceThreshold = app->configGetDouble("multipick.aicConfidenceThreshold"); }
	catch ( ... ) {}

	try { aicAdaptiveWindows = app->configGetBool("multipick.aicAdaptiveWindows"); }
	catch ( ... ) {}

	try { aicMultiBand = app->configGetBool("multipick.aicMultiBand"); }
	catch ( ... ) {}

	try { aicNumBands = app->configGetInt("multipick.aicNumBands"); }
	catch ( ... ) {}
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MultiPick::Config::init(const System::CommandLine &commandline) {
	test = commandline.hasOption("test");
	offline = commandline.hasOption("offline") || commandline.hasOption("ep");
	dumpRecords = commandline.hasOption("dump-records");
	sendDetections = commandline.hasOption("send-detections") ? true : sendDetections;
	extraPickComments = commandline.hasOption("extra-comments") ? true : extraPickComments;
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
void MultiPick::Config::dump() const {
	printf("Configuration:\n");
	printf("amplitude group                  %s\n",     amplitudeGroup.c_str());
	printf("testMode                         %s\n",     test ? "true":"false");
	printf("offline                          %s\n",     offline ? "true":"false");
	printf("useAllStreams                    %s\n",     useAllStreams ? "true":"false");
	printf("calculateAmplitudes              %s\n",     calculateAmplitudes ? "true":"false");
	printf("calculateAmplitudeTypes          ");
	if ( amplitudeList.empty() ) {
		printf("[]\n");
	}
	else {
		for ( StringSet::const_iterator it = amplitudeList.begin();
		      it != amplitudeList.end(); ++it ) {
			if ( it != amplitudeList.begin() ) {
				printf(", ");
			}
			printf("%s", it->c_str());
		}
		printf("\n");
	}

	printf("update amplitude types           ");
	if ( amplitudeUpdateList.empty() ) {
		printf("[]\n");
	}
	else {
		for ( StringSet::const_iterator it = amplitudeUpdateList.begin();
		      it != amplitudeUpdateList.end(); ++it ) {
			if ( it != amplitudeList.begin() ) {
				printf(", ");
			}
			printf("%s", it->c_str());
		}
		printf("\n");
	}
	printf("interpolateGaps                  %s\n",    interpolateGaps ? "true":"false");
	printf("maxGapLength                     %.2fs\n", maxGapLength);
	printf("defaultFilter                    %s\n",    defaultFilter.c_str());
	printf("defaultSFilter                   %s\n",    defaultSFilter.c_str());
	printf("defaultTriggerOnThreshold        %.2f\n",  defaultTriggerOnThreshold);
	printf("defaultTriggerOffThreshold       %.2fs\n", defaultTriggerOffThreshold);
	printf("minDuration                      %.2fs\n", minDuration);
	printf("maxDuration                      %.2f\n",  maxDuration);
	printf("triggerDeadTime                  %.2fs\n", triggerDeadTime);
	printf("amplitudeMaxTimeWindow           %.2fs\n", amplitudeMaxTimeWindow);
	printf("amplitudeMinOffset               %.2fs\n", amplitudeMinOffset);
	printf("defaultTimeCorrection            %.2fs\n", defaultTimeCorrection);
	printf("ringBufferSize                   %.0fs\n", ringBufferSize);
	printf("leadTime                         %.0fs\n", leadTime);
	printf("initTime                         %.0fs\n", initTime);
	printf("pickerType                       %s\n",    pickerType.c_str());
	printf("secondaryPickerType              %s\n",    secondaryPickerType.c_str());
	printf("killPendingSPickers              %s\n",    killPendingSecondaryProcessors ? "true" : "false");
	printf("sendDetections                   %s\n",    sendDetections ? "true" : "false");

	// Multi-phase specific options
	printf("\nMulti-phase configuration:\n");
	printf("enablePPhases                    %s\n",    enablePPhases ? "true" : "false");
	printf("enableSPhases                    %s\n",    enableSPhases ? "true" : "false");
	printf("enableRegionalPhases             %s\n",    enableRegionalPhases ? "true" : "false");
	printf("enableCorePhases                 %s\n",    enableCorePhases ? "true" : "false");
	printf("phaseClassificationMethod        %s\n",    phaseClassificationMethod.c_str());
	printf("minSPhaseSNR                     %.2f\n",  minSPhaseSNR);
	printf("minPPhaseSNR                     %.2f\n",  minPPhaseSNR);
	printf("sPhaseSearchWindow               %.2fs\n", sPhaseSearchWindow);
	printf("minPSTimeDiff                    %.2fs\n", minPSTimeDiff);
	printf("autoPhaseByComponent             %s\n",    autoPhaseByComponent ? "true" : "false");
	printf("useAmplitudeRatio                %s\n",    useAmplitudeRatio ? "true" : "false");
	printf("vhRatioThreshold                 %.2f\n",  vhRatioThreshold);
	printf("usePolarization                  %s\n",    usePolarization ? "true" : "false");
	printf("maxRegionalDistance              %.2f\n",  maxRegionalDistance);
	printf("minTeleseismicDistance           %.2f\n",  minTeleseismicDistance);
	if ( !sPhaseFilter.empty() )
		printf("sPhaseFilter                     %s\n",  sPhaseFilter.c_str());
	if ( sTriggerOnThreshold )
		printf("sTriggerOn                       %.2f\n",  *sTriggerOnThreshold);
	if ( sTriggerOffThreshold )
		printf("sTriggerOff                      %.2f\n",  *sTriggerOffThreshold);
	if ( sTimeCorrection )
		printf("sTimeCorrection                  %.2f\n",  *sTimeCorrection);
	if ( !sPickerType.empty() )
		printf("sPickerType                      %s\n",  sPickerType.c_str());
	printf("sDeadTime                        %.2fs\n", sTriggerDeadTime);

	// Enhanced AIC picker configuration
	printf("\nEnhanced AIC picker:\n");
	printf("useEnhancedAICPicker             %s\n",    useEnhancedAICPicker ? "true" : "false");
	printf("aicMarginMin                     %d\n",     aicMarginMin);
	printf("aicMarginMax                     %d\n",     aicMarginMax);
	printf("aicConfidenceThreshold           %.2f\n",  aicConfidenceThreshold);
	printf("aicAdaptiveWindows               %s\n",    aicAdaptiveWindows ? "true" : "false");
	printf("aicMultiBand                     %s\n",    aicMultiBand ? "true" : "false");
	printf("aicNumBands                      %d\n",     aicNumBands);
}
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<




// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
}
}
