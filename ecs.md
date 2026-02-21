# ECS Exploratory Findings for OPF-like Projects

This document presents recommendations for applying an Entity–Component–System (ECS) pattern to an OPF-style codebase. It focuses on identifying likely Entities, a component taxonomy (derived from a base `IComponent`), suggested Systems, ownership/storage, and practical migration notes.

**Goal**: separate identity/type (Entity) from data (Components) and logic (Systems) so we can reuse data, compose behavior, and simplify testing.

---

## 1. Overview

- Entity: a lightweight ID or typed tag that represents a conceptual object (no heavy data or behavior). Entities express what something is (e.g., `Dataset`, `Sample`, `Model`) but not how it works.
- Component (`IComponent`): small, POD-like data containers. Components can be composed, nested, or reused across entity types. They implement no heavy behavior beyond trivial helpers.
- System: encapsulates behavior over sets of entities which possess required components. Systems query entities by component presence (or entity type) and perform logic.

---

## 2. Candidate Entities (types)

Entities should represent high-level domain concepts in the OPF/ML domain. Examples:

- `Dataset` (type tag): logical collection of samples / metadata
- `Sample` (type tag): a single data sample / node in a graph
- `Model` / `Classifier` (type tag): learned classifier or OPF model artifact
- `DistanceMatrix` or `DistanceJob` (type tag): represents results of distance computations
- `Subgraph` / `Partition` (type tag): training/evaluating/testing split
- `Experiment` or `Run` (type tag): groups inputs, parameters, outputs for a test/benchmark

Notes:
- Entities are usually implemented as an ID plus an (optional) runtime type enum/tag. They don't own data fields themselves beyond a tiny descriptor.
- Use an entity registry that maps EntityId -> set of Components and a `type` property for quick filtering.

---

## 3. Component taxonomy (derived from `IComponent`)

Design each component as a small struct derived from an abstract `IComponent` interface (primarily to identify component type and support RTTI/registration). Prefer composition over deep class hierarchies.

Suggested components:

- `CFeatures` — vector/array of floats/doubles for a sample's features
- `CLabel` — integer/string class label
- `CMeta` — sample metadata (id, source-file, original-index)
- `CGraphNode` — graph pointers/indices (neighbours, weights) or index into adjacency storage
- `CDistanceParams` — parameters for distance calculation (type id, k, etc.)
- `CDistanceResult` — binary/serialized distances or index into a shared distance storage
- `CModelParams` — learned model parameters or pointer/handle to model storage
- `CEvalMetrics` — accuracy, timing, other metrics
- `CIOPath` — file paths for I/O (source, destination, model file)
- `CTemporal` — timestamps, creation/modification times
- `CFlags` — boolean flags like `isTraining`, `isEvaluated` for quick filtering

Component composition ideas:
- `CFeatures` + `CLabel` -> typical sample
- `CFeatures` + `CGraphNode` -> graph-enabled sample
- `CModelParams` + `CEvalMetrics` -> trained model entity

Reuse: implement small shared components (e.g., `CMeta`, `CIOPath`) and reference them from many entity types.

---

## 4. Systems and responsibilities

Systems encapsulate behavior and operate on entities matching component queries.

Examples:

- `DistanceSystem` — for entities with `CFeatures` and `CDistanceParams`; produces `CDistanceResult` (or writes into shared distance storage component)
- `SplitSystem` — for `Dataset` entities: creates/updates `Subgraph` entities and assigns `CFlags` (training/testing/eval)
- `TrainSystem` — consumes `Subgraph` + `CFeatures` + `CLabel` components to produce `CModelParams` and `CEvalMetrics`
- `ClassifySystem` — applies `CModelParams` to `CFeatures` to produce predictions; writes results into `CEvalMetrics` or `CFlags`
- `IOSystem` — serializes/deserializes entities/components (`CIOPath`) to/from disk
- `EvalSystem` — computes accuracy / statistics for entities with predicted labels and `CLabel`
- `CleanupSystem` — removes temporary components or files (fits your teardown requirement)

Systems should be small, composable, and testable. Avoid putting long-lived state in systems; prefer component storage for state.

---

## 5. Entity -> Component mapping (practical examples)

- `Sample` entity: `CFeatures`, `CLabel`, `CMeta`, optional `CGraphNode`
- `Dataset` entity: `CIOPath`, `CTemporal`, and references/IDs for constituent `Sample` entities
- `Subgraph` entity: references to `Sample` entities (or a component storing an index range), `CFlags` telling which partition it is
- `Model` entity: `CModelParams`, `CEvalMetrics`, `CIOPath`
- `DistanceJob` entity: `CDistanceParams`, `CDistanceResult`

---

## 6. Storage and ownership

