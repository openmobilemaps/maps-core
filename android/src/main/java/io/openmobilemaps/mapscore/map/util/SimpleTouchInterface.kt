package io.openmobilemaps.mapscore.map.util

import io.openmobilemaps.mapscore.shared.graphics.common.Vec2F
import io.openmobilemaps.mapscore.shared.map.controls.TouchInterface

open class SimpleTouchInterface : TouchInterface() {
	override fun onTouchDown(posScreen: Vec2F): Boolean {
		return false
	}

	override fun onClickUnconfirmed(posScreen: Vec2F): Boolean {
		return false
	}

	override fun onClickConfirmed(posScreen: Vec2F): Boolean {
		return false
	}

	override fun onDoubleClick(posScreen: Vec2F): Boolean {
		return false
	}

	override fun onLongPress(posScreen: Vec2F): Boolean {
		return false
	}

	override fun onMove(deltaScreen: Vec2F, confirmed: Boolean, doubleClick: Boolean): Boolean {
		return false
	}

	override fun onMoveComplete(): Boolean {
		return false
	}

	override fun onTwoFingerClick(posScreen1: Vec2F, posScreen2: Vec2F): Boolean {
		return false
	}

	override fun onTwoFingerMove(posScreenOld: ArrayList<Vec2F>, posScreenNew: ArrayList<Vec2F>): Boolean {
		return false
	}

	override fun onTwoFingerMoveComplete(): Boolean {
		return false
	}

	override fun clearTouch() {}
}