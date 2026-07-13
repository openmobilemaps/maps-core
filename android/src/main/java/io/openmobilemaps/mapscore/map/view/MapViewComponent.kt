package io.openmobilemaps.mapscore.map.view

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalInspectionMode
import androidx.compose.ui.viewinterop.AndroidView
import androidx.lifecycle.compose.LocalLifecycleOwner
import io.openmobilemaps.mapscore.shared.map.MapConfig
import io.openmobilemaps.mapscore.shared.map.MapInterface
import kotlinx.coroutines.flow.first

@Composable
fun MapViewComponent(
	mapConfig: MapConfig,
	is3d: Boolean,
	modifier: Modifier = Modifier,
	isTouchEnabled: Boolean = true,
	onMapDispose: () -> Unit = {},
	onMapInitialize: (MapInterface) -> Unit,
) {
	if (LocalInspectionMode.current) {
		Box(modifier.background(Color.White))
		return
	}

	val lifecycle = LocalLifecycleOwner.current.lifecycle

	var mapView by remember { mutableStateOf<MapView?>(null) }

	LaunchedEffect(mapView) {
		val view = mapView ?: return@LaunchedEffect
		view.mapViewState.first { it == MapViewState.RESUMED }
		onMapInitialize(view.requireMapInterface())
	}

	AndroidView(
		modifier = modifier,
		factory = { context ->
			MapView(context).apply {
				setupMap(
					mapConfig = mapConfig,
					density = context.resources.displayMetrics.xdpi,
					useMSAA = false,
					is3D = is3d
				)
				setTouchEnabled(isTouchEnabled)
				registerLifecycle(lifecycle)
				mapView = this
			}
		}
	)

	DisposableEffect(Unit) {
		onDispose {
			onMapDispose()
			mapView = null
		}
	}
}
