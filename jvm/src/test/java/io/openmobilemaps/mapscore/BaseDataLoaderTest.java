package io.openmobilemaps.mapscore;

import static org.junit.jupiter.api.Assertions.*;

import com.snapchat.djinni.Future;

import io.openmobilemaps.mapscore.map.loader.BaseDataLoader;
import io.openmobilemaps.mapscore.shared.map.loader.DataLoaderResult;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus;

import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;
import org.junit.jupiter.api.Test;

import java.io.ByteArrayInputStream;
import java.time.Duration;
import java.util.*;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.stream.Collectors;

class BaseDataLoaderTest {

    static class TestLoader extends BaseDataLoader {
        /**
         * Simulated test resource entry
         *
         * @param url Url for which this entry will be returned
         * @param data Data to be returned. If null, an exception is raised instead during loading.
         * @param delay Time to wait until completing the future. If null or zero, returns a
         *     completed future.
         */
        record Entry(@NotNull String url, @Nullable String data, @Nullable Duration delay) {}

        private final List<Entry> entries;

        TestLoader(List<Entry> entries) {
            this.entries = entries;
        }

        @NotNull
        @Override
        protected CompletableFuture<LoadResult> fetchData(
                @NotNull String url, @Nullable String etag) {

            var entry = entries.stream().filter(e -> e.url.equals(url)).findAny().orElse(null);
            assert (entry != null);
            if (entry.data != null) {
                return produceResult(entry);
            } else {
                return produceError(entry);
            }
        }

        private CompletableFuture<LoadResult> produceResult(Entry entry) {
            var inputStream = new ByteArrayInputStream(entry.data.getBytes());
            var result = new LoadResult(entry.url, inputStream, null, LoaderStatus.OK, null);

            if (entry.delay == null || entry.delay == Duration.ZERO) {
                return CompletableFuture.completedFuture(result);
            } else {
                return CompletableFuture.supplyAsync(
                        () -> {
                            try {
                                Thread.sleep(entry.delay);
                            } catch (InterruptedException e) {
                                throw new RuntimeException(e);
                            }
                            return result;
                        });
            }
        }

        private CompletableFuture<LoadResult> produceError(Entry entry) {
            if (entry.delay == null) {
                throw new RuntimeException("Just testing: error immediately");
            } else if (entry.delay == Duration.ZERO) {
                return CompletableFuture.failedFuture(
                        new RuntimeException("Just testing: completed immediately with error"));
            } else {
                return CompletableFuture.supplyAsync(
                        () -> {
                            try {
                                Thread.sleep(entry.delay);
                            } catch (InterruptedException e) {
                                throw new RuntimeException(e);
                            }
                            throw new RuntimeException("Just testing: error after delay");
                        });
            }
        }
    }

    /**
     * This test checks that the BaseDataLoader correctly handles the promises and futures even if
     * the implementation returns results very quickly or throw errors.
     *
     * <p>Note: Messy log output is expected.
     */
    @Test
    public void loadDataAsync() {
        var resources =
                List.of(
                        new TestLoader.Entry("test://bar", "slowest", Duration.ofMillis(20)),
                        new TestLoader.Entry("test://foo", "fizzle", Duration.ofMillis(10)),
                        new TestLoader.Entry(
                                "test://almost_immediately", "bang", Duration.ofNanos(100)),
                        new TestLoader.Entry("test://immediately", "poof", Duration.ZERO),
                        new TestLoader.Entry("test://throw", null, null),
                        new TestLoader.Entry("test://error_immediately", null, Duration.ZERO),
                        new TestLoader.Entry("test://error_delay", null, Duration.ofMillis(30)));

        var loader = new TestLoader(resources);

        var requests =
                resources.stream()
                        .map(entry -> entry.url)
                        .collect(Collectors.toCollection(ArrayList::new));
        Collections.shuffle(requests);

        var results = new CopyOnWriteArrayList<DataLoaderResult>();
        var completionCallback =
                new Future.FutureHandler<DataLoaderResult>() {
                    @Override
                    public void handleResult(Future<DataLoaderResult> res) throws Throwable {
                        assert (res.isReady());
                        results.add(res.get());
                    }
                };

        var noFuture = 0;
        for (var url : requests) {
            try {
                loader.loadDataAsync(url, null).then(completionCallback);
            } catch (RuntimeException e) {
                if (e.getMessage().equals("Just testing: error immediately")) {
                    noFuture++;
                } else {
                    fail(e);
                }
            }
        }
        var expectedResults = requests.size() - noFuture;

        // Busy wait.
        while (results.size() < expectedResults) {
            Thread.yield();
        }

        assertEquals(expectedResults, results.size());
        var resultData = new HashSet<String>();
        for (var result : results) {
            if (result.getStatus() != LoaderStatus.OK) {
                assertNotNull(result.getErrorCode());
                assertTrue(result.getErrorCode().contains("Just testing"));
                continue;
            }
            var data = result.getData();
            assertNotNull(data);
            assertTrue(data.isDirect(), "data buffer must be direct allocated");
            data.rewind();
            var buf = new byte[data.remaining()];
            data.get(buf);
            resultData.add(new String(buf));
        }

        assertEquals(Set.of("poof", "bang", "fizzle", "slowest"), resultData);
    }
}
