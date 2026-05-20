# scmultipick Implementation Status

## What Has Been Created ✅

### Directory Structure
```
src/base/main/apps/processing/scmultipick/
├── CMakeLists.txt              ✅ Build configuration
├── phaseclassifier.h            ✅ Phase classification interface
├── README.md                    ✅ Comprehensive documentation
└── IMPLEMENTATION_PLAN.md       ✅ This file
```

### Architecture Design
- **Phase types supported**: P, Pg, Pn, Pb, S, Sg, Sn, Sb, PKP, SKS, pP, sP, sS, etc.
- **Classification methods**: Polarization, Spectral, ML, Heuristic
- **Integration**: Fully compatible with scpsloc
- **Configuration**: Comprehensive config system planned

## What Still Needs To Be Implemented ⏳

### Core Files (Need Creation)

1. **app.cpp/h** - Main application class
   - Inherits from `Processing::Application`
   - Manages waveform processing
   - Handles pick output
   - ~500-800 lines of code

2. **detector.cpp/h** - STA/LTA detector
   - Can reuse/extend from scautopick
   - Trigger detection for all phases
   - ~300-500 lines

3. **picker.cpp/h** - Multi-phase picker
   - Main picking logic
   - Integrates with phase classifier
   - ~800-1200 lines

4. **phaseclassifier.cpp** - Phase classification implementation
   - Implements PhaseClassifier interface
   - Polarization analysis
   - Spectral analysis
   - Heuristic rules
   - ~600-1000 lines

5. **config.cpp/h** - Configuration management
   - Load/parse config file
   - Parameter validation
   - ~300-400 lines

6. **stationconfig.cpp/h** - Station configuration
   - Can reuse from scautopick
   - ~200-300 lines

7. **main.cpp** - Entry point
   - Simple main function
   - ~30 lines

### Configuration Files

8. **config/station.conf** - Station configuration template
9. **descriptions/scmultipick.xml** - Module description for SeisComP
10. **config/scmultipick.cfg** - Default configuration

## Implementation Approach

### Option 1: Extend scautopick (Recommended)
Copy scautopick as baseline and add:
- Phase classifier module
- Multi-phase detection logic
- Phase identification output
- **Effort**: 2-3 weeks

### Option 2: Build From Scratch
Create entirely new picker:
- Full control over architecture
- Can use modern techniques
- **Effort**: 4-6 weeks

### Option 3: AI-Based Picker
Integrate machine learning:
- Use EQCCT or similar
- Requires ML framework
- **Effort**: 6-8 weeks

## Recommended Next Steps

### Phase 1: Basic Structure (Week 1)
1. Copy scautopick files as baseline
2. Rename to scmultipick
3. Add phase classifier module
4. Update build system
5. Test compilation

### Phase 2: Phase Detection (Week 2)
1. Implement STA/LTA detection
2. Add P-phase picking
3. Add S-phase picking
4. Test with known events

### Phase 3: Phase Classification (Week 3)
1. Implement polarization analysis
2. Add spectral analysis
3. Implement heuristic rules
4. Test phase identification

### Phase 4: Integration (Week 4)
1. Connect to scpsloc
2. Test end-to-end workflow
3. Optimize performance
4. Documentation

## Key Design Decisions Needed

1. **Base Implementation**
   - Extend scautopick? (Recommended)
   - Build from scratch?
   - Use external library?

2. **Phase Classification Method**
   - Rule-based (simpler, faster)
   - ML-based (more accurate, complex)
   - Hybrid (best of both)

3. **Real-time vs Batch**
   - Real-time streaming (like scautopick)
   - Batch processing
   - Both modes

4. **AI/ML Integration**
   - Use existing models (EQCCT, PhaseNet)
   - Train custom model
   - No ML (traditional methods only)

## Files To Copy From scautopick

```bash
# Copy baseline
cp scautopick/detector.cpp scmultipick/
cp scautopick/detector.h scmultipick/
cp scautopick/picker.cpp scmultipick/
cp scautopick/picker.h scmultipick/
cp scautopick/config.cpp scmultipick/
cp scautopick/config.h scmultipick/
cp scautopick/stationconfig.cpp scmultipick/
cp scautopick/stationconfig.h scmultipick/
cp scautopick/main.cpp scmultipick/
cp scautopick/config/station.conf scmultipick/config/
cp scautopick/descriptions/scautopick.xml scmultipick/descriptions/scmultipick.xml
```

Then modify to add multi-phase support.

## Estimated Total Effort

| Phase | Duration | Lines of Code |
|-------|----------|---------------|
| Basic structure | 1 week | 500 |
| Detection | 1 week | 800 |
| Phase classification | 1-2 weeks | 1000 |
| Integration & testing | 1 week | 300 |
| Documentation | 0.5 week | 200 |
| **Total** | **4-5 weeks** | **~2800** |

## Immediate Next Action

Would you like me to:

**A)** Copy scautopick as baseline and start modifying it for multi-phase support? (Fastest approach)

**B)** Create the full implementation from scratch? (More control, longer timeline)

**C)** Create a detailed specification document first for your review? (Plan before coding)

**D)** Something else?

Let me know which approach you prefer and I'll proceed accordingly!
