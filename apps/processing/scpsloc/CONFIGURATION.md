# scpsloc Configuration Guide

## Overview

The scpsloc module is configured through the `scpsloc.cfg` file located in:
```
src/base/main/apps/processing/scpsloc/config/scpsloc.cfg
```

After installation, this file is typically found in:
```
@CONFDIR@/scpsloc.cfg
```

## Configuration Sections

### 1. Connection Settings

#### `connection.primaryGroup`
- **Type**: String
- **Default**: `LOCATION`
- **Description**: The messaging group to send origins to

```ini
connection.primaryGroup = LOCATION
```

#### `connection.subscriptions`
- **Type**: Comma-separated list
- **Default**: `PICK, AMPLITUDE`
- **Description**: Messaging groups to subscribe to

```ini
# Basic subscription
connection.subscriptions = PICK, AMPLITUDE

# Add LOCATION if you want to receive manual origins
connection.subscriptions = PICK, AMPLITUDE, LOCATION
```

### 2. Standard Location Parameters

#### `autoloc.maxRMS`
- **Type**: Double
- **Default**: `3.5`
- **Unit**: seconds
- **Description**: Maximum travel-time RMS for a location to be reported

```ini
autoloc.maxRMS = 3.5
```

#### `autoloc.maxResidual`
- **Type**: Double
- **Default**: `7.0`
- **Unit**: seconds
- **Description**: Maximum travel-time residual (unweighted) for a pick to be used

```ini
autoloc.maxResidual = 7.0
```

#### `autoloc.minPhaseCount`
- **Type**: Integer
- **Default**: `6`
- **Description**: Minimum number of phases for reporting origins

```ini
autoloc.minPhaseCount = 6
```

#### `autoloc.maxSGAP`
- **Type**: Double
- **Default**: `360`
- **Unit**: degrees
- **Description**: Maximum secondary azimuthal gap for an origin to be reported

```ini
autoloc.maxSGAP = 360
```

#### `autoloc.minStaCountIgnorePKP`
- **Type**: Integer
- **Default**: `15`
- **Description**: If station count at < 105° exceeds this, picks at > 105° won't be used

```ini
autoloc.minStaCountIgnorePKP = 15
```

### 3. P/S Phase Parameters (NEW)

#### `autoloc.useSPhases`
- **Type**: Boolean
- **Default**: `true`
- **Description**: Enable S phase usage in location

```ini
# Enable S phase association and usage
autoloc.useSPhases = true

# Disable S phases (behave like scautoloc)
autoloc.useSPhases = false
```

**When to disable**: 
- Testing/comparison with scautoloc
- Network doesn't record S phases well
- Velocity model lacks S-phase tables

#### `autoloc.minSPRatio`
- **Type**: Double
- **Default**: `0.0`
- **Range**: 0.0 to 1.0+
- **Description**: Minimum S/P phase ratio for quality control

```ini
# No S/P ratio requirement
autoloc.minSPRatio = 0.0

# Require at least 30% as many S phases as P phases
autoloc.minSPRatio = 0.3

# Require equal number of S and P phases
autoloc.minSPRatio = 1.0
```

**Recommended values**:
- `0.0`: No requirement (default)
- `0.2-0.3`: Loose requirement for regional networks
- `0.4-0.5`: Moderate requirement for good networks
- `> 0.5`: Strict, may reject valid events

#### `autoloc.sPhaseWeight`
- **Type**: Double
- **Default**: `1.2`
- **Description**: Weight multiplier for S phases relative to P phases

```ini
# S phases same weight as P
autoloc.sPhaseWeight = 1.0

# S phases 20% more weight (default)
autoloc.sPhaseWeight = 1.2

# S phases 50% more weight
autoloc.sPhaseWeight = 1.5

# S phases twice as important as P
autoloc.sPhaseWeight = 2.0
```

**Recommended values**:
- `1.0`: Equal weighting
- `1.2`: Slightly favor S phases (default, recommended)
- `1.5`: Strongly favor S phases
- `> 2.0`: May overemphasize S phases

#### `autoloc.useDepthPhases`
- **Type**: Boolean
- **Default**: `true`
- **Description**: Use depth phases (pP, sP, sS) for improved depth resolution

```ini
# Enable depth phase detection and usage
autoloc.useDepthPhases = true

# Disable depth phase usage
autoloc.useDepthPhases = false
```

