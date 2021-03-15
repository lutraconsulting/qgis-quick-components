package uk.co.lutraconsulting;

import android.os.Bundle;
import androidx.exifinterface.media.ExifInterface;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.util.Date;
import java.util.HashMap;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;
import android.util.Log;

public class EXIFUtils
{
    private static final String TAG = "EXIF Utils";
    private static final int DEGREE_PRECISION = 2;
    // GPS EXIF TAGS
    private static final String GPS_BEARING_TAG = "GPSDestBearing";
    private static final String GPS_BEARING_REF_TAG = "GPSDestBearingRef";
    private static final String GPS_LON_TAG = "GPSLongitude";
    private static final String GPS_LAT_TAG = "GPSLatitude";
    private static final String GPS_DATE_TAG = "GPSDateStamp";

    public static void writeExifGpsDirection(String src, double exifOrientation) {
        if (exifOrientation >= 0 && exifOrientation <= 360) {
            HashMap<String, String> attributes = new HashMap<String, String>();
            attributes.put(GPS_BEARING_TAG, calculateRational(exifOrientation));
            attributes.put(GPS_BEARING_REF_TAG, "M");
            writeExifAttributes(src, attributes);
        }
    }

    public static void writeExifAttributes(String src, HashMap<String, String> attributes) {
        Log.d(TAG, "WriteExif: " + src + " to file: ");
        InputStream in = null;
        try {
            ExifInterface exifInterface = new ExifInterface(src);
            for (String key: attributes.keySet()) {
                exifInterface.setAttribute(key, attributes.get(key));
            }
            exifInterface.saveAttributes();
        } catch (IOException e) {
            // Handle any errors
            Log.d(TAG, "WriteExif ERROR: " + e.getMessage());
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException ignored) {
                }
            }
        }
    }

    public static HashMap<String, String> getExifGpsAttributes(String filepath) {
        List<String> exifTags = new ArrayList<String>();
        exifTags.add(GPS_BEARING_TAG);
        exifTags.add(GPS_LAT_TAG);
        exifTags.add(GPS_LON_TAG);
        exifTags.add(GPS_DATE_TAG);

        return getEXIFdata(filepath, exifTags);
    }

    public static String getEXIFAttribute(String filepath, String tag) {
        Log.d(TAG, "getEXIFAttribute: " + filepath + " - " + tag);
        List<String> exifTags = new ArrayList<String>();
        exifTags.add(tag);
        return getEXIFdata(filepath, exifTags).get(tag);
    }

    public static HashMap<String, String> getEXIFdata(String filepath, List<String> exifTags) {
        Log.d(TAG, "getEXIFdata: " + filepath + " to file: ");
        InputStream in = null;
        HashMap<String, String> result = new HashMap<String, String>();

        if (exifTags.isEmpty()) return result;

        try {
            in = new FileInputStream(filepath);
            ExifInterface exifInterface = new ExifInterface(in);
            for (String tag : exifTags) {
                String value = exifInterface.getAttribute(tag);
                result.put(tag, value);
                Log.d(TAG, "getEXIFdata: " + tag + " -> " + value);
            }
        } catch (IOException e) {
            // Handle any errors
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException ignored) {
                }
            }
        }
        return result;
    }

    // Converts double value to rational string representation
    private static String calculateRational(double d) {

        double precision_multiplier = Math.pow(10, DEGREE_PRECISION);
        double rounded = Math.round(d * precision_multiplier)/precision_multiplier;
        String[] fraction = Double.toString(rounded).split("\\.");

        int denominator = (int)Math.pow(10, fraction[1].length());
        int numerator = Integer.parseInt(fraction[0] + "" + fraction[1]);

        return numerator + "/" + denominator;
    }
}
