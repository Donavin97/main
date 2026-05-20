# Quick Start Guide: scpsloc

## What is scpsloc?

**scpsloc** is an advanced P/S phase locator for SeisComP that provides improved earthquake location accuracy by using both P and S phases, compared to the standard scautoloc which only uses P phases.

## Key Advantages

✅ **Better depth resolution** - S phases provide additional depth constraints  
✅ **Improved location accuracy** - More phase types = better locations  
✅ **Quality metrics** - S/P phase ratio for quality assessment  
✅ **Fewer stations needed** - Can locate events with fewer stations  
✅ **Backward compatible** - Works like scautoloc when S phases disabled  

## Building

```bash
cd /home/seismocomp/scfork/build
make scpsloc -j4
```

Binary will be at: `build/bin/scpsloc` (6.1 MB)

## Basic Usage

### 1. Enable in SeisComP

```bash
seiscomp enable scpsloc
```

### 2. Configure (Optional)

The configuration file is located at: `config/scpsloc.cfg`

Key parameters you can adjust:

```ini
# Enable S phases (default: true)
autoloc.useSPhases = true

# Give S phases higher weight (default: 1.2)
autoloc.sPhaseWeight = 1.2

# Enable depth phases (default: true)  
autoloc.useDepthPhases = true

# Require minimum S/P ratio (0.0 = no requirement)
autoloc.minSPRatio = 0.0

# Maximum S-P residual for association (seconds)
autoloc.maxSPResidual = 5.0
```

### 3. Start

```bash
seiscomp start scpsloc
```

That's it! scpsloc will automatically:
- Connect to the messaging system
- Receive picks from scautopick
- Associate both P and S phases
- Produce improved locations

## Configuration Options

### Essential Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `autoloc.useSPhases` | true | Enable S phase usage |
| `autoloc.sPhaseWeight` | 1.2 | Weight for S phases |
| `autoloc.useDepthPhases` | true | Enable depth phases |
| `autoloc.minSPRatio` | 0.0 | Minimum S/P ratio |
| `autoloc.maxSPResidual` | 5.0 | Max S-P residual (s) |

### Standard Parameters

All scautoloc parameters work with scpsloc:
- `locator.profile` - Velocity model
- `locator.defaultDepth` - Default depth
- `autoloc.maxRMS` - Maximum RMS
- `autoloc.minPhaseCount` - Minimum picks
- And many more...

## Advanced Features

### Phase Scoring

scpsloc uses optimized phase scoring:
- **P phases**: 1.0 (backbone)
- **S phases**: 1.2 (higher weight)
- **Depth phases**: 1.5 (highest weight)
- **PKP phases**: 0.7

### Quality Metrics

Each location includes:
- P phase count
- S phase count
- S/P phase ratio
- Azimuthal gaps

### Supported Phases

- **P family**: P, PcP, PKP, PKiKP, PKKP
- **S family**: S, ScS, SKS, SKKP
- **Depth phases**: pP, sP, sS
- **Others**: PP, ScP, SKP

## Troubleshooting

### No S Phases Associated?

1. Check `autoloc.useSPhases = true`
2. Verify velocity model has S-phase tables
3. Check if S picks are being generated

### Poor Depth Resolution?

1. Enable depth phases: `autoloc.useDepthPhases = true`
2. Check S phase count in output
3. Verify good pick quality

### Want to Compare with scautoloc?

Run both modules and compare:
```bash
seiscomp start scautoloc
seiscomp start scpsloc
```

## Documentation

- **README.md** - Comprehensive documentation
- **CHANGES.md** - Implementation details
- **descriptions/scpsloc.rst** - SeisComP documentation

## Support

For detailed information, see:
- `/home/seismocomp/scfork/src/base/main/apps/processing/scpsloc/README.md`
- `/home/seismocomp/scfork/src/base/main/apps/processing/scpsloc/CHANGES.md`

## Next Steps

1. ✅ Module built successfully
2. ⏳ Test with your data
3. ⏳ Compare results with scautoloc
4. ⏳ Adjust parameters as needed
5. ⏳ Deploy to production

---

**Branch**: `feature/ps-phase-locator`  
**Status**: Ready for testing  
**Build**: Successful (6.1 MB binary)
