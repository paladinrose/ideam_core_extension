# Ideam Core Extension: Architecture Baseline

## Intro: Core Tenets & Toolchain

This document serves as the baseline context for understanding the `ideam_core_extension` architecture. When working within this codebase, AI assistants and developers must adhere to the following technical constraints and design philosophies:

* **Godot Version:** 4.5 (utilizing latest `godot-cpp` bindings and GDExtension interface changes).
* **Language:** C++26. Code should leverage modern features (e.g., `<concepts>`, `<span>`, advanced atomics) and compile cleanly under `-std=c++2b` or `-std=c++26` flags.
* **Build System:** SCons.
* **Design Philosophy:** The core memory and execution pipelines lean heavily into **Data-Oriented Design (DOD)** to maximize cache locality, parallel throughput, and GPU synchronization. However, this is *not* a strict, dogmatic DOD framework. Standard OOP concepts and Godot's node/resource paradigms are utilized where they make sense—particularly at the engine boundary layer, in UI components, and in domain-specific plugins that do not require batch-processed memory layouts.

---

## 1: Memory Primitives and Layout (The Bedrock)

**Target Directory:** `src/core/memory/`

The foundation of the Ideam Core maps raw memory allocation and access patterns. Without understanding how data is structured in memory, any subsequent logic will be misinterpreted. The core rule here is that data is contiguous, cleanly separated from execution logic, and safely manipulated via atomic concurrency and strict access claims.

### Key Components

#### 1. `MemoryManagerDOD`
The centralized authority for POD memory and GPU synchronization. It bypasses standard Godot memory allocation overhead for intensive data by managing its own massive, contiguous blocks.
* **Master Block:** A single `uint8_t* master_block_ptr` stores all persistent data.
* **Transient Arena:** A lock-free, atomic bump allocator (`transient_block_ptr`) used for high-speed, temporary workspace memory during wave execution.
* **Versioning:** Uses a `global_version` atomic counter. Any structural reallocation increments this, instantly invalidating stale pointers framework-wide.

#### 2. `MemoryBufferPOD`
Represents a structural slice of the Master Block. It defines *how* the memory is laid out and navigated. Buffer metadata is stored in flat, pre-reserved `std::vector`s inside the Manager to prevent pointer invalidation.

```cpp
// memory_buffer_pod.h
enum class BufferLayoutType : uint16_t {
    NONE       = 0,
    FLAT       = 1 << 0, 
    AOS        = 1 << 1,
    SOA        = 1 << 2,
    SPARSE_SET = 1 << 3,
    TILED_SOA  = 1 << 4,
    RING       = 1 << 5,
    PAGED      = 1 << 6,

    // --- UI/Graph Helper Masks ---
    ANY_LINEAR   = FLAT | AOS | SOA | SPARSE_SET | TILED_SOA | RING | PAGED,
    ANY_PARALLEL = SOA | TILED_SOA,
    ANY_SPATIAL  = FLAT | SOA | TILED_SOA | PAGED,

    // The Universal Mask: Matches any valid layout defined above.
    ANY          = ANY_LINEAR 
};
```

#### 3. `MemoryGrantPOD` & `GrantPartPOD`
To prevent data races in a highly parallel environment, tasks cannot access memory directly. They must bake a `MemoryGrant`. 
* Grants are strictly sized to fit perfectly into hardware cache lines (e.g., exactly 640 bytes or 1152 bytes) to prevent false sharing across threads.
* A `GrantPartPOD` holds an absolute pointer (`raw_base_ptr`) and an integrated `MemoryBufferSelectionPOD`, validating against the manager's global version to ensure memory hasn't shifted.

```cpp
// memory_grant_pod.h
struct GrantPartPOD {
    // --- 8-Byte Alignment Block ---
    uint8_t* raw_base_ptr = nullptr;      // Absolute start (Master + Offset)
    MemoryBufferSelectionPOD selection;   // Unified DOD Selection (104 bytes)

    // --- 4-Byte Alignment Block ---
    uint32_t buffer_id = 0xFFFFFFFF;
    uint32_t buffer_version_at_issue = 0;
    uint32_t element_stride = 0;          // Size of element (AoS)
    uint32_t column_id = 0;               // Target column (SoA)
    // ...
};
```

