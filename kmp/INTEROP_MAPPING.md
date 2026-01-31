# KMP ↔ Djinni Interop Mapping (Maps Core)

This document defines **the canonical mapping patterns** for KMP APIs that must be **drop‑in replacements on Android** (using Djinni types) and **wrappable on iOS** (ObjC/Swift interop). It is intended to be a **consistent recipe** for all maps‑core APIs.

## Goals

- **Common API is KMP‑first**: `expect` declarations in `commonMain`.
- **Android is drop‑in**: actuals map to Djinni types wherever possible.
- **iOS is implementable**: actuals wrap ObjC interfaces with minimal glue.
- **Predictable patterns**: same shapes across all APIs, minimal surprises.

---

## 1) Type Mapping Decisions

### Djinni records (structs / data)
- [x] **Selected:** `expect class` in `commonMain` with constructor + properties. Actuals are **`typealias`** to Djinni records when signatures match.
- [ ] Wrapper class (custom Kotlin data class, manual mapping).
- [ ] `expect data class` always (even if actual can be alias).

**Why:** Djinni records already match Kotlin data classes. `typealias` makes Android drop‑in and iOS easy (ObjC record classes). Wrapper is only used when names or defaults differ.

### Djinni enums
- [x] **Selected:** `expect enum class` with **actual `typealias`** on Android and iOS when possible.
- [ ] Wrapper enum mapping to Djinni enum values.

**Why:** Direct alias avoids extra mapping and stays drop‑in on Android.

### Djinni interfaces (class‑like)
- [x] **Selected:** `expect class` with `nativeHandle: Any?` and instance methods. Actuals:
  - **Android:** wrapper around Djinni interface (not typealias), unless the Djinni type already exactly matches expected API.
  - **iOS:** wrapper around ObjC interface (store ObjC instance in `nativeHandle`).
- [ ] `typealias` for all interfaces.

**Why:** iOS needs a wrapper to manage ObjC lifetimes and method translation. Keeping a wrapper in both platforms makes the API consistent and allows optional adaptation.

---

## 2) Factory / Static Methods

### Djinni static creators
- [x] **Selected:** `companion object` in the `expect class` with factory methods. Actuals forward to Djinni static and return **wrapper instances**.
- [ ] Standalone `expect object` for all factories.
- [ ] Global top‑level functions.

**Why:** Mirrors Djinni static methods closely and keeps the type cohesive. Works for both platforms and preserves Android drop‑in behavior.

**Pattern:**
```kotlin
// commonMain
expect class Foo constructor(nativeHandle: Any? = null) {
    protected val nativeHandle: Any?
    companion object {
        fun create(...): Foo?
    }
}
```

---

## 3) Arguments & Return Types

### Non‑primitive arguments
- [x] **Selected:** Common API uses **KMP wrapper types**, not Djinni types directly.
- [ ] Expose Djinni types in common API.

**Why:** Common API should be platform‑agnostic. Wrappers give room for platform differences.

### Collections
- [x] **Selected:** Use `List<T>` / `Map<K,V>` in common. Convert to `ArrayList` / `HashMap` in platform actuals as needed.
- [ ] Use mutable collections everywhere.

**Why:** `List`/`Map` is idiomatic and safer in common. Convert only where Djinni requires mutable types.

### Optionals
- [x] **Selected:** Kotlin nullable types (`T?`) for Djinni `optional<T>`.

---

## 4) Callbacks

### Djinni callback interfaces
- [x] **Selected:** Common `interface` for callbacks, plus **platform proxy** that implements Djinni callback interface and forwards to the common callback.
- [ ] Expose Djinni callback interface directly in common.

**Why:** Common code should not depend on Djinni types. A proxy keeps the boundary clean.

**Selected API shape:**
```kotlin
// commonMain
interface SelectionCallback {
    fun didSelectFeature(feature: FeatureInfo, layerIdentifier: String, coord: Coord): Boolean
    fun didMultiSelectLayerFeatures(features: List<FeatureInfo>, layerIdentifier: String, coord: Coord): Boolean
    fun didClickBackgroundConfirmed(coord: Coord): Boolean
}
```

---

## 5) Async & Future Types

- [x] **Selected:** Keep Djinni future types **in platform layer only**, translate to `suspend` in common API when needed.
- [ ] Expose Djinni Future in common.

**Why:** KMP callers should use Kotlin concurrency primitives. Platform adapters can bridge futures.

---

## 6) Ownership & Lifecycle

- [x] **Selected:** Wrapper types may expose `close()`/`destroy()` if Djinni object requires disposal. If not, omit.
- [ ] Always expose `close()` even if unused.

