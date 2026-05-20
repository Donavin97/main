# scpsloc Future Improvements & Features

## Overview

This document outlines recommended improvements and new features for the scpsloc (P/S phase locator) module. Features are organized by priority and include implementation details to guide development.

---

## 🎯 HIGH PRIORITY (Immediate Value)

### 1. Configurable S Phase Association Thresholds

**Current State:** S phase minimum score thresholds are hardcoded in `associator.cpp` (S=5.0, other S=6.0).

**Proposed Change:** Move to configuration file for user control.

**Implementation:**
- Add to `autoloc.h` Config struct:
  ```cpp
  double minScoreSPhase{5.0};
  double minScoreOtherSPhases{6.0};
  ```
- Add to `app.cpp` config loading:
  ```cpp
  try { _config.minScoreSPhase = configGetDouble("autoloc.minScoreSPhase"); }
  catch (...) {}
  try { _config.minScoreOtherSPhases = configGetDouble("autoloc.minScoreOtherSPhases"); }
  catch (...) {}
  ```
- Update `associator.cpp` to use config values instead of hardcoded constants
- Add to `scpsloc.cfg`:
  ```ini
  # Minimum origin score required for S phase association
  # Lower values allow S phases to associate earlier
  # Default: 5.0 for S, 6.0 for other S phases (ScS, SKS, etc.)
  #autoloc.minScoreSPhase = 5.0
  #autoloc.minScoreOtherSPhases = 6.0
  ```
- Add XML description in `scpsloc.xml`

**Benefit:** Users can tune S phase association behavior for their specific network characteristics.

**Effort:** Low (2-3 hours)

---

### 2. S/P Ratio Quality Control & Reporting

**Current State:** S/P ratio is calculated but not reported or monitored.

**Proposed Change:** Track, report, and optionally enforce S/P ratio requirements.

**Implementation:**
- Add to `OriginQuality` in `datamodel.h`:
  ```cpp
  std::string spRatioQualityFlag() const;  // Returns: excellent/good/fair/poor
  ```
- Implement quality flag logic:
  ```cpp
  std::string OriginQuality::spRatioQualityFlag() const {
      if (pPhaseCount == 0) return "undefined";
      if (spRatio >= 0.5) return "excellent";
      if (spRatio >= 0.3) return "good";
      if (spRatio >= 0.1) return "fair";
      return "poor";
  }
  ```
- Add to `printOrigin()` in `util.cpp`:
  ```cpp
  out << "S/P Ratio = " << origin->quality.spRatio 
      << " (" << origin->quality.sPhaseCount << "/" 
      << origin->quality.pPhaseCount << " phases)" << std::endl;
  out << "S/P Quality = " << origin->quality.spRatioQualityFlag() << std::endl;
  ```
- Add optional enforcement in `_publishable()`:
  ```cpp
  if (_config.minSPRatio > 0.0 && origin->quality.spRatio < _config.minSPRatio) {
      SEISCOMP_INFO("Origin %ld not sent (S/P ratio %.2f < %.2f)",
                   origin->id, origin->quality.spRatio, _config.minSPRatio);
      return false;
  }
  ```

**Benefit:** Users can assess location quality based on S phase usage and optionally require minimum S phase coverage.

**Effort:** Medium (4-5 hours)

---

### 3. Enhanced S Phase Debug Logging

**Current State:** Limited visibility into S phase processing decisions.

**Proposed Change:** Add detailed logging for S phase association, inclusion, and exclusion.

**Implementation:**
- In `_associate()`, add S phase specific logging:
  ```cpp
  if (phase == "S" || phase == "SKS") {
      SEISCOMP_INFO("S phase %s associated to origin %ld: residual=%.2f, affinity=%.4f",
                   pick->label.c_str(), origin->id, residual, affinity);
  }
  ```
- In `_trimResiduals()` and `_removeOutliers()`, track S phases:
  ```cpp
  if (is_S_arrival(arr) && arr.excluded) {
      SEISCOMP_DEBUG("S phase %s excluded: residual=%.2f exceeds threshold",
                    arr.pick->label.c_str(), arr.residual);
  }
  ```
