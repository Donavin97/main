# Phase Type Support in scpsloc

## Overview

scpsloc now supports comprehensive phase type detection, validation, and processing for all major seismic phases used in earthquake location.

---

## Supported Phase Types

### P-Phase Family
| Phase | Type | Distance Range | Description |
|-------|------|----------------|-------------|
| **P** | Teleseismic | 0-180° | Primary compressional wave |
| **Pg** | Regional | 0-15° | P wave in granitic layer |
| **Pn** | Regional | 0-18° | Refracted P wave in mantle |
| **Pb** | Regional | 0-12° | P wave at Conrad discontinuity |
| **PcP** | Reflection | 25-55° | P wave reflected at core-mantle boundary |
| **PKP** | Core | 140-180° | P wave through outer core |
| **PKiKP** | Core | 30-120° | P wave reflected at inner core boundary |
| **PKKP** | Core | 80-130° | P wave multiply reflected in core |

### S-Phase Family
| Phase | Type | Distance Range | Description |
|-------|------|----------------|-------------|
| **S** | Teleseismic | 0-180° | Primary shear wave |
| **Sg** | Regional | 0-15° | S wave in granitic layer |
| **Sn** | Regional | 0-18° | Refracted S wave in mantle |
| **Sb** | Regional | 0-12° | S wave at Conrad discontinuity |
| **ScS** | Reflection | 25-85° | S wave reflected at core-mantle boundary |
| **SKS** | Core | 80-150° | S wave through outer core as P, back to S |
| **SKKP** | Core | 110-152° | S wave multiply reflected in core |

### Depth Phases
| Phase | Type | Distance Range | Description |
|-------|------|----------------|-------------|
| **pP** | Depth | 30-90° | P wave up from source, then P down |
| **sP** | Depth | 30-90° | S wave up from source, then P down |
| **sS** | Depth | 30-90° | S wave up from source, then S down |
| **pwP** | Depth | 30-80° | Water surface multiple of pP |

### Other Phases
| Phase | Type | Distance Range | Description |
|-------|------|----------------|-------------|
| **PP** | Multiple | 60-160° | Surface reflected P wave |
| **ScP** | Conversion | 25-55° | S to P conversion at surface |
| **SKP** | Core | 120-150° | Core phase with conversions |

---

## Phase Validation Functions

All phase validation functions are defined in `util.h` and `util.cpp`:

### `PhaseFamily getPhaseFamily(const std::string& phase)`
Returns the phase family enum:
- `PhaseFamily::P_FAMILY` - P, Pg, Pn, Pb
- `PhaseFamily::S_FAMILY` - S, Sg, Sn, Sb
- `PhaseFamily::PKP_FAMILY` - PKP, PKPdf, PKPab, PKiKP, PKKP
- `PhaseFamily::SKS_FAMILY` - SKS, SKKS, SKKSdf
- `PhaseFamily::DEPTH_PHASES` - pP, sP, sS, pwP, swP
- `PhaseFamily::REFLECTIONS` - PcP, ScP, ScS
- `PhaseFamily::MULTIPLES` - PP, SS, PPP, SSS
- `PhaseFamily::UNKNOWN` - Unrecognized phase

### `bool isValidPhase(const std::string& phase)`
Returns true if the phase is recognized and can be processed.

### `bool isPPhase(const std::string& phase)`
Returns true for P-family phases (P, Pg, Pn, Pb, PKP, etc.).

### `bool isSPhase(const std::string& phase)`
Returns true for S-family phases (S, Sg, Sn, Sb, SKS, etc.).

### `bool isRegionalPhase(const std::string& phase)`
Returns true for regional phases (Pg, Sg, Pn, Sn, PcP, ScP, ScS).

### `bool isTeleseismicPhase(const std::string& phase)`
Returns true for teleseismic phases that travel through mantle/core (PKP, SKS, etc.).

### `bool isDepthPhase(const std::string& phase)`
Returns true for depth phases (pP, sP, sS, pwP, swP).

