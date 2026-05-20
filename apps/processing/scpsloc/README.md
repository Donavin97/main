# scpsloc - Advanced P/S Phase Locator for SeisComP

## Overview

**scpsloc** is an advanced automatic earthquake locator for SeisComP that uses both P and S phases for improved location accuracy, particularly for depth resolution. It is based on the scautoloc module but extends its capabilities significantly.

## Key Features

### 1. P and S Phase Location
- Automatically associates and uses both P and S phases
- S phases (S, SKS, ScS, etc.) provide additional constraints on the hypocenter
- Particularly valuable for improving depth resolution
- Better constrains the epicenter with fewer stations

### 2. Depth Phase Support
- Automatically detects and uses depth phases (pP, sP, sS)
- Extremely valuable for constraining focal depth
- Especially beneficial for deeper events where depth resolution is typically poor

### 3. Enhanced Phase Scoring
The scoring system has been optimized for P/S phase location:
- **P phases**: Base score of 1.0 (backbone of location)
- **S phases**: Higher score of 1.2 (very valuable for depth and location)
- **PKP phases**: Score of 0.7 (useful but less certain)
- **Depth phases (pP, sP, sS)**: Highest score of 1.5 (extremely valuable for depth)
- **Other S phases (ScS, etc.)**: Score of 0.9

### 4. Quality Metrics
scpsloc computes additional quality metrics:
- P phase count
- S phase count
- S/P phase ratio
- Primary and secondary azimuthal gaps

These metrics help assess the quality and reliability of locations.

### 5. Advanced Phase Association

#### Supported Phases
- **P-phase family**: P, PcP, PKP, PKiKP, PKKP
- **S-phase family**: S, ScS, SKS, SKKP
- **Depth phases**: pP, sP, sS
- **Other phases**: PP, ScP, SKP

#### Phase-Specific Features
- Automatic P to PKP renaming for teleseismic distances (>114°)
- Automatic S to SKS renaming for teleseismic distances
- S phase travel time lookup with proper handling
- Depth phase detection with tighter residual tolerances

## Configuration

### New P/S Phase Parameters

The following configuration parameters are available in the SeisComP configuration:

#### `autoloc.useSPhases` (boolean, default: true)
Enable/disable S phase usage in location. When enabled, S phases (S, SKS, ScS, etc.) will be automatically associated and used in the location process.

#### `autoloc.minSPRatio` (double, default: 0.0)
Minimum S/P phase ratio for quality control. A value of 0.0 means no requirement. Setting this to e.g. 0.3 would require at least 30% as many S phases as P phases. This helps ensure good phase balance for reliable locations.

#### `autoloc.sPhaseWeight` (double, default: 1.2)
Weight multiplier for S phases relative to P phases. A value of 1.0 means S phases have the same weight as P phases. Values > 1.0 give S phases higher weight, reflecting their value for constraining the hypocenter and improving depth resolution.

#### `autoloc.useDepthPhases` (boolean, default: true)
Enable/disable depth phase usage. When enabled, depth phases (pP, sP, sS) will be automatically detected and used to constrain the focal depth.

#### `autoloc.maxSPResidual` (double, default: 5.0)
Maximum S-P time residual for association (seconds). Controls how tightly S phases must match the predicted S-P time based on the P phase arrival. Smaller values ensure tighter association but may miss valid S phases with larger uncertainties.

### Standard Parameters

All standard scautoloc parameters are also supported:
- `locator.profile`: Velocity model (e.g., iasp91, tab)
- `locator.defaultDepth`: Default depth for comparison
- `locator.minimumDepth`: Minimum allowed depth
- `autoloc.maxRMS`: Maximum travel-time RMS
- `autoloc.minPhaseCount`: Minimum number of picks
- And many more...

## Usage

### Starting scpsloc

scpsloc runs as a standard SeisComP module:

```bash
# Start as a SeisComP daemon
seiscomp start scpsloc

# Or run directly
scpsloc [options]
```

### Command-Line Options

All standard scautoloc options are supported:

```
--test              Do not send any object
--offline           Do not connect to messaging (offline mode)
--playback          Flush origins immediately without delay
--ep file           Input XML file for offline processing
--formatted         Use formatted XML output
```

