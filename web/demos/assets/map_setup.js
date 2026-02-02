import WebMapSDK from "../build/webmapsdk.js";

const statusElement = document.getElementById("status");
const progressElement = document.getElementById("progress");
const spinnerElement = document.getElementById("spinner");

let Module;
let Density;
let MapInterface;
let Loader;
const PixelRatio = window.devicePixelRatio;

// polyfill for requestIdleCallback for safari/**
// https://github.com/behnammodi/polyfill/blob/e27dd5a590ff8c2b9f4b34cf307e76efdc239e14/window.polyfill.js#L7-L24
if (!window.requestIdleCallback) {
  window.requestIdleCallback = (callback, options) => {
    options = options || {};
    const relaxation = 1;
    const timeout = options.timeout || relaxation;
    const start = performance.now();
    return setTimeout(() => {
      callback({
        get didTimeout() {
          return options.timeout
            ? false
            : performance.now() - start - relaxation > timeout;
        },
        timeRemaining: () =>
          Math.max(0, relaxation + (performance.now() - start)),
      });
    }, relaxation);
  };
}

// density set without considering device pixel ratio, will be multiplied
export async function initialize(canvasId, density, is3d = false) {
  const module = {
    print: (() => {
      const element = document.getElementById("output");
      if (element) element.value = ""; // clear browser cache
      return (...args) => {
        const text = args.join(" ");
        // These replacements are necessary if you render to raw HTML
        //text = text.replace(/&/g, "&amp;");
        //text = text.replace(/</g, "&lt;");
        //text = text.replace(/>/g, "&gt;");
        //text = text.replace('\n', '<br>', 'g');
        console.log(text);
        if (element) {
          element.value += `${text}\n`;
          element.scrollTop = element.scrollHeight; // focus on bottom
        }
      };
    })(),
    preInitializedWebGLContext: (() => {
      const canvas = document.getElementById("glCanvas");
      canvas.setAttribute("height", 600 * PixelRatio);
      canvas.setAttribute("width", 900 * PixelRatio);
      const gl = canvas.getContext("webgl2", {
        stencil: true,
        antialias: true,
      });
      console.log(gl.getSupportedExtensions());
      /*
    const ext = gl.getExtension("WEBGL_polygon_mode");
    ext.polygonModeWEBGL(gl.FRONT_AND_BACK, ext.LINE_WEBGL);
    */
      return gl;
    })(),
    canvas: (() => {
      const canvas = document.getElementById(canvasId);

      // As a default initial behavior, pop up an alert when webgl context is lost. To make your
      // application robust, you may want to override this behavior before shipping!
      // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
      canvas.addEventListener(
        "webglcontextlost",
        (e) => {
          alert("WebGL context lost. You will need to reload the page.");
          e.preventDefault();
        },
        false
      );

      return canvas;
    })(),
    onRuntimeInitialized: () => {
      console.log("Runtime initialized");
    },
    setStatus: (text) => {
      module.setStatus.last ??= { time: Date.now(), text: "" };
      if (text === module.setStatus.last.text) return;
      const m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
      const now = Date.now();
      if (m && now - module.setStatus.last.time < 30) return; // if this is a progress update, skip it if too soon
      module.setStatus.last.time = now;
      module.setStatus.last.text = text;
      if (m) {
        text = m[1];
        progressElement.value = Number.parseInt(m[2]) * 100;
        progressElement.max = Number.parseInt(m[4]) * 100;
        progressElement.hidden = false;
        spinnerElement.hidden = false;
      } else {
        progressElement.value = null;
        progressElement.max = null;
        progressElement.hidden = true;
        if (!text) spinnerElement.hidden = true;
      }
      statusElement.innerHTML = text;
    },
    totalDependencies: 0,
    monitorRunDependencies: (left) => {
      module.totalDependencies = Math.max(module.totalDependencies, left);
      module.setStatus(
        left
          ? `Preparing... (${module.totalDependencies - left}/${module.totalDependencies})`
          : "All downloads complete."
      );
    },
  };

  module.setStatus("Downloading...");
  window.onerror = () => {
    module.setStatus("Exception thrown, see JavaScript console");
    spinnerElement.style.display = "none";
    module.setStatus = (text) => {
      if (text) {
        console.error(`[post-exception status] ${text}`);
      }
    };
  };

  try {
    const wmsdkModule = await WebMapSDK(module);
    const scheduler = wmsdkModule.ThreadPoolScheduler.create();
    const mapCoordinateSystem = is3d
      ? wmsdkModule.CoordinateSystemFactory.getUnitSphereSystem()
      : wmsdkModule.CoordinateSystemFactory.getEpsg3857System();
    const mapInterface = wmsdkModule.MapInterface.createWithOpenGl(
      { mapCoordinateSystem },
      scheduler,
      density * PixelRatio,
      is3d
    );
    mapInterface.getRenderingContext().onSurfaceCreated();
    mapInterface.getRenderingContext().asOpenGlRenderingContext().resume();
    const loader = wmsdkModule.WebLoaderInterface.create();

    const canvas = document.getElementById(canvasId);
    function setViewportSize() {
      /* use {width,height}, not offset/client/scroll/...{width,height};
    This is the actual size of the canvas that we render to. If the
    computed sizes are different, the content is stretched. If we
    also use these as the viewport size here, the content will be
    doubly distorted and not fill the canvas correctly.
    */
      mapInterface.setViewportSize({ x: canvas.width, y: canvas.height });
    }
    setViewportSize();
    new ResizeObserver(setViewportSize).observe(canvas);

    // black bg for globes
    if (is3d) {
      mapInterface.setBackgroundColor({
        r: 0,
        g: 0,
        b: 0,
        a: 1,
      });
    } else {
      mapInterface.setBackgroundColor({
        r: 0.8,
        g: 0.8,
        b: 0.8,
        a: 1,
      });
    }

    Module = wmsdkModule;
    Density = density;
    MapInterface = mapInterface;
    Loader = loader;

    return { mapInterface, wmsdkModule, loader };
  } catch (err) {
    console.error("Failed to load WebMapSDK", err);
  }
}

