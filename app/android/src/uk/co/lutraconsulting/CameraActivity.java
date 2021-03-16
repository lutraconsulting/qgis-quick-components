package uk.co.lutraconsulting;

import java.text.SimpleDateFormat;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.stream.Collectors;

import android.os.Bundle;
import android.os.Environment;
import android.net.Uri;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;
import android.app.Activity;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.provider.MediaStore;
import android.graphics.Bitmap;
import android.support.v4.content.FileProvider;

import uk.co.lutraconsulting.EXIFUtils;
// Sensors
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

public class CameraActivity extends Activity implements SensorEventListener {
    private static final String TAG = "Camera Activity";
    private static final int CAMERA_CODE = 102;

    private String targetPath;
    private File cameraFile;

    // Sensors
    int SENSOR_DELAY_MS = 5000;
    int SENSOR_DELAY = SensorManager.SENSOR_DELAY_NORMAL; //SENSOR_DELAY_MS * 1000;
    private SensorManager mSensorManager;
    private Sensor mSensorAccelerometer;
    private Sensor mSensorMagnetometer;

    // Current data from accelerometer & magnetometer.  The arrays hold values
    // for X, Y, and Z.
    private float[] mAccelerometerData = new float[3];
    private float[] mMagnetometerData = new float[3];
    // stores time -> azimuth (degree) for a whole run of the activity
    private HashMap<Long, Double> azimuthData = new HashMap<Long, Double>();
    private HashMap<Long, Double> pitchData = new HashMap<Long, Double>();
    private HashMap<Long, Double> rollData = new HashMap<Long, Double>();
    private HashMap<Long, Double> cameraAngleData = new HashMap<Long, Double>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate()");
        super.onCreate(savedInstanceState);

        // Init sensors
        mSensorManager = (SensorManager) getSystemService(
                Context.SENSOR_SERVICE);
        mSensorAccelerometer = mSensorManager.getDefaultSensor(
                Sensor.TYPE_ACCELEROMETER);
        mSensorMagnetometer = mSensorManager.getDefaultSensor(
                Sensor.TYPE_MAGNETIC_FIELD);

        targetPath = getIntent().getExtras().getString("targetPath");
        Log.d(TAG, "targetPath: " + targetPath);

        Intent takePictureIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
        if (takePictureIntent.resolveActivity(getPackageManager()) != null) {

            File photoFile = null;
            try {
                photoFile = createImageFile(targetPath);
            } catch (IOException ex) {
                // Handled in else branch since photoFile == null;
            }
            // Continue only if the File was successfully created
            if (photoFile != null) {
                Uri photoURI = FileProvider.getUriForFile(this,
                        "uk.co.lutraconsulting.fileprovider",
                        photoFile);

                takePictureIntent.putExtra(MediaStore.EXTRA_OUTPUT, photoURI);
                takePictureIntent.putExtra("__RESULT__", "takePictureIntent__RESULT__");
                startActivityForResult(takePictureIntent, CAMERA_CODE);
            } else {
                Intent activityIntent = getIntent();
                activityIntent.putExtra("__RESULT__", "Cannot access target path.");
                setResult(Activity.RESULT_CANCELED, activityIntent);
                finish();
            }
        }

