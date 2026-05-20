# Innovative Seismic Phase Picking Methods - Review for scmultipick Integration

**Date:** April 2026
**Author:** Development Team
**Purpose:** Evaluate modern seismic picking techniques for integration into scmultipick

---

## 1. Executive Summary

This document reviews current state-of-the-art seismic phase picking methods and evaluates their suitability for integration into the scmultipick module of SeisComP. The review covers traditional, machine learning, and hybrid approaches, with recommendations for practical implementation phases.

**Key finding:** A phased integration approach is recommended, starting with AR-AIC refinement (already partially implemented), followed by ML picker plugins via ONNX Runtime, and eventually hybrid detection combining STA/LTA triggers with ML-based onset refinement.

---

## 2. Current State of scmultipick

### Existing Capabilities
- **Detection:** STA/LTA-based (recstalt, karsta, filterrecstalt picker types)
- **Amplitude:** Peak amplitude computation from filtered waveforms
- **Phase Classification:** Component-based (Z→P, N/E→S), with heuristic refinement
- **Multi-Phase:** Simultaneous P and S detection on separate components
- **Output:** Standard SeisComP picks compatible with scpsloc locator

### Limitations
- Phase classification limited to P/S (no Pg, Pn, Sg, Sn, PKP, etc.)
- No uncertainty quantification on picks
- Low SNR events missed due to fixed thresholds
- Single-station processing only (no network coherence)

---

## 3. Method Review

### 3.1 Traditional Methods

#### AR-AIC (Autoregressive Akaike Information Criterion)
- **Approach:** Models pre-arrival noise and post-arrival signal as AR processes; AIC identifies change point
- **Accuracy:** Mean error ~0.1-0.3s for P-waves, ~0.3-0.8s for S-waves
- **Real-time:** ✅ Yes, lightweight (~10ms/trace)
- **Unique Features:**
  - Sub-sample onset time precision
  - Works well on pre-triggered waveforms
  - No training data required
- **Integration Effort:** Low (already partially used in scautopick)
- **Recommendation:** ✅ **Implement first** - immediate precision improvement

#### STA/LTA + Kurtosis
- **Approach:** Combines energy ratio (STA/LTA) with statistical kurtosis change
- **Accuracy:** Similar to standard STA/LTA, better noise rejection
- **Real-time:** ✅ Yes
- **Unique Features:** More robust in noisy environments
- **Integration Effort:** Low
- **Recommendation:** ⚠️ Already covered by existing picker types

#### Wavelet-Based Picking
- **Approach:** Wavelet transform to isolate phase-specific frequency bands
- **Accuracy:** Good in low SNR conditions
- **Real-time:** ✅ Yes, moderate compute
- **Unique Features:** Excellent noise rejection
- **Integration Effort:** Medium
- **Recommendation:** ⚠️ Consider if SNR issues persist

#### Polarization Analysis
- **Approach:** Eigenvalue decomposition of 3-component covariance matrix
- **Accuracy:** Good P/S discrimination
- **Real-time:** ✅ Yes
- **Unique Features:** Physics-based, no training needed
- **Integration Effort:** Medium
- **Recommendation:** ✅ Useful as supplementary classifier

---

### 3.2 Deep Learning Methods

#### PhaseNet (Zhu & Beroza, 2019)
- **Architecture:** U-Net CNN
- **Input:** 3-component waveform window (typically 30s)
- **Output:** Per-sample probability for P, S, Noise
- **Accuracy:**
  - P-wave MAE: ~0.1-0.2s
  - S-wave MAE: ~0.2-0.4s
  - F1 Score: 0.95+ (P), 0.90+ (S) on STEAD
- **Real-time:** ✅ Yes (~50ms/trace on CPU, ~10ms on GPU)
- **Unique Features:**
  - Simple, well-tested architecture
  - Per-sample probabilities enable uncertainty estimation
  - Widely adopted, many pre-trained models available
- **Training Data:** STEAD, Instance, regional datasets
- **Integration Effort:** Medium (ONNX Runtime dependency)
- **Recommendation:** ✅ **Primary ML candidate** - good balance of accuracy, speed, simplicity

#### EQTransformer (Mousavi et al., 2020)
- **Architecture:** Attention-based encoder-decoder (Transformer + CNN)
- **Input:** 60s 3-component waveform
- **Output:** Detection probability, P/S probabilities, magnitude estimate
- **Accuracy:**
  - P-wave MAE: ~0.15-0.3s
  - S-wave MAE: ~0.3-0.6s
  - Additional: Magnitude estimation, uncertainty quantification
- **Real-time:** ✅ Yes (~100ms/trace on CPU, ~20ms on GPU)
- **Unique Features:**
  - Joint detection + picking + magnitude
  - Built-in uncertainty estimates
  - Attention maps provide interpretability
- **Training Data:** STEAD (global), can be fine-tuned regionally
- **Integration Effort:** Medium (ONNX Runtime dependency)
- **Recommendation:** ✅ **Secondary ML candidate** - more features but heavier

