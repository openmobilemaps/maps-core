package io.openmobilemaps.mapscore.kmp

import java.net.HttpURLConnection
import java.net.SocketTimeoutException
import java.net.URL
import java.nio.ByteBuffer
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.concurrent.thread

actual class KMBasicLoader actual constructor() : KMLoaderInterface {
    private val activeRequests = ConcurrentHashMap<String, MutableSet<RequestHandle>>()

    actual override fun loadTexture(url: String, etag: String?): KMTextureLoaderResult =
        loadTextureInternal(url, etag, null)

    actual override fun loadData(url: String, etag: String?): KMDataLoaderResult =
        loadDataInternal(url, etag, null)

    actual override fun loadTextureAsync(url: String, etag: String?): KMFuture<KMTextureLoaderResult> {
        val promise = KMPromise<KMTextureLoaderResult>()
        val handle = track(url)

        thread(name = "KMBasicLoader-texture", isDaemon = true) {
            try {
                if (!handle.cancelled.get()) {
                    promise.setTextureLoaderResult(loadTextureInternal(url, etag, handle))
                }
            } finally {
                untrack(url, handle)
            }
        }

        return promise.future()
    }

    actual override fun loadDataAsync(url: String, etag: String?): KMFuture<KMDataLoaderResult> {
        val promise = KMPromise<KMDataLoaderResult>()
        val handle = track(url)

        thread(name = "KMBasicLoader-data", isDaemon = true) {
            try {
                if (!handle.cancelled.get()) {
                    promise.setDataLoaderResult(loadDataInternal(url, etag, handle))
                }
            } finally {
                untrack(url, handle)
            }
        }

        return promise.future()
    }

    actual override fun cancel(url: String) {
        activeRequests[url]?.toList()?.forEach { handle ->
            handle.cancelled.set(true)
            handle.connection?.disconnect()
        }
    }

    private fun loadTextureInternal(
        url: String,
        etag: String?,
        handle: RequestHandle?,
    ): KMTextureLoaderResult {
        val dataResult = loadDataInternal(url, etag, handle)
        val data = dataResult.data
        if (dataResult.status != KMLoaderStatus.OK || data == null) {
            return KMTextureLoaderResult(
                data = null,
                etag = dataResult.etag,
                status = dataResult.status,
                errorCode = dataResult.errorCode,
            )
        }

        val textureHolder = createTextureHolder(data)
        return if (textureHolder != null) {
            KMTextureLoaderResult(
                data = textureHolder,
                etag = dataResult.etag,
                status = KMLoaderStatus.OK,
                errorCode = null,
            )
        } else {
            KMTextureLoaderResult(
                data = null,
                etag = dataResult.etag,
                status = KMLoaderStatus.ERROR_OTHER,
                errorCode = "DECODING",
            )
        }
    }

    private fun loadDataInternal(
        url: String,
        etag: String?,
        handle: RequestHandle?,
    ): KMDataLoaderResult {
        var connection: HttpURLConnection? = null

        try {
            val openedConnection = URL(url).openConnection() as? HttpURLConnection
                ?: return KMDataLoaderResult(null, null, KMLoaderStatus.ERROR_NETWORK, "CONNECTION")

            connection = openedConnection
            handle?.connection = connection
            connection.requestMethod = "GET"
            connection.instanceFollowRedirects = true
            connection.connectTimeout = CONNECT_TIMEOUT_MS
            connection.readTimeout = READ_TIMEOUT_MS
            connection.setRequestProperty("Referer", "OMM_BasicLoader") // use proper referer
            connection.setRequestProperty("User-Agent", "UBTemplate_KMP;KMP;0.1.0") // use proper user-agent
            etag?.let { connection.setRequestProperty(HEADER_IF_NONE_MATCH, it) }

            return when (val responseCode = connection.responseCode) {
                HttpURLConnection.HTTP_OK -> {
                    val bytes = connection.inputStream.use { it.readBytes() }
                    KMDataLoaderResult(
                        data = bytes.toDirectByteBuffer(),
                        etag = connection.getHeaderField(HEADER_ETAG),
                        status = KMLoaderStatus.OK,
                        errorCode = null,
                    )
                }

                HttpURLConnection.HTTP_NO_CONTENT -> KMDataLoaderResult(
                    data = null,
                    etag = connection.getHeaderField(HEADER_ETAG),
                    status = KMLoaderStatus.OK,
                    errorCode = responseCode.toString(),
                )

                HttpURLConnection.HTTP_BAD_REQUEST -> KMDataLoaderResult(
                    data = null,
                    etag = connection.getHeaderField(HEADER_ETAG),
                    status = KMLoaderStatus.ERROR_400,
                    errorCode = responseCode.toString(),
                )

                HttpURLConnection.HTTP_NOT_FOUND -> KMDataLoaderResult(
                    data = null,
                    etag = connection.getHeaderField(HEADER_ETAG),
                    status = KMLoaderStatus.ERROR_404,
                    errorCode = responseCode.toString(),
                )

                else -> KMDataLoaderResult(
                    data = null,
                    etag = connection.getHeaderField(HEADER_ETAG),
                    status = KMLoaderStatus.ERROR_OTHER,
                    errorCode = responseCode.toString(),
                )
            }
        } catch (_: SocketTimeoutException) {
            return KMDataLoaderResult(null, null, KMLoaderStatus.ERROR_TIMEOUT, null)
        } catch (error: Exception) {
            val status = if (handle?.cancelled?.get() == true) {
                KMLoaderStatus.ERROR_OTHER
            } else {
                KMLoaderStatus.ERROR_NETWORK
            }
            return KMDataLoaderResult(null, null, status, error.message)
        } finally {
            handle?.connection = null
            connection?.disconnect()
        }
    }

    private fun track(url: String): RequestHandle {
        val handle = RequestHandle()
        activeRequests.compute(url) { _, existing ->
            val requests = existing ?: ConcurrentHashMap.newKeySet<RequestHandle>()
            requests.add(handle)
            requests
        }
        return handle
    }

    private fun untrack(url: String, handle: RequestHandle) {
        activeRequests.computeIfPresent(url) { _, requests ->
            requests.remove(handle)
            requests.takeIf { it.isNotEmpty() }
        }
    }

    private class RequestHandle {
        val cancelled = AtomicBoolean(false)

        @Volatile
        var connection: HttpURLConnection? = null
    }

    private companion object {
        private const val CONNECT_TIMEOUT_MS = 20_000
        private const val READ_TIMEOUT_MS = 20_000
        private const val HEADER_ETAG = "ETag"
        private const val HEADER_IF_NONE_MATCH = "If-None-Match"
    }
}

private fun ByteArray.toDirectByteBuffer(): ByteBuffer =
    ByteBuffer.allocateDirect(size).apply {
        put(this@toDirectByteBuffer)
    }
