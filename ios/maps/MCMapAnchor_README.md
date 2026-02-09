# MCMapAnchor Usage Guide

The MCMapAnchor feature enables positioning UIKit views relative to map coordinates using standard AutoLayout constraints. Views anchored to map coordinates will automatically update their position when the camera changes (pan, zoom, rotate).

## Basic Usage

### Creating an Anchor

```swift
// Create an anchor for a specific coordinate
let zurichCoord = MCCoord(lat: 47.3769, lon: 8.5417)
let anchor = mapView.createAnchor(for: zurichCoord)

// Position any UIView relative to the map coordinate using AutoLayout
let pinView = UIView()
pinView.backgroundColor = .red
NSLayoutConstraint.activate([
    pinView.centerXAnchor.constraint(equalTo: anchor.centerXAnchor),
    pinView.centerYAnchor.constraint(equalTo: anchor.centerYAnchor)
])

// The pin automatically stays positioned at the coordinate as users interact with the map
```

## Available Anchors

Each MCMapAnchor provides the following NSLayoutAnchor properties:

- `centerXAnchor` - Horizontal center of the coordinate
- `centerYAnchor` - Vertical center of the coordinate  
- `leadingAnchor` - Leading edge of the coordinate
- `trailingAnchor` - Trailing edge of the coordinate
- `topAnchor` - Top edge of the coordinate
- `bottomAnchor` - Bottom edge of the coordinate

## Dynamic Coordinate Updates

You can update the anchor's coordinate at any time:

```swift
let anchor = mapView.createAnchor(for: initialCoord)

// Later, move the anchor to a new location
anchor.coordinate = newCoord // View will automatically move
```

## Anchor Lifecycle Management

### Multiple Anchors

```swift
let anchor1 = mapView.createAnchor(for: coord1)
let anchor2 = mapView.createAnchor(for: coord2) 
let anchor3 = mapView.createAnchor(for: coord3)

// Check how many anchors are active
print("Active anchors: \(mapView.activeAnchors.count)")
```

### Removing Anchors

```swift
// Remove a specific anchor
mapView.removeAnchor(anchor1)

// Remove all anchors
mapView.removeAllAnchors()
```

## Advanced Usage Examples

### Offset Positioning

```swift
let anchor = mapView.createAnchor(for: coordinate)

let labelView = UILabel()
NSLayoutConstraint.activate([
    labelView.bottomAnchor.constraint(equalTo: anchor.topAnchor, constant: -10),
    labelView.centerXAnchor.constraint(equalTo: anchor.centerXAnchor)
])
```

### Multiple Views on Same Anchor

```swift
let anchor = mapView.createAnchor(for: coordinate)

let pinView = UIView() // Pin at center
NSLayoutConstraint.activate([
    pinView.centerXAnchor.constraint(equalTo: anchor.centerXAnchor),
    pinView.centerYAnchor.constraint(equalTo: anchor.centerYAnchor)
])

let labelView = UILabel() // Label above pin
NSLayoutConstraint.activate([
    labelView.bottomAnchor.constraint(equalTo: anchor.topAnchor, constant: -5),
    labelView.centerXAnchor.constraint(equalTo: anchor.centerXAnchor)
])
```

## Implementation Details

- Uses a hidden internal UIView that participates in the layout system
- Converts coordinates to screen positions using `camera.screenPosFromCoord()`
- Efficiently batches updates only when camera actually changes
- Proper memory management with weak references to prevent retain cycles
- Automatic constraint cleanup on anchor removal
- Thread-safe coordinate updates

## Performance Considerations

- Camera listener is only active when anchors exist
- Updates are batched for efficiency 
- Anchors are automatically cleaned up when MapView is deallocated
- Minimal overhead when no anchors are present