- Add summary when origin is reported:
  ```cpp
  size_t sCount = 0, pCount = 0;
  for (const auto& arr : origin->arrivals) {
      if (arr.excluded == Arrival::NotExcluded) {
          if (arr.phase[0] == 'S') sCount++;
          else pCount++;
      }
  }
  SEISCOMP_INFO("Origin %ld: %d phases used (%d P, %d S)",
               origin->id, pCount+sCount, pCount, sCount);
  ```

**Benefit:** Easier debugging and understanding of S phase behavior in locations.

**Effort:** Low (2-3 hours)

---

### 4. Phase Type Summary in Origin Output

**Current State:** Origin output shows all arrivals but no summary statistics.

**Proposed Change:** Add clear summary of phase types used/excluded.

**Implementation:**
- Modify `printOrigin()` in `util.cpp` to add summary section:
  ```cpp
  // Count phase types
  size_t pUsed=0, sUsed=0, pExcl=0, sExcl=0;
  for (const auto& arr : origin->arrivals) {
      bool isS = (arr.phase[0] == 'S');
      if (arr.excluded == Arrival::NotExcluded) {
          if (isS) sUsed++; else pUsed++;
      } else {
          if (isS) sExcl++; else pExcl++;
      }
  }
  
  out << std::endl << "Phase Summary:" << std::endl;
  out << "  Used:     " << (pUsed+sUsed) << " phases (" << pUsed << " P, " << sUsed << " S)" << std::endl;
  out << "  Excluded: " << (pExcl+sExcl) << " phases (" << pExcl << " P, " << sExcl << " S)" << std::endl;
  out << "  S/P Ratio: " << (pUsed > 0 ? (double)sUsed/pUsed : 0.0) << std::endl;
  ```

**Benefit:** Clear, immediate visibility of S phase contribution to each location.

**Effort:** Low (1-2 hours)

---

## 🔧 MEDIUM PRIORITY (Significant Value)

### 5. Automatic S Phase Travel Time Validation

**Current State:** S phases are associated based on travel time tables but not validated post-association.

**Proposed Change:** Validate S phase picks match predicted S arrival times and flag anomalies.

**Implementation:**
- Add validation function:
  ```cpp
  bool validateSPhase(const Pick* pick, const Origin* origin) {
      // Calculate expected S arrival time
      TravelTime tt = getSTravelTime(origin->hypocenter, pick->station());
      double predictedTime = origin->time + tt.time;
      double residual = pick->time - predictedTime;
      
      // Check if residual is reasonable (e.g., within 10 seconds)
      return std::abs(residual) < 10.0;
  }
  ```
- Add warning in association:
  ```cpp
  if (!validateSPhase(pick, origin)) {
      SEISCOMP_WARNING("S phase %s has unusual residual (%.1f s) - possible misidentification",
                      pick->label.c_str(), residual);
  }
  ```

**Benefit:** Improved data quality through automatic detection of misidentified phases.

**Effort:** Medium (4-6 hours)

---

### 6. Adaptive S Phase Weighting

**Current State:** S phase weight is fixed at 1.2x P phase weight.

**Proposed Change:** Dynamically adjust S phase weight based on network geometry and location quality.

**Implementation:**
- Add adaptive weight calculation:
  ```cpp
  double calculateSPhaseWeight(const Origin* origin) {
      double baseWeight = _config.sPhaseWeight;
      
      // If depth resolution is poor, increase S phase weight
      if (origin->hypocenter.deperr > 50.0) {
          baseWeight *= 1.5;
      }
      
      // If S phases have large residuals, decrease weight
      double avgSResidual = calculateAvgSResidual(origin);
      if (avgSResidual > 5.0) {
          baseWeight *= 0.8;
      }
      
      return baseWeight;
  }
  ```

**Benefit:** More intelligent use of S phases based on actual data quality.

**Effort:** Medium-High (6-8 hours)

---

### 7. S Phase Origin Quality Flag

**Current State:** No standardized quality metric for S phase usage.

**Proposed Change:** Add quality flag to origin output and XML.

