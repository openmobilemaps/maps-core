package ch.ubique.mapscore.shared.map.scheduling

interface AndroidSchedulerCallback {
	fun scheduleOnGlThread(task: TaskInterface)
}