Plus scpsloc-specific options will be available for P/S phase control.

## Architecture

### Module Structure

```
scpsloc/
├── app.cpp/h            - Main application entry point
├── autoloc.cpp/h        - Core autopicking logic with P/S support
├── associator.cpp/h     - Enhanced phase associator (P, S, depth phases)
├── locator.cpp/h        - Locator wrapper with P/S statistics
├── nucleator.cpp/h      - Grid search nucleator
├── config.cpp           - Configuration management
├── datamodel.cpp/h      - Custom data model
├── util.cpp/h           - Utility functions including travelTimeS()
├── sc3adapters.cpp/h    - SeisComP 3 adapters
├── scutil.cpp/h         - Utility functions
├── main.cpp             - Main entry point
├── config/              - Configuration files
└── descriptions/        - Documentation files
```

### Key Enhancements Over scautoloc

1. **associator.cpp**: Extended to handle S-phase associations with proper travel time lookups
2. **autoloc.cpp**: Enhanced `_associate()` to process S phases and depth phases
3. **locator.cpp**: Added `calculatePhaseStatistics()` for P/S phase metrics
4. **util.cpp**: Added `travelTimeS()` function for S-phase travel time calculation
5. **nucleator.cpp**: Updated scoring system for optimal P/S phase weighting
6. **datamodel.h**: Extended `OriginQuality` class with P/S phase statistics

## Technical Details

### Phase Association Logic

The associator now:
1. Checks for P phases with standard travel time tables
2. Checks for S phases with dedicated S-phase travel time lookup
3. Handles depth phases (pP, sP, sS) with tighter tolerances
4. Automatically renames P→PKP and S→SKS for teleseismic distances

### Travel Time Calculation

Two main functions handle travel time lookups:
- `travelTimeP()`: For P-phase family (P, PKP, etc.)
- `travelTimeS()`: For S-phase family (S, SKS, etc.)

Both functions properly handle different distance ranges and phase identification.

### Quality Control

The system computes:
- Number of P phases used in location
- Number of S phases used in location
- S/P phase ratio for quality assessment
- These metrics are stored in `OriginQuality` structure

## Building

### Prerequisites
- SeisComP development environment
- CMake
- Standard build tools

### Build Instructions

```bash
cd /path/to/seiscomp/build
cmake ..
make scpsloc -j4
```

The binary will be created in `build/bin/scpsloc`.

## Comparison with scautoloc

| Feature | scautoloc | scpsloc |
|---------|-----------|---------|
| P phases | ✓ | ✓ |
| S phases | ✗ | ✓ |
| Depth phases | Limited | ✓ (enhanced) |
| S/P ratio metrics | ✗ | ✓ |
| Enhanced S-phase scoring | ✗ | ✓ |
| Phase statistics | Basic | Advanced |
| Location accuracy | Good | Better (especially depth) |

## Future Enhancements

Potential areas for future development:
1. Machine learning-based phase identification
2. Real-time S/P ratio quality monitoring
3. Adaptive phase weighting based on network geometry
4. Integration with magnitude calculation
5. Support for additional phases (PP, SS, etc.)

## Troubleshooting

### Common Issues

**Issue**: No S phases being associated
- **Solution**: Check that `autoloc.useSPhases=true` in configuration
- **Solution**: Verify velocity model includes S-phase tables

**Issue**: Poor depth resolution
- **Solution**: Enable depth phases with `autoloc.useDepthPhases=true`
- **Solution**: Check if S phases are being used (should see S phase count > 0)

**Issue**: Low S/P ratio
- **Solution**: This may indicate a real seismicity pattern or network issue
- **Solution**: Consider adjusting `autoloc.maxSPResidual` to allow looser association

## Credits

scpsloc is based on the SeisComP scautoloc module developed by GFZ Potsdam.

The P/S phase enhancements were developed to improve automatic location quality, particularly for depth resolution.

## License

Same as SeisComP - GNU Affero General Public License version 3.0 (AGPL-3.0)

## Support

For issues or questions:
- Check SeisComP documentation
- Review configuration parameters
- Examine log files for phase association details