**Implementation:**
- Add to `OriginQuality`:
  ```cpp
  enum class SPhaseQuality {
      EXCELLENT,  // S/P > 0.5
      GOOD,       // S/P 0.3-0.5
      FAIR,       // S/P 0.1-0.3
      POOR,       // S/P < 0.1
      NONE        // No S phases
  };
  
  SPhaseQuality getSPhaseQuality() const;
  ```
- Include in SeisComP DataModel output via custom attributes or journal entries

**Benefit:** Standardized quality metric for downstream processing and filtering.

**Effort:** Medium (3-4 hours)

---

### 8. Command-Line Options for S Phase Control

**Current State:** S phase behavior controlled only via config file.

**Proposed Change:** Add command-line options for runtime control.

**Implementation:**
- In `app.cpp` `createCommandLineDescription()`:
  ```cpp
  commandline().addOption("Settings", "min-s-phase-ratio",
                         "Minimum S/P phase ratio for origin reporting.",
                         &_config.minSPRatio);
  commandline().addOption("Settings", "require-s-phases",
                         "Require at least one S phase for origin reporting.",
                         &_config.requireSPhases);
  commandline().addOption("Settings", "s-phase-weight",
                         "Weight multiplier for S phases (default: 1.2).",
                         &_config.sPhaseWeight);
  ```

**Benefit:** Easier testing and workflow customization without editing config files.

**Effort:** Low (1-2 hours)

---

## 📊 NICE TO HAVE (Future Enhancements)

### 9. S Phase Statistics in XML Output

**Proposed Change:** Include S phase statistics in SeisComP XML output.

**Implementation:**
- Extend `convertToSC()` to add custom attributes or use existing quality fields:
  ```xml
  <origin quality="...">
    <comment>
      <text>S/P Ratio: 0.33 (2S/6P)</text>
      <text>S Phase Quality: good</text>
    </comment>
  </origin>
  ```

**Benefit:** S phase information available to downstream modules (scolv, scbulletin).

**Effort:** Low-Medium (2-3 hours)

---

### 10. S Phase Travel Time Table Caching

**Current State:** S phase travel times recalculated on every association attempt.

**Proposed Change:** Cache travel times for station-origin pairs.

**Implementation:**
- Add cache structure:
  ```cpp
  struct TravelTimeCacheEntry {
      double lat, lon, dep;
      double staLat, staLon;
      double time;
      double delta;
      double azimuth;
  };
  
  std::vector<TravelTimeCacheEntry> _travelTimeCache;
  ```
- Check cache before computing:
  ```cpp
  TravelTime* getSTravelTimeWithCache(...) {
      // Check cache first
      for (const auto& entry : _travelTimeCache) {
          if (matches(entry, ...)) return entry;
      }
      // Compute and cache
      TravelTime tt = computeTravelTime(...);
      _travelTimeCache.push_back(tt);
      return tt;
  }
  ```

**Benefit:** Significant performance improvement for large datasets with many picks.

**Effort:** Medium (4-5 hours)

---

### 11. Automatic Depth Phase Detection

**Current State:** Depth phases (pP, sP, sS) supported but require manual identification.

**Proposed Change:** Automatically detect and utilize depth phases from pick labels or timing.

**Implementation:**
- Detect depth phases by pattern matching or S-P time analysis
- Enhance depth resolution using depth phase residuals
- Report depth phase usage in origin output

**Benefit:** Dramatically improved depth resolution for deeper events.

**Effort:** High (8-12 hours)

---

### 12. S Phase Consistency Check

**Proposed Change:** Validate S-P time differences are physically reasonable.

**Implementation:**
- For each station with both P and S picks:
  ```cpp
  double spTime = sPick.time - pPick.time;
  double expectedSP = getExpectedSPTime(distance, depth);
  if (std::abs(spTime - expectedSP) > tolerance) {
      SEISCOMP_WARNING("Station %s: S-P time %.1f s unusual (expected %.1f s)",
                      stationCode, spTime, expectedSP);
  }
  ```

**Benefit:** Automatic detection of timing errors or misidentified phases.

**Effort:** Medium (4-5 hours)

---

### 13. Dynamic S Phase Score Adjustment

**Proposed Change:** Adjust S phase contribution based on location convergence.

**Implementation:**
- Monitor location stability across iterations
- Increase S phase weight if location is unstable
- Decrease weight if S phases cause divergence

