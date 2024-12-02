package android.util;

import java.util.logging.Logger;

/**
 * Dummy implementation of android.util.Log. This is used in the Djinni Java support lib. To allow
 * us to use this without changes, we provide a dummy definition of this class.
 */
public class Log {
    public static void e(String tag, String msg, Exception e) {
        Logger.getLogger(tag).severe(msg + ": " + e.toString());
    }
}
