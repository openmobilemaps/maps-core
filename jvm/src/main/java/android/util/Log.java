package android.util;


import org.slf4j.LoggerFactory;

/**
 * Dummy implementation of android.util.Log. This is used in the Djinni Java support lib. To allow
 * us to use this without changes, we provide a dummy definition of this class.
 */
public class Log {
    public static void e(String tag, String msg, Exception e) {
        LoggerFactory.getLogger(tag).error(msg, e);
    }
}
