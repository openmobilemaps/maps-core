package io.openmobilemaps.mapscore.extensions

import io.openmobilemaps.mapscore.shared.map.scheduling.*

fun SchedulerInterface.addGraphicsTask(id: String, delay: Long = 0, priority: TaskPriority = TaskPriority.NORMAL, block: () -> Unit) =
	addTask(object : TaskInterface() {
		override fun getConfig() = TaskConfig(id, delay, priority, ExecutionEnvironment.GRAPHICS)
		override fun run() = block()
	})