**When to disable**:
- Shallow crustal events where depth phases are weak
- Poor signal-to-noise ratio
- Testing/comparison purposes

#### `autoloc.maxSPResidual`
- **Type**: Double
- **Default**: `5.0`
- **Unit**: seconds
- **Description**: Maximum S-P time residual for association

```ini
# Tight association (may miss valid S phases)
autoloc.maxSPResidual = 3.0

# Default association tolerance
autoloc.maxSPResidual = 5.0

# Loose association (may include incorrect phases)
autoloc.maxSPResidual = 8.0
```

**Recommended values**:
- `3.0`: Tight, high-quality picks
- `5.0`: Default (recommended)
- `7.0-8.0`: Loose, noisy data
- `> 10.0`: Very loose, not recommended

### 4. File Paths

#### `autoloc.grid`
- **Type**: File path
- **Default**: `@DATADIR@/scpsloc/grid.conf`
- **Description**: Grid configuration file for nucleation

```ini
autoloc.grid = @DATADIR@/scpsloc/grid.conf
```

#### `autoloc.stationConfig`
- **Type**: File path
- **Default**: `@DATADIR@/scpsloc/station.conf`
- **Description**: Station configuration file

```ini
autoloc.stationConfig = @DATADIR@/scpsloc/station.conf
```

#### `autoloc.pickLog`
- **Type**: File path
- **Default**: `@LOGDIR@/scpsloc-picklog`
- **Description**: Pick log file location

```ini
autoloc.pickLog = @LOGDIR@/scpsloc-picklog
```

### 5. Locator Profile

#### `locator.profile`
- **Type**: String
- **Default**: `iasp91`
- **Description**: Velocity model to use

```ini
# Global standard
locator.profile = iasp91

# Regional model (example)
locator.profile = tab

# Custom model
locator.profile = ak135
```

**Important**: The velocity model must include S-phase travel time tables.

#### `locator.defaultDepth`
- **Type**: Double
- **Default**: `10`
- **Unit**: km
- **Description**: Default depth for comparison

```ini
locator.defaultDepth = 10
```

#### `locator.minimumDepth`
- **Type**: Double
- **Default**: `5`
- **Unit**: km
- **Description**: Minimum allowed depth

```ini
locator.minimumDepth = 5
```

### 6. Manual Picks/Origins

#### `autoloc.useManualPicks`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Process manual phase picks

```ini
autoloc.useManualPicks = false
```

#### `autoloc.useManualOrigins`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Process manual origins

```ini
autoloc.useManualOrigins = false
```

**Note**: If true, add `LOCATION` to `connection.subscriptions`.

#### `autoloc.adoptManualDepth`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Adopt depth from manual origins

```ini
autoloc.adoptManualDepth = false
```

## Example Configurations

### Basic Configuration (Default)

```ini
## Connection
connection.primaryGroup = LOCATION
connection.subscriptions = PICK, AMPLITUDE

## Standard parameters
autoloc.maxRMS = 3.5
autoloc.minPhaseCount = 6
autoloc.maxResidual = 7.0

## P/S phase parameters (all defaults)
autoloc.useSPhases = true
autoloc.sPhaseWeight = 1.2
autoloc.useDepthPhases = true
autoloc.minSPRatio = 0.0
autoloc.maxSPResidual = 5.0

## Locator
locator.profile = iasp91
locator.defaultDepth = 10
locator.minimumDepth = 5

## Logging
autoloc.pickLog = @LOGDIR@/scpsloc-picklog
autoloc.amplTypeSNR = snr
autoloc.amplTypeAbs = mb
```

### Comparison Mode (Like scautoloc)

```ini
## Connection
connection.primaryGroup = LOCATION
connection.subscriptions = PICK, AMPLITUDE

## Disable S phases to match scautoloc behavior
autoloc.useSPhases = false
autoloc.useDepthPhases = false

## Standard parameters
autoloc.maxRMS = 3.5
autoloc.minPhaseCount = 6
autoloc.maxResidual = 7.0
```

### High-Quality Network

```ini
## Connection
connection.primaryGroup = LOCATION
connection.subscriptions = PICK, AMPLITUDE

## Require good phase balance
autoloc.useSPhases = true
autoloc.minSPRatio = 0.4
autoloc.sPhaseWeight = 1.3
autoloc.useDepthPhases = true
autoloc.maxSPResidual = 4.0

## Tighter quality control
autoloc.maxRMS = 2.5
autoloc.minPhaseCount = 8
autoloc.maxResidual = 5.0
```