**Why:** Keep API minimal but safe when required by Djinni.

---

## 7) Naming & Interop

- [x] **Selected:** Keep KMP names close to Djinni names, but **no `MC` prefix** in common.
- [ ] Mirror ObjC names exactly in common.

**Why:** KMP API should be platform‑agnostic and readable. Prefixes are implementation details.

---

# Templates

## A) Record (Djinni `record`)
```kotlin
// commonMain
expect class Coord(
    systemIdentifier: Int,
    x: Double,
    y: Double,
    z: Double,
) {
    val systemIdentifier: Int
    val x: Double
    val y: Double
    val z: Double
}

// androidMain
actual typealias Coord = io.openmobilemaps.mapscore.shared.map.coordinates.Coord

// iosMain
actual typealias Coord = MapCoreSharedModule.MCCoord
```

## B) Interface Wrapper (Djinni `interface`)
```kotlin
// commonMain
expect class MapVectorLayer constructor(nativeHandle: Any? = null) {
    protected val nativeHandle: Any?
    fun setSelectionDelegate(delegate: SelectionCallback?)
    fun setGlobalState(state: Map<String, FeatureInfoValue>)
}

// androidMain
actual class MapVectorLayer actual constructor(nativeHandle: Any?) {
    protected actual val nativeHandle: Any? = nativeHandle
    private val layer = nativeHandle as? MapscoreVectorLayerInterface

    actual fun setSelectionDelegate(delegate: SelectionCallback?) {
        val proxy = delegate?.let { SelectionCallbackProxy(it) }
        layer?.setSelectionDelegate(proxy)
    }

    actual fun setGlobalState(state: Map<String, FeatureInfoValue>) {
        // map → Djinni
    }
}

// iosMain
actual class MapVectorLayer actual constructor(nativeHandle: Any?) {
    protected actual val nativeHandle: Any? = nativeHandle
    private val layer = nativeHandle as? MCTiled2dMapVectorLayerInterface

    actual fun setSelectionDelegate(delegate: SelectionCallback?) {
        val proxy = delegate?.let { SelectionCallbackProxy(it) }
        layer?.setSelectionDelegate(proxy)
    }

    actual fun setGlobalState(state: Map<String, FeatureInfoValue>) {
        // map → ObjC
    }
}
```

## C) Static Creator (Djinni `static`)
```kotlin
// commonMain
expect class VectorLayer constructor(nativeHandle: Any? = null) {
    protected val nativeHandle: Any?
    companion object {
        fun createExplicitly(...): VectorLayer?
    }
}

// androidMain
actual class VectorLayer actual constructor(nativeHandle: Any?) {
    protected actual val nativeHandle: Any? = nativeHandle
    actual companion object {
        actual fun createExplicitly(...): VectorLayer? {
            val raw = DjinniVectorLayerInterface.createExplicitly(...)
            return raw?.let { VectorLayer(it) }
        }
    }
}

// iosMain
actual class VectorLayer actual constructor(nativeHandle: Any?) {
    protected actual val nativeHandle: Any? = nativeHandle
    actual companion object {
        actual fun createExplicitly(...): VectorLayer? {
            val raw = MCVectorLayerInterface.createExplicitly(...)
            return raw?.let { VectorLayer(it) }
        }
    }
}
```

## D) Callback Proxy
```kotlin
// commonMain
interface SelectionCallback { ... }

// androidMain
actual class SelectionCallbackProxy actual constructor(
    private val handler: SelectionCallback,
) : MapscoreSelectionCallbackInterface() {
    override fun didSelectFeature(...): Boolean =
        handler.didSelectFeature(...)
}

// iosMain
actual class SelectionCallbackProxy actual constructor(
    private val handler: SelectionCallback,
) : NSObject(), MCTiled2dMapVectorLayerSelectionCallbackInterfaceProtocol {
    override fun didSelectFeature(...): Boolean =
        handler.didSelectFeature(...)
}
```

---

## When to Deviate
- **Defaults / renames required:** use a wrapper instead of typealias.
- **Async API surface:** expose suspend/Flow in common and bridge in actuals.
- **Performance‑critical:** consider direct alias on Android only, but keep wrapper for iOS.

---

## Checklist for New API
1. Does it exist in Djinni? → use these mappings.
2. Record/enum? → expect + typealias.
3. Interface? → wrapper + `nativeHandle`.
4. Static creators? → companion object forwarding.
5. Callback? → common interface + platform proxy.
6. Any list/map? → use Kotlin `List/Map`, convert in actuals.
7. Any optional? → Kotlin nullable.

