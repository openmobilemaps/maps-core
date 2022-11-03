package io.openmobilemaps.mapscore.map.scheduling

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.asCoroutineDispatcher
import java.util.concurrent.Executors

data class AndroidSchedulerDispatchers(
	val io: CoroutineDispatcher = Executors.newFixedThreadPool(32).asCoroutineDispatcher(),
	val computation: CoroutineDispatcher = Executors.newFixedThreadPool(16).asCoroutineDispatcher(),
	val default: CoroutineDispatcher = Executors.newFixedThreadPool(16).asCoroutineDispatcher()
)