- Components can be stored in contiguous arrays (SoA) keyed by component type for cache-friendly access. Alternatively start with a map-based approach (EntityId -> vector<IComponent*>) for prototyping.
- Shared heavyweight data (e.g., distance matrices) should be stored in dedicated storage services and referenced by components (e.g., `CDistanceResult` holds an index/handle into a `DistanceStorage`).
- Lifetime: Entities live in an `EntityRegistry`. Components may be attached/detached by systems; ensure safe iteration (deferred removal pattern).

---

## 7. Runtime type checks and system selection

- Each entity may carry a small `EntityType` enum (e.g., `Sample`, `Dataset`, `Model`, `Subgraph`) to allow coarse-grain selection.
- Systems should select entities through component presence queries primarily. Use the `EntityType` only for higher-level selection when needed.
- Use component-type identifiers (integer IDs or C++ type_index) to perform fast lookups.

---

## 8. Component inheritance vs composition

- Prefer composition: components are POD-like and independent.
- If you need component families, prefer composition by embedding shared small components inside larger ones rather than inheriting from many concrete component classes.
- `IComponent` is mainly for RTTI and polymorphic container handling, not rich behavior.

---

## 9. Migration notes for OPFpp (practical incremental plan)

1. Start by introducing `IComponent`, `Entity` (ID + type), and `EntityRegistry` APIs in a new `ecs/` module.
2. Implement `CIOPath`, `CFeatures`, `CLabel`, and `CFlags` as first components.
3. Refactor a simple test/utility (e.g., `opf_split`) to be expressed as a `SplitSystem` operating on a `Dataset` entity.
4. Gradually refactor library functions: move data out of classes into components, keep current API working by providing adapters (wrappers that populate components and call systems).
5. Replace file-based integration tests with ECS-driven tests that assemble entities + components and run systems.

Benefits: reduce coupling, easier testing, reuse components across systems.

---

## 10. Example `IComponent` sketch (C++ implementation — DONE)

The ECS runtime has been scaffolded with the following structure:

**Headers under `include_cpp/ecs/`:**

- `IComponent.h` — Base interface for all components
- `Entity.h` — `EntityId` type, `EntityType` enum, and `Entity` struct
- `EntityRegistry.h` — Type-erased component storage and entity querying
- `System.h` — Base `ISystem` interface
- `Components.hpp` — Common components (`CFeatures`, `CLabel`, `CIOPath`, `CFlags`) with constructors

**Key features:**
- Template-based component storage (map-based for simplicity)
- `registry.create(type)`, `registry.addComponent<T>()`, `registry.getComponent<T>()`, `registry.view<T1, T2, ...>()` API
- Component constructors for easy initialization:
  - `CFeatures`: move/copy constructors from `std::vector<float>`
  - `CLabel`: explicit constructor from `int`
  - `CIOPath`: move/copy constructors from `std::string`
  - `CFlags`: parameterized constructor for flags
- Example usage in `src_cpp/ecs_example.cpp` and comprehensive tests in `tests_cpp/test_ecs_components.cpp`

**Build:**
```cpp
// Example: create and populate an entity
auto sample = registry.create(ecs::EntityType::Sample);
registry.addComponent<ecs::CFeatures>(sample.id, ecs::CFeatures({1.0f, 2.0f, 3.0f}));
registry.addComponent<ecs::CLabel>(sample.id, ecs::CLabel(42));

// Query: find all entities with both features and label
auto results = registry.view<ecs::CFeatures, ecs::CLabel>();
for (auto id : results) {
    auto *feat = registry.getComponent<ecs::CFeatures>(id);
    auto *lbl = registry.getComponent<ecs::CLabel>(id);
    // process...
}
```


---

## 11. Testing & teardown (tie back to your change)

- Implement a `CleanupSystem` that removes temporary files and components; run it in test teardown.
- Because components like `CIOPath` store file names, the `CleanupSystem` can look for `classifier.opf`, `training.dat`, `testing.dat`, etc., and remove them via a `RemoveFile` utility component or service.

---

## 12. Risks & caveats

- Large refactors: converting an existing codebase to ECS is non-trivial. Use incremental steps and adapters to avoid breaking everything at once.
- Performance: naive map-based ECS is fine for prototyping; for production consider SoA layouts and efficient iteration.
- Ownership confusion: clearly document whether components own memory (vectors) or hold handles.

---

## 13. Next steps (implementation roadmap)

### ✅ Completed
1. Scaffolded minimal ECS runtime (headers + components)
2. Implemented `EntityRegistry` with component storage and querying
3. Created common components (`CFeatures`, `CLabel`, `CIOPath`, `CFlags`) with constructors
4. Added `ecs_example.cpp` demonstration
5. Added comprehensive unit tests (`test_ecs_components.cpp`)
6. **Phase 1.1: SplitSystem** — implemented with `CSamples`, `CSubgraphRange`, `CSplitParams` components
   - Creates training/eval/testing subgraph entities from dataset
   - Comprehensive tests in `test_ecs_split_system.cpp`