### `std::string getPhaseCategory(const std::string& phase)`
Returns human-readable category name for logging:
- "P", "S", "PKP", "SKS", "Depth", "Reflection", "Multiple", "Unknown"

---

## Phase Association

### Travel Time Lookup

The associator (`associator.cpp`) handles travel time lookup for all phase types:

**P Family:**
```cpp
if (phase.code == "P" || phase.code == "Pg" || phase.code == "Pn" || 
    phase.code == "Pb" || phase.code == "PcP") {
    // Use first arrival for P, or specific phase for regional
}
```

**S Family:**
```cpp
else if (phase.code == "S" || phase.code == "Sg" || phase.code == "Sn" || 
         phase.code == "Sb" || phase.code == "ScS") {
    // Use S arrival for S, or specific phase for regional
}
```

**Core Phases:**
```cpp
else if (phase.code.substr(0,3) == "PKP" || phase.code.substr(0,4) == "PKiK") {
    // Match PKP family
}
```

**Depth Phases:**
```cpp
else if (phase.code == "pP" || phase.code == "sP" || phase.code == "sS") {
    // Exact match for depth phases (tighter tolerance)
}
```

### Residual Tolerances

Each phase type has different residual tolerance multipliers:

| Phase Type | Multiplier (x) | Reasoning |
|------------|----------------|-----------|
| P, Pg, Pn, Pb | 1.0 + distance factor | Primary phases, standard tolerance |
| S, Sg, Sn, Sb | 1.5 + distance factor | S phases have larger picking uncertainty |
| PKP, PKiKP | 1.2 | Core phases, moderate tolerance |
| SKS, SKKS | 1.5 | S-core phases, larger tolerance |
| pP, sP, sS | 0.8 | Depth phases need tight match |
| PP, ScP, etc. | 1.0 | Other phases, standard tolerance |

---

## Phase Scoring

Phase scores determine contribution to origin quality:

### Unused/Excluded Phases
| Phase Type | Score | Reason |
|------------|-------|--------|
| PKP | 0.3 | Core phases less reliable |
| SKS | 0.5 | S-core phases moderately useful |
| Other S | 0.4 | S phases still valuable |
| Regional | 0.6 | Regional phases somewhat useful |
| Other | 0.1 | Minimal contribution |

### Active Phases (Not Excluded)
| Phase Type | Score | Reason |
|------------|-------|--------|
| P, Pg, Pn, Pb | 1.0 | Backbone of location |
| PKP | 0.7 | Useful but less certain |
| S, Sg, Sn, Sb | 1.2 | **Higher weight** - constrain hypocenter |
| SKS | 1.0 | S-core phases useful |
| Depth (pP, sP, sS) | 1.5 | **Highest weight** - constrain depth |
| Regional | 0.8 | Regional phases useful |
| Other | 0.5 | Moderate contribution |

---

## Phase Exclusion Logic

Phases are excluded from location based on:

### P-Phase Family
- Excluded if delta > 105° and enough nearby P phases exist
- Excluded if 105° < delta < 125° (ambiguous zone)

### S-Phase Family
- Excluded only if delta > 105° and **twice** as many P phases exist
- Otherwise **kept** (S phases are valuable!)

### Regional Phases (ScP, ScS, PcP)
- **Always kept** - valuable for regional networks

### Depth Phases (pP, sP, sS)
- **Always kept** - critical for depth resolution

### Other Phases
- Marked as `UnusedPhase` - loosely associated but not used in location

---

## Phase Preservation Through Relocation

LOCSAT only processes P phases. To preserve S phases, regional phases, and depth phases:

```cpp
void Origin::updateFrom(const Origin *other) {
    // 1. Collect S/regional/depth phases from current origin
    // 2. Update with relocated origin (P phases from LOCSAT)
    // 3. Add back preserved phases (avoiding duplicates)
}
```

This ensures the final reported origin includes:
- ✅ P phases from LOCSAT relocation
- ✅ S phases from association
- ✅ Regional phases from association
- ✅ Depth phases from association