#### 4. `SelectionUtils`
Low-level, stateless bit manipulation utilities that operate on the `MemoryBufferSelectionPOD`. 

In a highly parallel Data-Oriented environment, you frequently need to filter data (e.g., "get all entities with health > 0"). `SelectionUtils` handles converting these subsets of queried data between **Dense** (packed arrays) and **Sparse** (bitset) modes *in-place*. This ensures zero heap allocations happen during the hot path of the execution loop, maintaining cache integrity.

```cpp
// selection_utils.h
struct SelectionUtils {
    /**
     * convert_to_sparse
     * Transitions a selection from Dense to Sparse mode IN-PLACE.
     * Rebuilds the packed index array from the active bits.
     */
    static void convert_to_sparse(MemoryBufferSelectionPOD& r_selection) {
        if (r_selection.mode == SelectionMode::SPARSE) return;

        uint64_t* bitset = r_selection.data.bitset;
        int64_t* indices = reinterpret_cast<int64_t*>(r_selection.data.bitset);
        
        int64_t write_ptr = 0;
        const int64_t capacity = r_selection.capacity;

        // Iterate through the bitset and pack the active indices.
        for (int64_t i = 0; i < capacity; ++i) {
            if (bitset[i >> 6] & (1ULL << (i & 63))) {
                indices[write_ptr++] = i;
            }
        }
        
        // Zero out the remaining bytes to prevent ghost data
        const size_t bytes_to_clear = (capacity - write_ptr) * sizeof(int64_t);
        if (bytes_to_clear > 0) {
            std::memset(&indices[write_ptr], 0, bytes_to_clear);
        }

        r_selection.data.active_count = write_ptr;
        r_selection.mode = SelectionMode::SPARSE;
    }
};
```

#### 5. How Data is Accessed (The View Layer)

Because the `MemoryManagerDOD` stores data as raw `uint8_t*` blocks according to a `BufferLayoutType`, execution logic cannot (and should not) attempt to cast or read these pointers directly. 

Instead, tasks interact with memory exclusively through **Memory Views**. Views are zero-overhead, templated structural lenses that wrap a `MemoryGrant`. They enforce the access pattern, provide the mathematical stride calculations, and completely abstract away the underlying memory layout from the business logic.

This enforces the core rule: **Logic knows *what* it is doing, the View knows *where* the data is.**

#### 6. ViewTraits: Compile-Time Dispatch & UX Matching
Views aren't just wrappers; they possess compile-time metadata defined in `view_traits.h`. The `ViewCapability` bitmask is crucial for two reasons:
1. **Compile-Time Instructing:** High-level execution logic (`T_Logic` dispatchers) can use `static_assert` or SFINAE to ensure they are only handed views capable of doing what they need (e.g., a spatial convolution kernel refusing to compile if handed a view without `SPATIAL_ACCESS`).
2. **UX Type Matching:** At the engine boundary layer, Godot needs to know how to draw inspectors for these raw memory blocks. By querying the `ViewCapability`, the UI knows whether to render a flat list, a 2D grid, or a masked SIMD lane.

```cpp
// view_traits.h
enum class ViewCapability : uint32_t {
    NONE                   = 0,
    LINEAR_ACCESS          = 1 << 0,  // Supports operator[] selection-relative
    SPATIAL_ACCESS         = 1 << 1,  // Supports multidimensional indexing
    SIMD_ACCESS            = 1 << 2,  // Supports get_lane / LaneWidth
    RANDOM_ACCESS          = 1 << 3,  // Supports arbitrary ID lookups
    VIRTUAL_MEMORY         = 1 << 4,  // Paged/Indirect (Strategy-dependent)
    QUEUE_ACCESS           = 1 << 5,  // Supports stateful consumption (pop)
    STENCIL_ACCESS         = 1 << 6,  // Supports center() and neighbor(k)
    SWAP_ACCESS            = 1 << 7,  // Supports Temporal Ping-Pong read/write proxies
    ENTITY_ID_ACCESS       = 1 << 8,  // Supports get_entity_at() for Sparse ECS bridging
    MULTI_COMPONENT_ACCESS = 1 << 9,  // Supports pluck<T>() for heterogeneous payloads
    ATOMIC_ACCESS          = 1 << 10  // Supports std::atomic_ref returns
};
```