7. **Phase 1.2: IOSystem** — implemented with serialization/deserialization
   - Save/load single entities to/from text files
   - Save multiple entities to collection files
   - Support for all component types (CFeatures, CLabel, CFlags, CSamples, CSubgraphRange, CSplitParams, CIOPath)
   - Comprehensive tests in `test_ecs_io_system.cpp`

### 🔄 Next priorities (in order)

**Phase 1: Core Systems (high impact, relatively low effort)** — ✅ PHASE 1 COMPLETE
1. ✅ **SplitSystem** — creates Subgraph entities from Dataset
2. ✅ **IOSystem** — serializes/deserializes entities to text files

**Phase 2: Refactor existing OPF operations (medium effort, medium impact)** — IN PROGRESS
3. 🔄 **Phase 2.1: TrainSystem** — wrap `opf_train` logic (IMPLEMENTATION COMPLETE, TESTS INCOMPLETE)
   - ✅ Created `CModelParams` component: stores prototypes, path values, predecessors, ordered nodes, labels
   - ✅ Created `CEvalMetrics` component: tracks training statistics, accuracy, num_prototypes
   - ✅ Implemented `TrainSystem` header with simplified IFT (Image Foresting Transform) algorithm
   - ⚠️ Created 18 test cases BUT tests do NOT verify internal `train_entity()` dispatcher logic
     - Tests verify end results through public `update()` API (component creation, label propagation, metrics recording)
     - **Tests don't verify dispatcher**: Cannot directly test private `train_entity()` method
     - **Cannot independently test routing logic**: Would need to refactor method accessibility or use friend classes/functions
     - **Tests only verify final output**: Single-sample vs multi-sample behavior is indirectly validated through results
     - **Assessment**: Tests are useful for integration/end-to-end validation BUT not for unit-testing dispatcher mechanism
   - ✅ **All 18 tests passing** (0.28s execution time)
   - 🟡 **Tests are adequate for functionality** but cannot directly validate dispatcher routing
4. 🔴 **Phase 2.2: ClassifySystem** — wrap `opf_classify` logic (IMPLEMENTATION INCOMPLETE)
   - ✅ Created `ClassifySystem.hpp` implementing OPF classification algorithm structure
   - ✅ Implements cost-based routing: `cost = max(pathval, distance)`
   - ✅ Early termination optimization when `min_cost <= next pathval`
   - ✅ Created 16 comprehensive test cases
   - 🔴 **CRITICAL ISSUE**: Distance computation not implemented
     - `model->prototypes` stores indices, not feature vectors
     - Cannot compute distance without access to original training features
     - Current implementation uses `weight = 0.0f` (placeholder)
   - **9 tests passing, 7 failing** due to missing distance computation
   - **Next step**: Need to store prototype feature vectors in `CModelParams` OR maintain reference to training data
5. **Phase 2.3: AccuracySystem** — compute metrics
   - Input: Predicted labels + `CLabel` components
   - Output: `CEvalMetrics` with accuracy values

**Phase 3: Integration & Migration (higher effort, long-term)**
6. **Create adapters** — wrap existing OPF API functions to use ECS internally
   - Old: `opf_split(dataset_path, ...)`
   - New: Create entities, populate with components, run `SplitSystem`, extract results
   - Benefit: Gradual migration without breaking existing code
7. **Refactor tests** — replace file-based tests with ECS-driven tests
   - Instead of: `opf_split_run()` → check files
   - New: Create `Dataset` entity, run `SplitSystem`, query resulting `Subgraph` entities
   - Tie-in: `CleanupSystem` removes temp components/files at test teardown
8. **Optimize storage** — migrate from map-based to SoA (Structure of Arrays) if performance is a concern

### Recommendation: Start with Phase 1, Step 1 (SplitSystem)

**Why?**
- `opf_split` is relatively self-contained and well-understood
- Easy to test without heavy ML dependencies
- Demonstrates the full pattern (entity creation, component assignment, system query)
- Quick win that validates the approach before larger refactors

**Sketch:**
```cpp
class SplitSystem : public ISystem {
    void update(EntityRegistry &registry, double dt) override {
        // 1. Find Dataset entities with CIOPath
        // 2. Load data from path
        // 3. Split into train/eval/test subsets
        // 4. Create Subgraph entities and assign CFlags
        // 5. Store sample indices in a new CSampleRange or CSubgraphData component
    }
};
```

---

Generated at: automated exploratory note with implementation updates