export function registerAnimationLoop() {
  if (!MapInterface || !Module) {
    throw new Error("Map not initialized, run 'initialize' first");
  }

  const invalidationCallback = Module.WebMapCallbackInterface.create();
  MapInterface.setCallbackHandler(invalidationCallback.asCallbackInterface());

  //// Main loop(s)
  // - process load requests enqueued by worker threads; trigger an
  //  asynchronous load for each request; the callbacks will occur on this
  //  thread
  // - draw frames
  function idleProcessLoadRequests() {
    Loader.processLoadRequests();
    window.requestIdleCallback(idleProcessLoadRequests, { timeout: 10 });
  }
  window.requestIdleCallback(idleProcessLoadRequests, { timeout: 10 });

  // TODO: it would be nicer to really interrupt the
  // requestAnimationFrame-drawing-loop when the scene is not being
  // invalidated. We'd could then re-start the drawing-loop with a postMessage
  // from the workers.
  // - Multi-canvas / multi-map setups? Ensure that message would be addressed
  //   to the correct instance.
  // - Investigate if this doesnt add more latency before drawing.
  //
  function animationFrameDraw(timestamp) {
    if (invalidationCallback.getAndResetInvalid()) {
      MapInterface.prepare();
      MapInterface.drawFrame();
    }
    window.requestAnimationFrame(animationFrameDraw);
  }
  window.requestAnimationFrame(animationFrameDraw);
}

export function registerTouchHandler() {
  if (!MapInterface || !Density || !Module) {
    throw new Error("Map not initialized, run 'initialize' first");
  }

  // setup touch handler
  const touchHandler = MapInterface.getTouchHandler();

  // Track if mouse is down to handle move/up events properly
  let isMouseDown = false;

  Module.canvas.addEventListener(
    "mousedown",
    (e) => {
      // Get coordinates relative to canvas
      const rect = Module.canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;

      // console.log("mousedown", e);
      isMouseDown = true;
      touchHandler.onTouchEvent({
        pointers: [{ x: x * PixelRatio, y: y * PixelRatio }],
        scrollDelta: 0,
        touchAction: Module.TouchAction.DOWN,
      });
      e.preventDefault();
    },
    false
  );

  // Add mousemove listener to document to capture events outside canvas
  document.addEventListener(
    "mousemove",
    (e) => {
      if (!isMouseDown) return; // Only process if mouse button is down

      // Get coordinates relative to canvas
      const rect = Module.canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;

      // console.log("mousemove", x, y);
      touchHandler.onTouchEvent({
        pointers: [{ x: x * PixelRatio, y: y * PixelRatio }],
        scrollDelta: 0,
        touchAction: Module.TouchAction.MOVE,
      });
    },
    false
  );

  // Add mouseup listener to document to capture events outside canvas
  document.addEventListener(
    "mouseup",
    (e) => {
      if (!isMouseDown) return; // Only process if mouse button is down
      isMouseDown = false;

      // Get coordinates relative to canvas
      const rect = Module.canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;

      // console.log("mouseup", x, y);
      touchHandler.onTouchEvent({
        pointers: [{ x: x * PixelRatio, y: y * PixelRatio }],
        scrollDelta: 0,
        touchAction: Module.TouchAction.UP,
      });
    },
    false
  );

  Module.canvas.addEventListener(
    "touchcancel",
    (e) => {
      touchHandler.onTouchEvent({
        pointers: [...e.touches],
        scrollDelta: 0,
        touchAction: Module.TouchAction.CANCEL,
      });
      e.preventDefault();
    },
    false
  );

  Module.canvas.addEventListener(
    "wheel",
    (e) => {
      // Get coordinates relative to canvas
      const rect = Module.canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;

      const delta = e.wheelDelta;
      // console.log("scroll", x, y, delta);
      if (delta) {
        touchHandler.onTouchEvent({
          pointers: [{ x: x * PixelRatio, y: y * PixelRatio }],
          scrollDelta: delta / 2,
          touchAction: Module.TouchAction.SCROLL,
        });
      }

      e.preventDefault();
    },
    false
  );

  Module.canvas.addEventListener(
    "mousewheelend",
    (e) => {
      touchHandler.onTouchEvent({
        pointers: [],
        scrollDelta: 0,
        touchAction: Module.TouchAction.CANCEL,
      });
      e.preventDefault();
    },
    false
  );
}