#### 7. Memory Strategies: The Addressing Engine
The true power of the View layer comes from its pairing with **Memory Strategies** (`strategies.h`). 

A View dictates the *iteration and concurrency rules* (e.g., "I am reading a single element", or "I am iterating over a sparse set"). The Strategy dictates the *pointer arithmetic* (e.g., "This memory is a Flat Array", or "This memory is a 2D Spatial Grid"). 

By decoupling these, any view can act on any structural layout just by changing a template argument. Furthermore, because Strategies are stateless `struct`s, they utilize the `[[no_unique_address]]` attribute. This means passing a strategy into a view adds **zero bytes** of overhead to the struct size.

```cpp
// C++20 Concept ensuring a valid Strategy is passed to a View
template <typename T>
concept IsMemoryStrategy = requires {
    { T::is_spatial } -> std::convertible_to<bool>;
};
```

#### 8. Core View Archetypes

The framework provides several highly specialized lenses for reading the Master Block:

* **`AOSOAView` (Array of Structures of Arrays)**
  The primary workhorse for highly parallel, cache-friendly data processing. Optimized for SIMD kernels.
* **`SparseSetView`**
  The ECS-style view. Provides O(1) random access ID lookups alongside dense, cache-friendly O(N) iteration, using the indices packed by `SelectionUtils`.
* **`SwapView`**
  A zero-overhead C++26 abstraction for Ping-Pong buffers (`current_state` and `next_state`). Crucial for cellular automata, physics integration, or any simulation where writes cannot immediately overwrite reads during a tick. It resolves `current` and `next` pointers simultaneously.
* **`BridgeView`**
  A hierarchical structural adapter. It maps a Parent selection to a subdivided Child buffer (e.g., resolving a parent Quadtree node to its 4 child indices).
* **`RingView` & `PagedView`**
  Views for streaming circular buffers (Event Queues) and virtualized memory (Paged Trees), abstracting away head/tail wrapping and virtual page shifts.
* **`AtomicView`**
  Returns elements wrapped in `std::atomic_ref<T>`. Used for lock-free parallel accumulation when multiple task workers might write to the same target index.