#### SegPhase (2025)
- **Architecture:** Hierarchical Vision Transformer (ViT) + CNN decoder
- **Input:** 30s 3-component waveform
- **Output:** Per-sample P/S/Noise probabilities
- **Accuracy:**
  - ~11% improvement in arrival time match rate vs PhaseNet
  - ~15% more events detected in continuous data
  - AUC = 1.0, MCC = 1.0 on STEAD benchmark
- **Real-time:** ~Near real-time (hierarchical design reduces ViT cost)
- **Unique Features:**
  - Best published accuracy
  - Explicit noise class
  - Dynamic attention for interpretability
  - Handles heterogeneous networks (100/250 Hz, varied components)
- **Integration Effort:** High (large model, ViT not yet optimized for edge)
- **Recommendation:** ⚠️ **Monitor** - promising but not yet proven in real-time ops

#### RED-PAN (Real-time Earthquake Detection and Phase-picking Attention Network)
- **Architecture:** Multi-task attention network
- **Input:** Continuous waveform stream
- **Output:** Detection trigger + P/S onset times
- **Accuracy:** Comparable to EQTransformer
- **Real-time:** ✅ Yes (designed for real-time)
- **Unique Features:** End-to-end detection + picking, attention-based
- **Integration Effort:** Medium
- **Recommendation:** ⚠️ Consider if RED-PAN model becomes publicly available

#### EdgePhase
- **Architecture:** Graph Neural Network
- **Input:** Multi-station waveforms (stations as graph nodes, distances as edges)
- **Output:** Network-consistent phase picks
- **Accuracy:** Superior to single-station methods in noisy conditions
- **Real-time:** ❌ No (requires station synchronization)
- **Unique Features:** Models inter-station spatial relations, resolves ambiguous picks
- **Integration Effort:** High
- **Recommendation:** ❌ **Future work** - complex deployment, requires architectural changes

#### CubeNet
- **Architecture:** 3D CNN (station × time × component)
- **Input:** Multi-station array data
- **Output:** Array-level phase detection
- **Accuracy:** Excellent for microseismic monitoring
- **Real-time:** ❌ No (array processing overhead)
- **Unique Features:** Leverages spatial and temporal correlations
- **Integration Effort:** High
- **Recommendation:** ❌ **Not suitable** for scmultipick's single-station design

---

### 3.3 Hybrid Approaches

#### STA/LTA Trigger + ML Refinement
- **Concept:** Use fast STA/LTA for detection, ML for precise onset + phase classification
- **Advantages:**
  - Maintains low false-negative rate of STA/LTA
  - Gains precision and phase discrimination from ML
  - ML only runs on triggered segments (reduces compute)
- **Integration Effort:** Medium
- **Recommendation:** ✅ **Recommended architecture** for Phase 4

#### STA/LTA + AR-AIC Refinement
- **Concept:** STA/LTA detects trigger, AR-AIC refines onset time
- **Advantages:**
  - No ML dependency
  - Proven in operational systems (scautopick)
  - Sub-sample precision
- **Integration Effort:** Low
- **Recommendation:** ✅ **Implement first** - immediate gains

---

## 4. Uncertainty Quantification

Modern pickers can provide uncertainty estimates, which improve location quality:

| Method | Uncertainty Source | Output |
|--------|-------------------|--------|
| PhaseNet | Probability threshold sensitivity | ± time window from probability width |
| EQTransformer | Monte Carlo Dropout | Predictive distribution (mean + std) |
| PoViT-UQ (2025) | MCD + Transformer | P-wave polarity + arrival time with confidence |
| AR-AIC | AIC curve curvature | Standard error from minimum curvature |

**Benefit for scpsloc:** Picks with uncertainty can be weighted in location, improving epicenter and depth resolution. Outlier picks can be rejected automatically.

---

## 5. Implementation Roadmap

### Phase 1: AR-AIC Repicker (Weeks 1-2)
**Scope:** Add AR-AIC onset refinement to existing STA/LTA triggers
- [ ] Implement AR-AIC picker as new picker type (`pickerType = "araic"`)
- [ ] Run AR-AIC on triggered waveform segment (pre- and post-trigger samples)
- [ ] Replace trigger time with AR-AIC refined onset
- [ ] Compute pick uncertainty from AIC curve
- [ ] Add uncertainty to pick output (comment or time uncertainty field)

**Dependencies:** None (pure C++ implementation)
**Risk:** Low
**Expected Benefit:** 30-50% improvement in onset precision

### Phase 2: ML Picker Plugin via ONNX Runtime (Weeks 3-6)
**Scope:** Integrate PhaseNet and EQTensor as optional picker types
- [ ] Add ONNX Runtime as optional dependency (CMake option)
- [ ] Create ML picker base class inheriting from Processing::Picker
- [ ] Implement PhaseNet picker (`pickerType = "ml:phasenet"`)
- [ ] Implement EQTransformer picker (`pickerType = "ml:eqtransformer"`)
- [ ] Add model path configuration (`ml.modelPath = /path/to/model.onnx`)
- [ ] Add probability threshold configuration (`ml.probThreshold = 0.3`)
- [ ] Output per-sample probabilities as pick comments
- [ ] Handle batching for efficiency (process multiple stations together)

