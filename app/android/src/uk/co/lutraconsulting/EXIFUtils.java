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
  // GPS EXIF TAGS
  private static final String GPS_BEARING_TAG = "GPSDestBearing";
  private static final String GPS_BEARING_REF_TAG = "GPSDestBearingRef";
  private static final String GPS_LON_TAG = "GPSLongitude";
  private static final String GPS_LAT_TAG = "GPSLatitude";
  private static final String GPS_DATE_TAG = "GPSDateStamp";

  public static void writeExifGpsDirection(String src, double exifOrientation) {
    HashMap<String, String> attributes = new HashMap<String, String>();
    attributes.put(GPS_BEARING_TAG, calculateRational(exifOrientation));
    attributes.put(GPS_BEARING_REF_TAG, "M");
    writeExifAttributes(src, attributes);
  }

  public static void writeExifAttributes(String src, HashMap<String, String> attributes) {
    Log.d(TAG, "WriteExif: " + src + " to file: ");
    InputStream in = null;
    try {
      ExifInterface exifInterface = new ExifInterface(src);
      Log.d(TAG, "Set attributes:");
      for (String key: attributes.keySet()) {
        exifInterface.setAttribute(key, attributes.get(key));
      }
      exifInterface.saveAttributes();
    } catch (IOException e) {
      Log.d(TAG, "WriteExif ERROR: " + e.getMessage());
      // Handle any errors
    } finally {
      if (in != null) {
        try {
          in.close();
        } catch (IOException ignored) {
        }
      }
    }
  }

  // TODO
  public static void readExif(String src) throws IOException {
    Log.d(TAG, "readExif: " + src + " to file: ");
    InputStream in = null;
    try {
      in = new FileInputStream(src);
      ExifInterface exifInterface = new ExifInterface(in);
      String date = exifInterface.getAttribute("GPSDateStamp");
      String lat = exifInterface.getAttribute("GPSDestLatitude");
      String lon = exifInterface.getAttribute("GPSDestLongitude");
      String lat2 = exifInterface.getAttribute("GPSLatitude");
      String lon2 = exifInterface.getAttribute("GPSLongitude");
      String dir = exifInterface.getAttribute("GPSImgDirection");
      String dir2 = exifInterface.getAttribute("M");
      String dir3 = exifInterface.getAttribute("T");
      String dir4 = exifInterface.getAttribute("GPSTrack");
      String dir5 = exifInterface.getAttribute("GPSDestBearing");
      String dir5Ref = exifInterface.getAttribute("GPSDestBearingRef");
      String dir6 = exifInterface.getAttribute("GPSBearing");
      String gpsInfo = exifInterface.getAttribute("GPSSatellites");

      Log.d(TAG, "readExif LAT: " + lat2);
      Log.d(TAG, "readExif LON: " + lon2);
      Log.d(TAG, "readExif DIR: " + dir + "," + dir2 + "," + dir3 + "," + dir4 + "," + dir5 + "," + dir6);
      Log.d(TAG, "readExif INFO: " + gpsInfo);
      Log.d(TAG, "readExif TEST: " + dir5 + "|" + dir5Ref);


      Log.d(TAG, "readExif REF: " + exifInterface.getAttribute("GPSImgDirectionRef"));
      Log.d(TAG, "readExif REF: " + exifInterface.getAttribute("GPSTrackRef"));
      Log.d(TAG, "readExif REF: " + exifInterface.getAttribute("GPSDestBearingRef"));

      Log.d(TAG, "readExif: " + src + " to file: ");
      // Now you can extract any Exif tag you want
      // Assuming the image is a JPEG or supported raw format
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

    // TODO if exigTags empty
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

  private static String calculateRational(double d) {

    double rounded = Math.round(d * 100)/100; // TODO proper precision
    String aString = Double.toString(rounded);
    String[] fraction = aString.split("\\.");

    int denominator = (int)Math.pow(10, fraction[1].length());
    int numerator = Integer.parseInt(fraction[0] + "" + fraction[1]);

    Log.d(TAG, "calculateRational: " + numerator + "/" + denominator + "==" + d);
    return numerator + "/" + denominator;
  }
}