### Regional Network (Dense)

```ini
## Connection
connection.primaryGroup = LOCATION
connection.subscriptions = PICK, AMPLITUDE

## Moderate S phase requirements
autoloc.useSPhases = true
autoloc.minSPRatio = 0.3
autoloc.sPhaseWeight = 1.2
autoloc.useDepthPhases = true
autoloc.maxSPResidual = 5.0

## Regional settings
autoloc.maxStationDistance = 10
autoloc.minPhaseCount = 6
autoloc.minStaCountIgnorePKP = 10
```

### Teleseismic Network

```ini
## Connection
connection.primaryGroup = LOCATION
connection.subscriptions = PICK, AMPLITUDE, LOCATION

## Looser S phase association
autoloc.useSPhases = true
autoloc.minSPRatio = 0.2
autoloc.sPhaseWeight = 1.5
autoloc.useDepthPhases = true
autoloc.maxSPResidual = 7.0

## Teleseismic settings
autoloc.maxStationDistance = 180
autoloc.minPhaseCount = 10
autoloc.maxRMS = 4.0
```

## Configuration Tips

### 1. Start with Defaults
Begin with default settings and adjust based on results.

### 2. Monitor S/P Ratios
Check the output to see typical S/P ratios for your network:
- If consistently low (< 0.2): May indicate S phases not being picked
- If moderate (0.3-0.6): Good phase balance
- If high (> 0.7): Excellent, but verify S picks are correct

### 3. Adjust Gradually
Change one parameter at a time and monitor results:
1. Start with `useSPhases = true`
2. Monitor S/P ratios
3. Adjust `minSPRatio` if needed
4. Tune `sPhaseWeight` based on location quality

### 4. Compare with scautoloc
Run both modules to compare:
```bash
seiscomp start scautoloc
seiscomp start scpsloc
```

Compare:
- Number of events located
- Depth resolution
- RMS values
- Location uncertainties

### 5. Quality Control

Recommended monitoring:
- Check pick log: `@LOGDIR@/scpsloc-picklog`
- Monitor S phase counts in output
- Verify depth improvements
- Compare with manual locations

## Troubleshooting

### No S Phases Associated

**Symptoms**: S phase count = 0 in output

**Solutions**:
1. Verify `autoloc.useSPhases = true`
2. Check velocity model has S-phase tables
3. Confirm S picks are being generated
4. Check `autoloc.maxSPResidual` is not too tight

### Too Many Rejected Events

**Symptoms**: Fewer events located than scautoloc

**Solutions**:
1. Lower `autoloc.minSPRatio` or set to 0.0
2. Increase `autoloc.maxSPResidual`
3. Check if S picks are actually bad quality
4. Temporarily disable S phases for comparison

### Poor Depth Resolution

**Symptoms**: Depth uncertainties still large

**Solutions**:
1. Ensure `autoloc.useDepthPhases = true`
2. Check if depth phases (pP, sP, sS) are being picked
3. Increase `autoloc.sPhaseWeight` to 1.5
4. Verify velocity model accuracy

### High RMS Values

**Symptoms**: Many events with RMS > 5s

**Solutions**:
1. Check pick quality
2. Verify velocity model is appropriate
3. Reduce `autoloc.maxSPResidual` to tighten association
4. Increase `autoloc.minPhaseCount`

## Advanced Topics

### Phase Scoring Weights

Internal scoring (cannot be directly configured, but good to know):
- P phases: 1.0 (base)
- S phases: 1.2 × `sPhaseWeight` parameter
- PKP phases: 0.7
- Depth phases: 1.5
- Other S phases: 0.9

### Quality Metrics

Each origin includes:
- `pPhaseCount`: Number of P phases used
- `sPhaseCount`: Number of S phases used
- `spRatio`: S/P phase ratio (sCount/pCount)
- `aziGapPrimary`: Primary azimuthal gap
- `aziGapSecondary`: Secondary azimuthal gap

### Performance Tuning

For high seismicity:
```ini
autoloc.cleanupInterval = 1800  # Clean up more frequently
autoloc.maxAge = 10800          # Keep less history
autoloc.wakeupInterval = 3      # Process more frequently
```

## See Also

- `README.md` - Comprehensive documentation
- `QUICKSTART.md` - Quick start guide
- SeisComP configuration documentation