**Dependencies:** ONNX Runtime C++ API
**Risk:** Medium (external dependency, model management)
**Expected Benefit:** 15-30% more picks detected, especially low-magnitude events

### Phase 3: Uncertainty Quantification (Weeks 7-8)
**Scope:** Propagate pick uncertainties to location
- [ ] Add pick time uncertainty field to output
- [ ] Implement probability-width based uncertainty for ML picks
- [ ] Implement AIC-curve based uncertainty for AR-AIC picks
- [ ] Update scpsloc to use pick uncertainties as weights (optional)

**Dependencies:** Phase 1 or Phase 2
**Risk:** Low
**Expected Benefit:** Improved location quality, automatic outlier rejection

### Phase 4: Hybrid Detection (Weeks 9-12)
**Scope:** Combine STA/LTA detection with ML refinement
- [ ] Configure STA/LTA as trigger detector (low threshold for high recall)
- [ ] Configure ML picker as onset refiner (runs on triggered segments only)
- [ ] Add hybrid picker type (`pickerType = "hybrid:stalta+phasenet"`)
- [ ] Optimize batching: collect triggered segments, run ML in batch
- [ ] Add fallback: if ML fails, use STA/LTA trigger time

**Dependencies:** Phase 1 + Phase 2
**Risk:** Medium
**Expected Benefit:** Best of both worlds: speed + accuracy + robustness

### Phase 5: Multi-Station Coherence (Future / Research)
**Scope:** EdgePhase-style network-consistent picking
- [ ] Evaluate if multi-station coherence improves picks in current network
- [ ] Prototype graph-based pick validation
- [ ] Consider integration if benefit justifies complexity

**Dependencies:** Significant architectural changes
**Risk:** High
**Expected Benefit:** Improved picks in noisy conditions, reduced false detections

---

## 6. What NOT to Implement (Yet)

### SegPhase
- **Reason:** Hierarchical ViT is computationally expensive; not yet proven in real-time operational settings
- **Monitor:** If real-time latency benchmarks become available and show <100ms/trace, reconsider

### EdgePhase / CubeNet
- **Reason:** Require multi-station synchronization and array processing, which contradicts scmultipick's single-station design
- **Monitor:** If network-wide picking becomes a priority, these could be implemented as a separate module

### Full ML Replacement
- **Reason:** STA/LTA is faster and more robust for detection; ML excels at refinement, not replacement
- **Approach:** Use ML as supplement, not replacement

---

## 7. Technical Considerations

### ONNX Runtime Integration
- **Package:** `onnxruntime` (C++ API)
- **Installation:** `apt install libonnxruntime-dev` or build from source
- **Models:** Export PhaseNet/EQTransformer from PyTorch to ONNX format
- **Threading:** ONNX Runtime supports intra-op parallelism; configure for multi-core
- **GPU:** Optional CUDA support for acceleration (not required for CPU inference)

### Model Management
- Pre-trained models should be distributed with scmultipick or downloadable
- Models should be versioned and validated against local network data
- Configuration should specify model path: `ml.modelPath = @DATADIR@/scmultipick/phasenet.onnx`

### Backward Compatibility
- New picker types should be opt-in (default remains `recstalt`)
- Existing configurations should work without changes
- ML picker should gracefully fall back to STA/LTA if ONNX Runtime not available

### Testing
- Unit tests with synthetic waveforms
- Integration tests with real event data
- Benchmark against manual picks for accuracy
- Performance benchmarks for real-time latency

---

## 8. References

1. Zhu, W., & Beroza, G. C. (2019). PhaseNet: A deep-neural-network-based seismic arrival-time picking method. *Geophysical Journal International*, 216(1), 261-273.
2. Mousavi, S. M., Ellsworth, W. L., Zhu, W., Chuang, L. Y., & Beroza, G. C. (2020). Earthquake transformer—an attentive deep-learning model for simultaneous earthquake detection and phase picking. *Nature Communications*, 11(1), 3952.
3. SegPhase authors (2025). SegPhase: development of arrival time picking models for Japan's dense seismic networks. *Earth, Planets and Space*, 77, 123.
4. PoViT-UQ authors (2025). P-wave polarity and arrival time determination using Transformer with uncertainty quantification. *Geophysical Journal International*, 243(1).
5. SeisBench documentation: https://github.com/seisbench/seisbench
6. ONNX Runtime: https://onnxruntime.ai/

---

## 9. Conclusion

The most practical path forward for scmultipick is a **phased approach** starting with AR-AIC refinement (low risk, immediate benefit), followed by ML picker integration via ONNX Runtime (medium risk, significant accuracy improvement). This maintains backward compatibility while adding state-of-the-art picking capabilities. The hybrid approach (STA/LTA trigger + ML refinement) offers the best balance of speed, accuracy, and robustness for operational deployment.
