/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package io.openmobilemaps.mapscore.map.scheduling

import io.openmobilemaps.mapscore.shared.map.scheduling.ExecutionEnvironment
import io.openmobilemaps.mapscore.shared.map.scheduling.SchedulerGraphicsTaskCallbacks
import io.openmobilemaps.mapscore.shared.map.scheduling.SchedulerInterface
import io.openmobilemaps.mapscore.shared.map.scheduling.TaskConfig
import io.openmobilemaps.mapscore.shared.map.scheduling.TaskInterface
import io.openmobilemaps.mapscore.shared.map.scheduling.TaskPriority
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.coroutines.CoroutineContext
import kotlin.coroutines.EmptyCoroutineContext

class AndroidScheduler(
	schedulerCallback: AndroidSchedulerCallback,
	private val dispatchers: AndroidSchedulerDispatchers = AndroidSchedulerDispatchers()
) : SchedulerInterface() {

	private var isResumed = AtomicBoolean(false)
	private lateinit var coroutineScope: CoroutineScope

	private val taskQueueMap: ConcurrentHashMap<TaskPriority, ConcurrentLinkedQueue<TaskInterface>> = ConcurrentHashMap()
	private val runningTasksMap: ConcurrentHashMap<String, Job> = ConcurrentHashMap()
	private val delayedTaskMap: ConcurrentHashMap<String, Job> = ConcurrentHashMap()

	private var schedulerCallback: AndroidSchedulerCallback? = schedulerCallback
	private var graphicsTaskCallbacks: SchedulerGraphicsTaskCallbacks? = null

	init {
		TaskPriority.values().forEach { taskQueueMap.put(it, ConcurrentLinkedQueue()) }
	}

	fun setCoroutineScope(coroutineScope: CoroutineScope) {
		this.coroutineScope = coroutineScope
	}

    override fun addTasks(tasks: ArrayList<TaskInterface>) {
        tasks.forEach { addTask(it) }
    }

	override fun addTask(task: TaskInterface) {
		if (isResumed.get()) {
			handleNewTask(task)
		} else {
			taskQueueMap[task.getConfig().priority]!!.offer(task)
		}
	}

	private fun handleNewTask(task: TaskInterface) {
		if (task.getConfig().delay > 0) {
			val id = task.getConfig().id
			delayedTaskMap[id] = coroutineScope.launch(dispatchers.default) {
				delay(task.getConfig().delay)
				if (isActive) {
					if (isResumed.get()) {
						scheduleTask(task)
					} else {
						val newTaskConfig = task.getConfig().let { TaskConfig(it.id, 0L, it.priority, it.executionEnvironment) }
						val newTask = object : TaskInterface() {
							override fun getConfig(): TaskConfig = newTaskConfig
							override fun run() { task.run() }
						}
						taskQueueMap[task.getConfig().priority]!!.offer(newTask)
					}
					delayedTaskMap.remove(id)
				}
			}
		} else {
			scheduleTask(task)
		}
	}

	private fun scheduleTask(task: TaskInterface) {
		val executionEnvironment = task.getConfig().executionEnvironment
		if (executionEnvironment == ExecutionEnvironment.GRAPHICS) {
			schedulerCallback?.scheduleOnGlThread(task)
			graphicsTaskCallbacks?.requestGraphicsTaskExecution()
		} else {
			val dispatcher = when (executionEnvironment) {
				ExecutionEnvironment.IO -> dispatchers.io
				ExecutionEnvironment.COMPUTATION -> dispatchers.computation
				else -> dispatchers.default
			}
			runningTasksMap[task.getConfig().id] = coroutineScope.launch(dispatcher) {
				if (!isActive) {
					return@launch
				}
				task.run()
				runningTasksMap.remove(task.getConfig().id)
			}
		}
	}

	override fun removeTask(id: String) {
		delayedTaskMap.remove(id)?.cancel()
		runningTasksMap.remove(id)?.cancel()
	}

	override fun clear() {
		taskQueueMap.forEach { it.value.clear() }

		val tempDelayedJobs = delayedTaskMap.values.toList()
		delayedTaskMap.clear()
		tempDelayedJobs.forEach { it.cancel() }

		val tempRunningJobs = runningTasksMap.values.toList()
		runningTasksMap.clear()
		tempRunningJobs.forEach { it.cancel() }
	}

	override fun pause() {
		isResumed.set(false)
	}

	override fun resume() {
		isResumed.set(true)
		listOf(TaskPriority.HIGH, TaskPriority.NORMAL, TaskPriority.LOW).forEach { priority ->
			val taskQueue = taskQueueMap[priority] ?: return@forEach
			while (taskQueue.isNotEmpty()) {
				taskQueue.poll()?.let { it ->
					handleNewTask(it)
				}
			}
		}
	}

	override fun destroy() {
		schedulerCallback = null
	}

	override fun hasSeparateGraphicsInvocation(): Boolean = false

	override fun runGraphicsTasks(): Boolean = false // unused

	override fun setSchedulerGraphicsTaskCallbacks(callbacks: SchedulerGraphicsTaskCallbacks) {
		graphicsTaskCallbacks = callbacks
	}

	fun launchCoroutine(
		context: CoroutineContext = EmptyCoroutineContext,
		start: CoroutineStart = CoroutineStart.DEFAULT,
		block: suspend CoroutineScope.() -> Unit
	) {
		if (isResumed.get()) {
			coroutineScope.launch(context, start, block)
		}
	}
}
