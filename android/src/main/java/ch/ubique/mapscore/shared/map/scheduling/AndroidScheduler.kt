/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

package ch.ubique.mapscore.shared.map.scheduling

import kotlinx.coroutines.*
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ConcurrentLinkedQueue
import java.util.concurrent.atomic.AtomicBoolean

class AndroidScheduler(private val schedulerCallback: AndroidSchedulerCallback) : SchedulerInterface() {

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
			delayedTaskMap.put(id, coroutineScope.launch(Dispatchers.Default) {
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
			ExecutionEnvironment.IO -> coroutineScope.launch(Dispatchers.IO) { task.run() }
			ExecutionEnvironment.COMPUTATION -> coroutineScope.launch(Dispatchers.Default) { task.run() }
			else -> coroutineScope.launch(Dispatchers.Default) { task.run() }
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
				taskQueue.poll()?.let { ::handleNewTask }
			}
		}
	}
}