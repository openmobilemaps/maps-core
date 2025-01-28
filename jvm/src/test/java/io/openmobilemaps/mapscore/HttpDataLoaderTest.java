package io.openmobilemaps.mapscore;

import static org.junit.jupiter.api.Assertions.*;

import io.openmobilemaps.mapscore.map.loader.HttpDataLoader;
import io.openmobilemaps.mapscore.shared.map.loader.LoaderStatus;

import org.junit.jupiter.api.Test;

import java.util.concurrent.ExecutionException;

public class HttpDataLoaderTest {
    @Test
    public void loadDataOk() {
        var loader = new HttpDataLoader();
        var result =
                loader.loadData(
                        "https://openmobilemaps.io/images/openmobilemaps-logo-combined.svg", null);
        assertNotNull(result);
        assertEquals(result.getStatus(), LoaderStatus.OK);
        assertNotNull(result.getEtag());
        assertFalse(result.getEtag().isBlank());
        var data = result.getData();
        assertNotNull(data);
        assertTrue(data.isDirect(), "data buffer must be direct allocated");
        final var expectedPrefix = "<svg ";
        data.rewind();
        var buf = new byte[expectedPrefix.getBytes().length];
        data.get(buf);
        assertEquals(new String(buf), expectedPrefix);
    }

    @Test
    public void loadDataFail404() {
        var loader = new HttpDataLoader();
        var result =
                loader.loadData("https://openmobilemaps.io/this-file-does-not-exist.svg", null);
        assertNotNull(result);
        assertEquals(result.getStatus(), LoaderStatus.ERROR_404);
    }

    @Test
    public void loadTextureOk() {
        var loader = new HttpDataLoader();
        var result = loader.loadTexture("https://openmobilemaps.io/images/favicon.png", null);
        assertNotNull(result);
        assertEquals(result.getStatus(), LoaderStatus.OK);
        assertNotNull(result.getEtag());
        assertFalse(result.getEtag().isBlank());
        var data = result.getData();
        assertNotNull(data);
        assertEquals(data.getImageHeight(), 32);
        assertEquals(data.getImageWidth(), 32);
    }

    @Test
    public void loadTextureFailDecode() {
        var loader = new HttpDataLoader();
        // cannot load HTML document as an image...
        var result = loader.loadTexture("https://openmobilemaps.io/", null);
        assertNotNull(result);
        assertEquals(result.getStatus(), LoaderStatus.ERROR_OTHER);
    }

    @Test
    public void loadImageConcurrent() throws ExecutionException, InterruptedException {
        var imgUrl = "https://openmobilemaps.io/images/favicon.png";
        var loader = new HttpDataLoader();
        var futureA = loader.loadTextureAsync(imgUrl, null);
        var futureB = loader.loadTextureAsync(imgUrl, null);

        // concurrent load returns same result:
        var resultA = futureA.get();
        var resultB = futureB.get();
        assertEquals(resultA.getStatus(), LoaderStatus.OK);
        assertEquals(resultB.getStatus(), LoaderStatus.OK);
        assertSame(resultA.getData(), resultB.getData());

        // later load returns a new object:
        var futureC = loader.loadTextureAsync(imgUrl, null);
        var resultC = futureC.get();
        assertEquals(resultC.getStatus(), LoaderStatus.OK);
        assertNotSame(resultC.getData(), resultB.getData());
    }
}