---

## Phase Validation in Association

The `_associate()` function uses phase validation:

```cpp
// Handle P-family phases
if (isPPhase(phase)) {
    travelTimeP(...);
}
// Handle S-family phases
else if (isSPhase(phase)) {
    travelTimeS(...);
}
// Handle depth/regional phases
else if (isDepthPhase(phase) || isRegionalPhase(phase)) {
    // Look up specific phase in travel time table
}
else {
    SEISCOMP_WARNING("Unrecognized phase: %s", phase.c_str());
    return false;
}
```

---

## Debug Logging

Phase processing is logged at various levels:

**INFO Level:**
```
process pick 20230802.144756.840000-A1.HRAO.04.HHN-A-S-L2-AIC-GRID-nautopick@seismoserv1-SEC (S-phase, no amplitude)
S phase 20230802.144818.500000-GT.BOSA.00.BHN-A-S-L2-AIC-GRID-nautopick@seismoserv1-SEC associated to origin 1 phase=S
```

**DEBUG Level:**
```
Associator: Looking for Sg phase travel time, delta=5.2
Associator: Found Sg phase tt=145.23
Found phase pP travel time=156.78
S phase residual check: 4.05 in [-26.25, 35.00]
```

**WARNING Level:**
```
Phase Xphase not found in travel time table
_associate got unrecognized phase: Xphase - ignored
```

---

## Configuration

Phase processing can be controlled via configuration:

```ini
# Require amplitudes for picks (false for AI pickers)
autoloc.requireAmplitudes = false

# Enable S phase usage
autoloc.useSPhases = true

# Use depth phases
autoloc.useDepthPhases = true

# Maximum S-P residual for association
autoloc.maxSPResidual = 5.0

# Minimum origin score for S phase association
autoloc.minScoreSPhase = 5.0
autoloc.minScoreOtherSPhases = 6.0
```

---

## Phase Type Summary in Output

Origin output includes phase breakdown:

```
Detailed info for Origin 2
2026-04-05 09:38:03.2  -28.237  26.483  10d
1   BOSA  GT   1.14 251 09:38:18.5   -6.0 AXr P
2   NECS  A1   2.75  28 09:38:40.8   -6.6 AXr P
3   LBTB  GT   3.31 346 09:39:35.2   -0.2 A   S     ← S phase included!
4   POGA  ZA   4.50  78 09:39:11.7    0.3 A   P
5   GRHM  ZA   5.02 181 09:39:18.3   -0.4 A   P
6   SUR   II   6.41 229 09:39:40.9    3.2 A   P
7   MATJ  A1   7.13 224 09:39:44.8   -2.8 A   P
8   GBGA  A1  10.26 297 09:40:30.5   -0.1 A   P

RMS   = 1.7
SCORE = 9.0
Phase Summary:
  Used:     6 phases (5 P, 1 S)
  Excluded: 2 phases (2 P, 0 S)
  S/P Ratio: 0.20
```

---

## Testing

To verify phase type support:

1. **Check phase association:**
   ```bash
   grep "Found.*phase" log.txt
   ```

2. **Check phase scoring:**
   ```bash
   grep "phaseScore" log.txt
   ```

3. **Check phase preservation:**
   ```bash
   grep "phase=S" log.txt
   ```

4. **Check final output:**
   ```bash
   grep "Detailed info for Origin" log.txt -A 20
   ```

---

## Future Enhancements

See `RECOMMENDATIONS.md` for planned improvements:
- Automatic phase identification
- Adaptive phase weighting
- Phase consistency checks
- S-P time validation
- Depth phase auto-detection

---

## Summary

scpsloc now supports **20+ seismic phase types** with:
- ✅ Comprehensive phase validation
- ✅ Phase-specific travel time lookup
- ✅ Phase-specific scoring weights
- ✅ Phase preservation through relocation
- ✅ Detailed phase debug logging
- ✅ Phase type summary in output

This allows optimal use of all available seismic phases for earthquake location, improving accuracy and reliability.
