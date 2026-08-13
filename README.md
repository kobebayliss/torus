# Torus

A lock-free, single-producer/single-consumer (SPSC) ring buffer queue for low-latency C++ applications.

Torus is a header-only, fixed-capacity circular buffer designed for the common "one thread writes, one thread reads" pattern found in trading systems, audio pipelines, telemetry, and other latency-sensitive workloads.

## Features

- **Lock-free** — built entirely on `std::atomic` with acquire/release semantics, no mutexes or spinlocks
- **Cache-line aware** — the head and tail indices are each aligned to `std::hardware_destructive_interference_size` to prevent false sharing between producer and consumer cores
- **Power-of-two capacity** — capacity is rounded up to the next power of two (via `std::bit_ceil`) so index wrapping can be performed with a 1-cycle AND operation rather than modulo
- **64-byte aligned storage** — the backing buffer is allocated with `std::aligned_alloc(64, ...)` for cache-friendly access patterns

## Performance
 
Benchmark: 100,000,000 items pushed and popped, verified in order with no drops or duplicates.
 
| Implementation | Total Time | Time per Item (push+pop) | Time per Op | Throughput |
|---|---|---|---|---|
| `std::deque` (mutex) | 12,626.4 ms | 126.26 ns | 63.13 ns | ~7.92M items/sec |
| Boost (lock-free) | 5,797.38 ms | 57.97 ns | 28.99 ns | ~17.25M items/sec |
| **Torus** | **3,281.48 ms** | **32.81 ns** | **16.41 ns** | **~30.47M items/sec** |
 
Torus is **~3.85x faster** than a mutex-guarded `std::deque` and **~1.77x faster** than Boost's lock-free queue on this benchmark.
 
> Results are from a single SPSC producer/consumer benchmark run and will vary by hardware, compiler, and optimization flags. Re-run on your target platform before relying on these numbers for capacity planning.

<img width="664" height="157" alt="Screenshot 2026-08-13 at 8 53 41 PM" src="https://github.com/user-attachments/assets/f4a8153f-f4b1-45e9-af0b-1f8e13726b3d" />

## API

```cpp
template<typename T, uint64_t capacity>
class SPSCQueue {
public:
    SPSCQueue();
    ~SPSCQueue();

    // Non-copyable, non-movable
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;
    SPSCQueue(SPSCQueue&&) = delete;
    SPSCQueue& operator=(SPSCQueue&&) = delete;

    [[nodiscard]] bool try_push(const T& item) noexcept;
    [[nodiscard]] std::optional<T> try_pop() noexcept;
};
```

| Method | Description |
|---|---|
| `try_push(const T&)` | Attempts to enqueue an item. Returns `false` if the queue is full. Called only from the producer thread. |
| `try_pop()` | Attempts to dequeue an item. Returns `std::nullopt` if the queue is empty. Called only from the consumer thread. |

> **Note:** `capacity` is a minimum — the actual usable capacity is rounded up to the next power of two internally (`rounded_capacity`).



## Requirements

- C++20 or later (uses `std::bit_ceil`, `std::hardware_destructive_interference_size`)
- `T` must be trivially copyable (`std::is_trivially_copyable_v<T>`), enforced via `static_assert`

## Design Notes

- **Index space vs. slot space.** `head` and `tail` are monotonically increasing 64-bit counters, not wrapped slot indices. Wrapping into `[0, rounded_capacity)` happens only at the point of buffer access, via `& (rounded_capacity - 1)`. This lets full/empty be distinguished cleanly by comparing raw counter values (`h - t == rounded_capacity` means full) without needing a sentinel slot or separate size counter.
- **Cached indices avoid cross-core traffic.** The producer keeps a local `tail_cached` and the consumer keeps a local `head_cached`. Each side only re-reads the other's atomic (with `acquire`) when its cached value suggests the queue might be full or empty, respectively. This significantly reduces contention on the shared cache line under steady-state throughput.
- **Memory ordering.** Publishing a new `head`/`tail` uses `memory_order_release`, and reading the other side's index (on the cache-miss path) uses `memory_order_acquire`. This establishes a happens-before relationship ensuring the buffer write is visible before the index update is observed.
- **False-sharing prevention.** `head` and `tail` live on separate cache lines (`alignas(std::hardware_destructive_interference_size)`), since they're written by different threads. `tail_cached` and `head_cached` are deliberately colocated with the *opposite* index rather than isolated further, since each is only ever touched by the thread that owns the atomic it sits beside.
- **Trivially copyable only.** Items are copied in/out with plain assignment (`buffer[i] = item`), so no constructors/destructors are run on the storage. This keeps `try_push`/`try_pop` allocation-free and branch-light, at the cost of not supporting non-trivial types.

## Limitations

- Strictly SPSC — using it from multiple producers or multiple consumers concurrently is undefined behavior.
- No blocking API; callers are expected to spin, back off, or integrate with their own signaling/eventing mechanism while the queue is full or empty.
- No built-in way to query current size/occupancy (by design, to keep the hot path minimal).
- Fixed capacity, set at compile time via the `capacity` template parameter.

## License

Add your license of choice here (e.g. MIT, Apache 2.0).