**Benefit:** More robust locations, especially for challenging events.

**Effort:** High (8-10 hours)

---

### 14. S Phase Network Coverage Visualization

**Proposed Change:** Export S phase station information for visualization.

**Implementation:**
- Add export function:
  ```cpp
  void exportSPhaseCoverage(const Origin* origin, const std::string& filename) {
      // Export as KML/GeoJSON
      // Include station locations, residuals, weights
  }
  ```

**Benefit:** Visual quality assessment and network planning.

**Effort:** Medium (4-6 hours)

---

### 15. Benchmarking Mode

**Proposed Change:** Compare locations with/without S phases.

**Implementation:**
- Add command-line flag: `--benchmark-s-phases`
- Run location twice (with and without S phases)
- Report differences:
  ```
  Location Comparison:
    P-only:     Lat -27.08, Lon 26.80, Depth 10 km, RMS 1.1
    P+S:        Lat -27.07, Lon 26.85, Depth 10 km, RMS 0.9
    
    Improvement: 1.2 km horizontal, 0.2 km depth, 18% RMS reduction
  ```

**Benefit:** Quantify value of S phases for your specific network.

**Effort:** Medium-High (6-8 hours)

---

## 🎯 Recommended Implementation Order

### Phase 1: Quick Wins (1-2 days)
1. Configurable S Phase Association Thresholds
2. Enhanced S Phase Debug Logging
3. Phase Type Summary in Origin Output
4. Command-Line Options for S Phase Control

### Phase 2: Quality Improvements (2-3 days)
5. S/P Ratio Quality Control & Reporting
6. S Phase Origin Quality Flag
7. Automatic S Phase Travel Time Validation
8. S Phase Consistency Check

### Phase 3: Advanced Features (3-5 days)
9. S Phase Statistics in XML Output
10. Adaptive S Phase Weighting
11. S Phase Travel Time Table Caching
12. Benchmarking Mode

### Phase 4: Future Development (1-2 weeks)
13. Automatic Depth Phase Detection
14. Dynamic S Phase Score Adjustment
15. S Phase Network Coverage Visualization

---

## 💡 Additional Considerations

### Testing Strategy
- Create test dataset with known S phases
- Verify S phase association accuracy
- Test with/without amplitudes
- Benchmark location quality improvements

### Documentation Updates
- Update user manual with S phase features
- Add configuration examples
- Create troubleshooting guide for S phase issues

### Backward Compatibility
- All new features should have sensible defaults
- Existing workflows should work without changes
- Deprecation warnings for any changed behavior

---

## 📝 Implementation Notes

### Key Files to Modify

| Feature | Primary Files |
|---------|--------------|
| Config thresholds | `autoloc.h`, `associator.cpp`, `scpsloc.cfg` |
| S/P ratio reporting | `datamodel.h`, `util.cpp`, `app.cpp` |
| Debug logging | `autoloc.cpp`, `associator.cpp` |
| Phase summary | `util.cpp` |
| Validation | `autoloc.cpp` |
| Adaptive weighting | `autoloc.cpp`, `locator.cpp` |
| Quality flags | `datamodel.h`, `sc3adapters.cpp` |
| Command-line options | `app.cpp` |

### Testing Checklist

- [ ] S phases associate with low-score origins
- [ ] S phases preserved through relocation
- [ ] S phases appear in final output
- [ ] S/P ratio calculated correctly
- [ ] Works with amplitude-less picks
- [ ] Works with mixed (with/without amplitude) picks
- [ ] Configuration changes take effect
- [ ] Debug logging provides useful information

---

## 🏁 Summary

The scpsloc module is now fully functional with P and S phase support and AI picker compatibility. The recommended improvements above will enhance:

1. **Usability**: Better configuration control and visibility
2. **Quality**: Automated validation and adaptive weighting
3. **Performance**: Caching and optimized processing
4. **Integration**: Better SeisComP ecosystem integration

**Estimated Total Effort:** 3-4 weeks for all features, or 1 week for Phase 1-2 priorities.

**Recommended Next Step:** Implement Phase 1 features (Configurable thresholds, Phase summary, Debug logging) for immediate user benefit with minimal effort.
