package ch.ubique.mapscore.shared.map.scheduling

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
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

	override fun addTask(task: TaskInterface?) {
		val task = task ?: return
		if (isResumed.get()) {
			scheduleTask(task)
		} else {
			taskQueueMap[task.config.priority]!!.offer(task)
		}
	}

	private fun scheduleTask(task: TaskInterface) {
		// TODO: if is delayed - wrap execution/graphics schedule in cancellable coroutine and delay
		when (task.config.executionEnvironment) {
			ExecutionEnvironment.GRAPHICS -> schedulerCallback.scheduleOnGlThread(task)
			ExecutionEnvironment.IO -> coroutineScope.launch(Dispatchers.IO) { task.run() }
			ExecutionEnvironment.COMPUTATION -> coroutineScope.launch(Dispatchers.Default) { task.run() }
			else -> coroutineScope.launch(Dispatchers.Default) { task.run() }
		}
	}

	override fun removeTask(id: String?) {
		val id = id ?: return
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
				taskQueue.poll()?.let { ::scheduleTask }
			}
		}
	}
}