*(Note on C++26 Optimizations: Almost all Views heavily utilize the `[[assume(selection.is_selected(flat_idx))]]` attribute. Because the `MemoryGrant` and `SelectionUtils` pre-validate memory bounds during the Graph compilation phase, we can instruct the compiler's optimizer to completely strip out bounds-checking branches during the hot loop, unleashing massive auto-vectorization gains).*


## 2: Execution Logic and Task Topology

**Target Directory:** `src/core/tasks/`

Once the memory layout is anchored (Phase 1), the instruction set architecture for mutating that memory must be established. The core rule of the Execution layer is that **operations are batched, isolated functions executed against queried subsets of memory**, rather than methods called on individual objects.

### Key Components

#### 1. The Execution Engine (`TaskGraphDOD` & `INativeTask`)
The `TaskGraphDOD` is the execution orchestrator. It compiles the topological dependencies of tasks, allocates transient lock-free workspaces, and dispatches execution in parallel "Waves".

Instead of passing complex objects around, tasks are handed a lightweight `TaskContextPOD`. This context contains the simulation delta, raw pointers to the locked `MemoryGrant`, and access to the Command Buffers.

#### 2. The Command System (Deferred Mutation)
Because the `MemoryManagerDOD` relies on strict contiguous arrays and sparse sets, tasks cannot safely add or remove elements in the middle of a highly parallel hot-loop without causing data races or triggering a global reallocation. 

To solve this, mutations are deferred via lock-free Command Arenas:
* **Tier 2 (Wave Commands):** `TaskSelectionCommandPOD` is a thread-local, 32-byte bump buffer. During a Wave, tasks like Queries use this to queue "Add" operations (e.g., waking up an entity). The graph resolves these immediately after the Wave completes.
* **Tier 1 (Graph Commands):** `TaskGraphCommandPOD` defers physical extensions (e.g., expanding the master block, spawning new entities) until the very end of the entire graph cycle.

#### 3. The `T_Logic` Paradigm and Compile-Time Firewalls
To prevent developers from drowning in DOD boilerplate (resolving pointers, validating selection bitsets, securing grants), the framework splits tasks into two parts:
1. **The Wrapper:** (e.g., `TransformTask<T_Logic>`, `QueryTask<T_Logic>`) Handles the graph interface, secures the memory leases, and builds the Views.
2. **The Payload:** (e.g., `EulerIntegrationTransformLogic`) A trivially copyable struct containing purely the mathematical or logical operation.

**The Compile-Time Firewall:**
Using C++20 Concepts and `LogicTraits`, the wrappers enforce hardware requirements at compile-time. If a `T_Logic` dictates `TransformRequirement::REQUIRES_SPATIAL`, and the user attempts to instantiate it with a flat `SingleElementView`, the compiler will halt.

#### 4. Logic Sub-Systems

Operations are divided into distinct groups based on their mutation privileges:

##### A. Transforms
Transforms are pure mathematical operations or reductions. They read data and mutate data, but they **cannot** alter the selection bitmask. 
* *Examples:* `EulerIntegrationTransformLogic`, `FastNoiseLiteTransformLogic`.

```cpp
// Example: Euler Integration (Pure Math, zero virtual overhead)
struct alignas(64) EulerIntegrationTransformLogic {
    using ValueType       = godot::Vector3;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<ValueType, DefaultStrategy>; // Reads T, Writes T+1

    static constexpr TransformRequirement requirements = TransformRequirement::NONE;
    uint32_t position_buffer_id = INVALID_ID;
    uint32_t velocity_buffer_id = INVALID_ID;
    float time_scale = 1.0f;

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD& context, T_View& pos_view) const {
        const GrantPartPOD* vel_part = context.get_grant_part(velocity_buffer_id);
        const ValueType* velocities = reinterpret_cast<const ValueType*>(vel_part->raw_base_ptr);
        const float dt = static_cast<float>(context.delta) * time_scale;

        // The Hot Loop - perfectly auto-vectorizable by the compiler
        for (int64_t i = 0; i < pos_view.count; ++i) {
            pos_view[i].write() = pos_view[i].read() + (velocities[i] * dt);
        }
    }
};
```

##### B. Query Logic
Queries mutate the `MemoryBufferSelectionPOD`. They act as filters, evaluating data and either culling bits (deactivating elements) or pushing Add commands to the Tier 2 buffer (activating elements).
* *Examples:* `ArchetypeQueryLogic`, `FrustumQueryLogic`.
* **Bridge Queries:** A special subset of queries that read from a Source buffer to alter the selection of a Target buffer (e.g., `EventRingBridgeQueryLogic` drains an event ring to wake up sleeping entities in a Sparse Set).

##### C. Metadata Logic
Operations that manipulate non-primary shadow buffers used for acceleration structures, such as hierarchical level-of-detail shifts or spatial clustering.
* *Examples:* `GroupMaskMetadataLogic`, `DSUClusterMetadataLogic`.


## 3: Graph Core Systems (The Routing Layer)

**Target Directory:** `src/core/graphs/` & `src/core/memory/`

While Phase 1 dictates how memory is stored, and Phase 2 dictates how it is mutated, Phase 3 defines the **routing layer**. The Graph Core Systems establish the relational mapping of how tasks form execution pipelines and how memory pools are structured hierarchically or sequentially. 

The core philosophy of this layer is that **topology is treated as data, not as a web of object pointers.**

### Key Components

#### 1. Base Topology: `IdeamGraphDOD`
The foundation of the routing layer. It manages raw nodes, edges, and execution order, stripping away all Godot Node overhead.

* **Cache-Friendly SoA Topology:** Instead of an array of heavy `Node` objects, the build phase uses a Structure of Arrays (`BuildNodesSoA`). This guarantees perfect cache-line utilization during topological sorting and priority evaluations. Cold data (like UI coordinates or type IDs) cannot pollute the L1 cache when the engine is trying to determine execution order.
* **Dirty Flag Architecture:** Uses `GraphDirtyFlags` (`CONNECTIONS`, `PRIORITY`, `STRUCTURE`) to intelligently bypass unnecessary rebuilds. If a user only tweaks a parameter, the graph doesn't recompile its topology.
* **Defragmentation:** When nodes are deleted, it leaves `INVALID_ID` holes. The `defragment()` cycle purges these holes and compacts the memory footprint, invoking a virtual `_remap_ids` down the inheritance chain to keep parallel execution data perfectly synced.

#### 2. The Memory Integrator: `MemoryGraphDOD`
Inherits from `IdeamGraphDOD` to tie the raw execution topology directly to the `MemoryManagerDOD`. This is where logic nodes formally "lease" their memory.

* **Staged Requirements:** Nodes declare their hardware requirements and desired selections. These are appended to a contiguous registry using zero-copy `std::span` operations.
* **Global Validation (`validate_grants`):** Before a graph executes, it audits all leases. It handles physical re-acquisition from the Manager (if the master block was rebased) and logical selection-dirtying (if upstream data changed).
* **Reactive Selection Metadata:** To prevent redundant processing, the graph tracks `SelectionMetadata` for each node. 
  * `dirty_parts_mask`: Tracks if upstream dependencies have altered the bitmask, meaning the query needs to run again.
  * `dependency_version`: A memoization snapshot (e.g., the current Camera Transform version). If the camera hasn't moved, the Frustum Culling node doesn't re-execute; it just passes its cached selection down the pipeline.

#### 3. Pipeline Assembly: `TaskGraphDOD` (Recap)
*(Located in `src/core/tasks/`)*
As the final layer of inheritance (`IdeamGraphDOD` -> `MemoryGraphDOD` -> `TaskGraphDOD`), this brings the execution commands, CPU/GPU metadata, and transient workspaces together. By the time `TaskGraphDOD::execute_graph_dod()` is called, all memory is proven contiguous, all grants are mathematically validated, and the CPU only has to iterate through dense arrays of function pointers.

##### The Graph Data Flow Guarantee
By structuring the Graph inheritance this way, the architecture enforces a strict guarantee: **A Task will never execute if its required memory footprint is invalid, unaligned, or claimed exclusively by another thread.** The Graph resolves these conflicts during compilation and wave-batching, leaving the execution hot-loop completely branchless and lock-free.


## 4: The Engine Boundary Layer

**Target Directory:** `src/godot/` (Specifically `memory/`, `tasks/`, and `graphs/`)

Only after the DOD core is rigidly defined does the Godot API integration enter the picture. The critical rule of the Engine Boundary Layer is that **Godot is treated strictly as an interface, presentation, and command-dispatch layer. It is not the state-holder.** The high-performance C++ backend acts as the "server," and the Godot Scene Tree acts as the "client."

### Key Components

#### 1. The Execution Bridge: `TaskGraphHost`
If `MemoryManagerDOD` is the heart, and `TaskGraphDOD` is the brain, then `TaskGraphHost` is the nervous system connecting them to the Godot Engine. It is a standard Godot `Node` that you drop into a Scene, but its sole purpose is to manage the isolated C++ execution environment.

* **Lifecycle Ownership:** It holds the `std::shared_ptr` to the active `MemoryManagerDOD` and `TaskGraphDOD`. When the Godot Node is freed, the DOD execution environment is safely dismantled.
* **Shared vs. Isolated Topology:** The host can initialize in two ways:
  1. `setup_isolated()`: Creates its own Manager and Graph. Perfect for independent systems (like a localized UI simulation).
  2. `setup_shared()`: Accepts a pointer to *another* `TaskGraphHost`. It shares the same `MemoryManagerDOD` but compiles its own Graph execution order. This allows modular game systems (e.g., Physics Host and AI Host) to operate on the exact same contiguous memory blocks without copying data.
* **The Tick:** It exposes `execute_graph(double p_delta)` to GDScript, allowing the Godot `_process` or `_physics_process` loop to explicitly trigger the Wave batches.

#### 2. State Serialization: Resources & Profiles
Godot's editor needs to save configurations to disk (`.res` or `.tres`), but standard Godot Resources are inherently Object-Oriented and bloated. To solve this, the framework uses "Wrappers."

* **`TaskGraphResource` & `MemoryGraphResource`:** These hold standard Godot dictionaries and arrays for the editor UI to read/write. However, at runtime, they expose a `compile_to_task_graph()` method. This takes the user-friendly serialized data and translates it into the hardcore C++26 `BuildNodesSoA` vectors, entirely shedding the Godot Object overhead before execution begins.
* **`ManagedBufferProfile` & `MemoryBufferResource`:** Godot cannot natively inspect a raw `uint8_t*` DOD block. These classes act as the UI translation layer, reading the compile-time `ViewTraits` (like `SIMD_ACCESS` or `SPATIAL_ACCESS`) and telling the Godot Inspector how to render the data (e.g., as a flat list vs. a 2D grid).

#### 3. Editor UI & Custom Graph Nodes
To provide a smooth developer experience without compromising backend performance, the framework implements custom Editor tooling:

* **`IdeamEditorPlugin` Ecosystem:** Uses a universal registry (`.ideam_registry.cfg`) and a lateral handshake API (`is_plugin_active`). This allows independent GDExtension modules (Memory, Tasks, Narratives) to communicate and share UI space without tightly coupling their C++ compile targets.
* **Custom UI Renders (`TaskGraphEdit`, `TaskGraphNode`):** Because the underlying topology is a strict DOD DAG (Directed Acyclic Graph), standard Godot `GraphEdit` controls are too permissive. The custom UI nodes enforce architectural rules visually (e.g., preventing recursive loops). *Crucially, these visual controls strictly mirror the C++ inheritance of the core* (e.g., `MemoryGraphEdit` -> `TaskGraphEdit`). This ensures complex DOD connection validation rules are inherited perfectly across layers.
* **Pragmatic Inspector Composition:** Unlike the Graph UI, the Inspector UIs intentionally *break* inheritance. While the data resources mirror inheritance, Godot's `_parse_begin()` control injection gets messy across module boundaries. Therefore, `TaskGraphInspector` safely composes its UI independently, proving a pragmatic approach to Godot UI development.


## 5: Narratives

**Target Directory:** `src/godot/narratives/`

The highest level of abstraction in the Ideam Core is the application of the DOD Memory and Graph systems to specific game logic domains. The `narratives` module demonstrates how to map abstract concepts (like story progression and character relationships) onto the high-performance infrastructure.

By treating story logic as a massive, parallel simulation rather than a web of Godot Object references, the framework can evaluate thousands of narrative conditions and state changes per frame without stuttering.

### Key Components

#### 1. The Core Component: `Narreme`
The fundamental unit of narrative data. Registered as an abstract base class, it serves as the foundational data packet that gets loaded into the `MemoryManagerDOD`'s contiguous arrays. 
* All narrative entities (Characters, Props, Locations) and narrative events (Incidents, Plots) inherit from `Narreme`.
* By standardizing the base class, the underlying `TaskGraphDOD` can process mixed narrative elements efficiently using Sparse Sets and Archetype Queries.

#### 2. Narrative Entities
These are the "nouns" of the simulation, representing persistent state data in the memory blocks:
* **`Character`**: Holds dynamic state (health, mood, allegiances).
* **`Location`**: Holds spatial context and ownership data.
* **`Prop`**: Holds inventory and utility state.

#### 3. Narrative Events
These are the "verbs" of the simulation. They utilize the Graph's Command Arenas (Tier 1 and Tier 2) to trigger mutations:
* **`Incident`**: A localized event (e.g., "A sword is swung," "A dialogue line is spoken").
* **`Plot`**: A macro-structure that tracks the progression of multiple incidents.
* **`Narrative`**: The master container tracking the global state of the story.

#### 4. Condition Evaluators (The Query Logic Bridge)
To determine *if* an Incident should occur or *how* a Character feels, the system uses Condition helpers. These map directly to the high-performance `QueryLogic` systems established in Phase 2:
* **`Causal_Condition`**: Evaluates historical data (e.g., "Did Event A happen before Event B?").
* **`Gameplay_Condition`**: Bridges narrative logic with physics/engine logic (e.g., "Is the player inside the Trigger Volume?"). Maps cleanly to a `Spatial_Inclusion_Bridge_Query_Logic` task.
* **`Relationship`**: Evaluates the affinity between two entities. When processed en masse, this acts as a high-speed matrix multiplication (Transform Task) across the relationship buffer.

### The Domain Takeaway
The Narrative system proves that the DOD core is not just for particle systems or physics. By flattening "Story" into contiguous memory arrays, the engine can execute complex, reactive, and highly branchable narratives using the exact same Wave-based execution loops that drive standard gameplay systems.