        return;
    }

    /**
     * Listeners for the sensors are registered in this callback so that
     * they can be unregistered in onStop().
     */
    @Override
    protected void onStart() {
        super.onStart();

        if (mSensorAccelerometer != null) {
            mSensorManager.registerListener(this, mSensorAccelerometer,
                    SENSOR_DELAY);
        }
        if (mSensorMagnetometer != null) {
            mSensorManager.registerListener(this, mSensorMagnetometer,
                    SENSOR_DELAY);
        }
    }

    @Override
    public void onSensorChanged(SensorEvent sensorEvent) {
        // The sensor type (as defined in the Sensor class).
        int sensorType = sensorEvent.sensor.getType();

        // The sensorEvent object is reused across calls to onSensorChanged().
        // clone() gets a copy so the data doesn't change out from under us
        switch (sensorType) {
            case Sensor.TYPE_ACCELEROMETER:
                mAccelerometerData = sensorEvent.values.clone();
                break;
            case Sensor.TYPE_MAGNETIC_FIELD:
                mMagnetometerData = sensorEvent.values.clone();
                break;
            default:
                return;
        }
        // Compute the rotation matrix: merges and translates the data
        // from the accelerometer and magnetometer, in the device coordinate
        // system, into a matrix in the world's coordinate system.
        //
        // The second argument is an inclination matrix, which isn't
        // used in this example.
        float[] rotationMatrix = new float[9];
        boolean rotationOK = SensorManager.getRotationMatrix(rotationMatrix,
                null, mAccelerometerData, mMagnetometerData);

        // Remap the matrix based on current device/activity rotation.
        float[] rotationMatrixAdjusted = new float[9];
        rotationMatrixAdjusted = rotationMatrix.clone();

        // Get the orientation of the device (azimuth, pitch, roll) based
        // on the rotation matrix. Output units are radians.
        float orientationValues[] = new float[3];
        if (rotationOK) {
            SensorManager.getOrientation(rotationMatrixAdjusted,
                    orientationValues);
        }

        // Pull out the individual values from the array.
        float azimuth = orientationValues[0];
        float pitch = orientationValues[1];
        float roll = orientationValues[2];

        // angle in degree [0 - 360] degree
        double degrees = toDegrees(orientationValues[0]); // angle of rotation about the -z axis
        double pitchDegrees = toDegrees(orientationValues[1]); // angle of rotation about the x axis
        double rollDegrees = toDegrees(orientationValues[2]); // angle of rotation about the y axis

        // Store sensor values
        double normDegrees = adjustDegreesToScreenOrientation(degrees);
        double cameraAngle = adjustCameraAngleOrientation(pitchDegrees, rollDegrees);

        azimuthData.put(System.currentTimeMillis(), normDegrees);
        pitchData.put(System.currentTimeMillis(), pitchDegrees);
        rollData.put(System.currentTimeMillis(), rollDegrees);
        cameraAngleData.put(System.currentTimeMillis(), cameraAngle);
    }


    /**
     * Must be implemented to satisfy the SensorEventListener interface;
     * unused in this app.
     */
    @Override
    public void onAccuracyChanged(Sensor sensor, int i) {
    }

    private File createImageFile(String targetPath) throws IOException {
        // Create an image file name
        String currentPhotoPath;
        String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        String imageFileName = "JPEG_" + timeStamp + "_";

        cameraFile = File.createTempFile(
                imageFileName, /* prefix */
                ".jpg", /* suffix */
                getCacheDir() /* directory */
        );

        // Save a file: path for use with ACTION_VIEW intents
        currentPhotoPath = cameraFile.getAbsolutePath();
        Log.d(TAG, "currentPhotoPath: " + currentPhotoPath);
        return cameraFile;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "onActivityResult()");
        Log.d(TAG, "request: " + requestCode);
        Log.d(TAG, "resultCode: " + resultCode);
        mSensorManager.unregisterListener(this);

        if (requestCode == CAMERA_CODE && resultCode == Activity.RESULT_OK) {
            Log.d(TAG, "tmp exists: " + cameraFile.exists());
            Log.d(TAG, "tmp path: " + cameraFile.getAbsolutePath());

            try {
                extendGPSExifData(cameraFile.lastModified());
                copyFile(cameraFile, new File(targetPath, cameraFile.getName()));
                if (data == null) {
                    data = getIntent();
                }
                data.putExtra("__RESULT__", cameraFile.getAbsolutePath());
                setResult(Activity.RESULT_OK, data);

            } catch (IOException e) {
                Intent intent = this.getIntent();
                if (data == null) {
                    data = getIntent();
                }
                data.putExtra("__RESULT__", e.getMessage());
                setResult(Activity.RESULT_CANCELED, data);
            }

            // TODO: after copy, verify if is correctly copied and then remove the old one
        }
        finish();

        // TODO
        azimuthData.clear();
        pitchData.clear();
        rollData.clear();
        cameraAngleData.clear();
    }

    private void extendGPSExifData(long captureTime) {
        double degrees = getValueByTime(azimuthData, captureTime);
        double cameraAngle = getValueByTime(cameraAngleData, captureTime);
        double pitch = getValueByTime(pitchData, captureTime);
        double roll = getValueByTime(rollData, captureTime);

        EXIFUtils.writeExifGpsDirection(cameraFile.getAbsolutePath(), degrees, cameraAngle, pitch, roll);
    }

    private double getValueByTime(HashMap<Long, Double> data, long time) {
        List<Double> result = data.entrySet().stream()
                .filter(x -> Math.abs(x.getKey() - time) <= SENSOR_DELAY_MS)
                .map(x->x.getValue())
                .collect(Collectors.toList());
        if (result.isEmpty()) return -1;
        return result.get(0);
    }

    private double toDegrees(double value) {
        return (Math.toDegrees(value) + 360) % 360;
    }

    // Angle of the device according to surface (camera pointing to surface has 0 degree)
    private double adjustDegreesToScreenOrientation(double degrees) {
        Display display = ((WindowManager) getApplicationContext().getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        switch (display.getRotation()) {
            case Surface.ROTATION_0: {
                return normalizeDegree(degrees);
            }
            case Surface.ROTATION_90: {
                return normalizeDegree(degrees + 90);
            }
            case Surface.ROTATION_180: {
                return normalizeDegree(degrees + 180);
            }
            case Surface.ROTATION_270: {
                return normalizeDegree(degrees + 270);
            }
            default: return degrees;
        }
    }

    private double adjustCameraAngleOrientation(double pitch, double roll) {
        Display display = ((WindowManager) getApplicationContext().getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        switch (display.getRotation()) {
            case Surface.ROTATION_0: {
                return (360 - pitch);
            }
            case Surface.ROTATION_90: {
                return (360 - roll);
            }
            case Surface.ROTATION_180: {
                return pitch;
            }
            case Surface.ROTATION_270: {
                return roll;
            }
            default: return pitch;
        }
    }

    private double normalizeDegree(double degree) {
        while (degree > 360)
            degree = degree - 360;
        return degree;
    }

    private void copyFile(File src, File dst) throws IOException {
        Log.d(TAG, "Copied file: " + src.getAbsolutePath() + " to file: " + dst.getAbsolutePath());
        InputStream in = null;
        OutputStream out = null;

        try {
            in = new FileInputStream(src);
            out = new FileOutputStream(dst);
            // Transfer bytes from in to out
            byte[] buf = new byte[1024];
            int len;
            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
        } catch (IOException e) {
            throw new IOException("Cannot copy a photo to working directory.");
        } finally {
            if (in != null)
                in.close();
            if (out != null)
                out.close();
        }

    }
}
