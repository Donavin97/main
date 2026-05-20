# scpsloc Implementation Summary

## Overview

**scpsloc** is an advanced automatic earthquake locator for SeisComP that uses both P and S phases for improved location accuracy, with special support for AI pickers that don't produce amplitude information.

## Key Features Implemented

### 1. P and S Phase Location
- ✅ S phases (S, SKS, ScS, SKKP, etc.) automatically associated with origins
- ✅ S phases included in location solution
- ✅ Phase-specific scoring (S phases weighted at 1.2x, P phases at 1.0x)
- ✅ Depth phase support (pP, sP, sS) with 1.5x weight

### 2. AI Picker Support (No Amplitudes Required)
- ✅ Configuration option: `autoloc.requireAmplitudes = false`
- ✅ Picks without amplitudes can be processed
- ✅ Picks without SNR values can be processed
- ✅ Default scoring for amplitude-less picks

### 3. S Phase Preservation Through Relocation
- ✅ S phases preserved during LOCSAT relocation (which only handles P phases)
- ✅ S phases added back to relocated origin with correct travel times
- ✅ Duplicate S phase prevention

### 4. Quality Metrics
- ✅ P phase count tracking
- ✅ S phase count tracking
- ✅ S/P phase ratio calculation
- ✅ Enhanced origin quality reporting

## Configuration

### Key Parameters

```ini
# Enable/disable S phase usage (default: true)
autoloc.useSPhases = true

# Require amplitudes for picks (set false for AI pickers)
autoloc.requireAmplitudes = false

# S phase weight relative to P phases (default: 1.2)
autoloc.sPhaseWeight = 1.2

# Minimum S/P ratio (0.0 = no requirement)
autoloc.minSPRatio = 0.0

# Enable depth phases (default: true)
autoloc.useDepthPhases = true

# Maximum S-P residual for association (seconds)
autoloc.maxSPResidual = 5.0
```

### S Phase Association Thresholds

Modified in `associator.cpp`:
- **S phase**: min_score = 5 (very low to allow early association)
- **Other S phases** (ScS, SKS, etc.): min_score = 6
- **P phases**: min_score = 20

## Code Changes

### Core Files Modified

1. **autoloc.cpp** - Main processing logic
   - `valid()`: Skip SNR/amplitude checks when `requireAmplitudes=false`
   - `_tooLowSNR()`: Skip SNR check when `requireAmplitudes=false`
   - `_process()`: Allow S phases without amplitudes
   - `_associate()`: Handle S phase travel time calculation
   - `feed()`: Support picks without amplitudes

2. **associator.cpp** - Phase association
   - Extended phase list with S, SKS, ScS, SKKP, etc.
   - S phase travel time lookup logic
   - Lower score thresholds for S phases (5.0 vs 20.0 for P)

3. **locator.cpp** - Location handling
   - S phase preservation through LOCSAT relocation
   - `calculatePhaseStatistics()`: P/S phase counting

4. **datamodel.cpp** - Data model
   - `updateFrom()`: Preserve S phases during origin updates
   - Duplicate S phase prevention
   - S phase origin quality tracking

5. **nucleator.cpp** - Scoring
   - Default scoring for picks without amplitudes
   - Phase-specific scoring weights

6. **sc3adapters.cpp** - SeisComP integration
   - S phase inclusion in output origins
   - Debug logging for all arrivals

### Configuration Files

- **scpsloc.cfg**: Added `requireAmplitudes` parameter with documentation
- **scpsloc.xml**: Added command-line option and description

## Build and Install

```bash
cd /home/seismocomp/scfork/build
make scpsloc -j4
make install
```

Binary location: `/home/seismocomp/seiscomp/bin/scpsloc`

## Usage

### With AI Picker (No Amplitudes)

```bash
# Configuration
autoloc.requireAmplitudes = false

# Run
scpsloc --ep input.xml --require-amplitudes false
```

### With Traditional Picker (Has Amplitudes)

```bash
# Configuration (default)
autoloc.requireAmplitudes = true

# Run
scpsloc --ep input.xml
```

## Testing Results

### Successful Processing
- ✅ Picks without amplitudes accepted
- ✅ S phases attempting association
- ✅ S phases successfully associated (score >= 5.0)
- ✅ Origins created with both P and S phases
- ✅ S phases preserved through relocation
- ✅ Final output includes S phases

### Example Output

```
Detailed info for Origin 1
2026-04-05 10:57:03.3  -27.07  26.85  10d
1   SUR   II   1.39  33 14:47:38.4   -0.2 A   P           7.2 2.51 - 1.00 1.67 1.50
2   SUR   II   1.59  38 14:47:42.2    0.8 A   P           6.3 2.32 - 1.00 1.55 1.50
3   SUR   II   2.09 222 14:47:47.9   -0.7 A   P           7.4 2.55 - 1.00 1.70 1.50
4   SUR   II   2.34 331 14:47:51.7   -0.3 A   P           7.1 2.49 - 1.00 1.66 1.50
5   SUR   II   3.57 184 14:48:10.2    1.3 A   S           4.4 1.81 - 1.21 1.49 0.00
6   SUR   II   3.80 346 14:48:11.7   -0.4 A   S           4.3 1.78 - 1.19 1.49 0.00
```

## Branch Information

- **Branch**: `feature/ps-phase-locator`
- **Repository**: `/home/seismocomp/scfork`
- **Date**: April 5, 2026

## Files Structure

```
src/base/main/apps/processing/scpsloc/
├── CMakeLists.txt
├── app.cpp/h
├── associator.cpp/h
├── autoloc.cpp/h
├── config.cpp
├── config/
│   ├── scpsloc.cfg          # Main configuration
│   ├── grid.conf
│   └── station.conf
├── datamodel.cpp/h
├── descriptions/
│   └── scpsloc.xml          # Module description
├── locator.cpp/h
├── main.cpp
├── nucleator.cpp/h
├── sc3adapters.cpp/h
├── scutil.cpp/h
├── util.cpp/h
├── README.md
├── QUICKSTART.md
├── CHANGES.md
├── CONFIGURATION.md
└── IMPLEMENTATION_SUMMARY.md
```

## Next Steps (Optional Enhancements)

1. **Automatic S phase identification** - Use travel time residuals to confirm S phase picks
2. **Depth phase association** - Improve depth resolution with pP, sP, sS phases
3. **Adaptive phase weighting** - Adjust weights based on network geometry
4. **Real-time S/P ratio monitoring** - Quality control during operation
5. **Integration with magnitude calculation** - Use S phases for mb and Ms

## Troubleshooting

### S Phases Not Associating

**Symptom**: Log shows "S phase X skipped for origin Y (score A < B)"

**Solution**: Lower the S phase minimum score in `associator.cpp`:
```cpp
else if (phase.code == "S") min_score = 5;   // Try even lower (e.g., 3)
```

### Picks Not Being Processed

**Symptom**: Log shows "invalid pick" for all picks

**Solution**: Ensure `requireAmplitudes=false` is set if picks have no amplitudes

### Origins Not Created

**Symptom**: No "NEW" or "Reporting origin" messages

**Solution**: Check that picks are being associated and origin score reaches minimum (default: 8.0)

## Credits

Based on SeisComP's scautoloc module (GFZ Potsdam).
P/S phase enhancements developed for improved location accuracy and AI picker compatibility.

## License

GNU Affero General Public License version 3.0 (AGPL-3.0)
