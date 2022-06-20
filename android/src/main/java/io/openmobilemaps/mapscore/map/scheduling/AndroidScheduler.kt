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

import io.openmobilemaps.mapscore.shared.map.scheduling.*
import kotlinx.coroutines.*
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.coroutines.CoroutineContext
import kotlin.coroutines.EmptyCoroutineContext

class AndroidScheduler(
	private val schedulerCallback: AndroidSchedulerCallback,
	private val ioDispatcher: CoroutineDispatcher = Dispatchers.IO,
	private val computationDispatcher: CoroutineDispatcher = Dispatchers.Default,
	private val defaultDispatcher: CoroutineDispatcher = Dispatchers.Default
) : SchedulerInterface() {

	private var isResumed = AtomicBoolean(false)
	private lateinit var coroutineScope: CoroutineScope

	private val taskQueueMap: ConcurrentHashMap<TaskPriority, ConcurrentLinkedQueue<TaskInterface>> = ConcurrentHashMap()

	init {
		TaskPriority.values().forEach { taskQueueMap.put(it, ConcurrentLinkedQueue()) }
	}

	private val delayedTaskMap: ConcurrentHashMap<String, Job> = ConcurrentHashMap()

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
			delayedTaskMap.put(id, coroutineScope.launch(defaultDispatcher) {
				delay(task.getConfig().delay)
				if (isActive) {
					if (isResumed.get()) {
						scheduleTask(task)
					} else {
						task.getConfig().delay = 0
						taskQueueMap[task.getConfig().priority]!!.offer(task)
					}
					delayedTaskMap.remove(id)
				}
			})
		} else {
			scheduleTask(task)
		}
	}

	private fun scheduleTask(task: TaskInterface) {
		when (task.getConfig().executionEnvironment) {
			ExecutionEnvironment.GRAPHICS -> schedulerCallback.scheduleOnGlThread(task)
			ExecutionEnvironment.IO -> coroutineScope.launch(ioDispatcher) { task.run() }
			ExecutionEnvironment.COMPUTATION -> coroutineScope.launch(computationDispatcher) { task.run() }
			else -> coroutineScope.launch(defaultDispatcher) { task.run() }
		}
	}

	override fun removeTask(id: String) {
		delayedTaskMap.remove(id)?.cancel()
	}

	override fun clear() {
		taskQueueMap.forEach { it.value.clear() }
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
