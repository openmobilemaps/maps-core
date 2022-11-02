package io.openmobilemaps.mapscore.map.scheduling

import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.asCoroutineDispatcher
import java.util.concurrent.Executors

data class AndroidSchedulerDispatchers(
	val ioDispatcher: CoroutineDispatcher = Executors.newFixedThreadPool(32).asCoroutineDispatcher(),
	val computationDispatcher: CoroutineDispatcher = Executors.newFixedThreadPool(16).asCoroutineDispatcher(),
	val defaultDispatcher: CoroutineDispatcher = Executors.newFixedThreadPool(16).asCoroutineDispatcher()
)
