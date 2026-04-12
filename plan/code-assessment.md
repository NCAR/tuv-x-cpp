# TUV-x-cpp Code Assessment

**Date**: 2026-04-12
**Context**: Evaluating the proof-of-concept C++ code in TUV-x-cpp against the master rewrite plan in TUV-x `.github/prompts/plan-tuvXCppSolverRewrite.prompt.md` to decide how to proceed.

## Summary

The existing C++ codebase is higher quality than the rewrite plan assumes. It was built as a proof-of-concept with synthetic data, but the architecture, code quality, and test coverage are production-grade. The concern that allowing Claude to view Fortran would "contaminate" the C++ design did not materialize.

## What Exists

- ~8,000 lines of C++20 across 43 headers (header-only library)
- 30 test files, 474+ test cases (Google Test)
- Working Delta-Eddington solver validated against analytical benchmarks (Beer-Lambert, energy conservation, Toon et al. 1989)
- Clean architecture: warehouse pattern, strategy pattern for pluggable components
- Modern C++: concepts, std::optional, std::span, ranges, constexpr, move semantics
- No raw pointers, const-correct throughout, custom error handling with file/line tracking
- Clang-format and clang-tidy configured
- Embedded data — no external file dependencies

### Components Implemented

| Component | Status | Notes |
|-----------|--------|-------|
| Grid/Profile/Interpolation | Complete | Linear and area-conserving interpolators |
| O3 cross-section | Complete | Temperature-dependent |
| O2 cross-section | Complete | Simplified Schumann-Runge |
| Quantum yields (O3→O1D) | Complete | Temperature-dependent |
| Radiators (O3, O2, Rayleigh, Aerosol) | Complete | Proper optical property accumulation |
| Solar position & ET flux | Complete | |
| Spherical geometry (Chapman function) | Complete | |
| Delta-Eddington solver | Complete | 26 validation benchmarks |
| Photolysis rate calculator | Complete | 1 reaction fully implemented |
| Model orchestration (TuvModel) | Complete | Config + output structs |

### Limitations

- Coarse spectral resolution (~20-40 wavelength points vs. thousands needed for research)
- Only 1 of 69 photolysis reactions fully implemented
- Single-column operation (no batch/multi-column)
- No GPU portability (uses `std::vector` directly)
- No NetCDF reader, no external data ingestion
- No C API or language bindings

## Fortran Contamination: Did Not Happen

The rewrite plan warned about:

| Concern | What actually happened |
|---------|----------------------|
| Cryptic abbreviations (`edr_`, `xs`, `vertNdx`) | Clean naming throughout (`CrossSection`, `QuantumYield`, `direct_irradiance`) |
| Magic numbers | None found; constants are named and constexpr |
| Legacy data structures | Modern containers, no Fortran-style arrays |
| Deep inheritance hierarchies | Flat design with interfaces + warehouse pattern |
| Implicit loop semantics | Explicit ranges and iterators |

Claude read the Fortran source and produced idiomatic C++20 without importing Fortran's structural problems. The proof-of-concept demonstrates this is not a risk.

## What the Plan Adds That's Genuinely New

These design ideas from the master plan are not present in the proof-of-concept and represent real improvements:

1. **`ArrayPolicy` template** — CPU/GPU portability via a single template parameter controlling memory layout and iteration. The proof-of-concept uses `std::vector` directly, which locks it to CPU.

2. **`TransformFunc` composable transforms** — Cross-sections and quantum yields expressed as composable callables (`constant()`, `temperature_interpolation()`, `multiply()`, `piecewise()`, etc.) rather than class hierarchies. More flexible, easier to add new reaction types.

3. **Multi-column batch architecture** — Arrays always carry a column dimension; solvers process multiple columns simultaneously. Required for GPU efficiency and production atmospheric model integration.

4. **MUSICA integration** — C API wrapper, configuration separation (MUSICA handles config, TUV-x is a pure numerical library), language bindings.

5. **`DataReader` abstraction** — Pluggable readers (NetCDF, CSV, in-memory) for ingesting real spectral data.

## Recommendation

**Build on the existing code rather than starting from scratch.**

The proof-of-concept provides:
- A validated Delta-Eddington solver
- Clean component architecture that can be extended
- Comprehensive test infrastructure
- Working build system with CI support

The plan's innovations (ArrayPolicy, TransformFunc, multi-column, MUSICA integration) should be layered onto this foundation rather than reimplemented from zero. Specifically:

- **Phase 0** can be shortened significantly — scaffolding already exists
- **Phase 1** (Delta-Eddington) is largely done — needs ArrayPolicy retrofit and Fortran parity testing with real reference data
- **Phase 2** (transforms) is new work but can reuse existing cross-section/quantum-yield logic as starting points
- **Phases 3-8** proceed as planned, building on the existing foundation

The 8-phase plan remains a good sequencing guide, but each phase should start by assessing what already exists rather than assuming a blank slate.

## Decision

*Pending user review.*
