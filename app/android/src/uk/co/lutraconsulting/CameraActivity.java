package uk.co.lutraconsulting;

import java.text.SimpleDateFormat;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.util.Date;

import android.os.Bundle;
import android.os.Environment;
import android.net.Uri;
import android.app.Dialog;
import android.app.AlertDialog;
import android.app.Activity;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.Context;
import android.content.Intent;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ImageView;
import android.widget.Button;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.LayoutInflater;
import android.util.Log;
import android.graphics.drawable.ColorDrawable;
import android.provider.MediaStore;
import android.graphics.Bitmap;
import android.support.v4.content.FileProvider;

public class CameraActivity extends Activity{
    private static final String TAG = "Camera Activity";
    private static final int CAMERA_CODE = 102;
    private String targetPath;
    private File cameraFile;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate()");
        super.onCreate(savedInstanceState);

        targetPath = getIntent().getExtras().getString("targetPath");
        Log.d(TAG, "targetPath: "+ targetPath);
        
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

        //throw new IOException("Tralalal!#@#$@JAVA");

        return;
    }

    private File createImageFile(String targetPath) throws IOException {
        // Create an image file name
        String currentPhotoPath;
        String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        String imageFileName = "JPEG_" + timeStamp + "_";
        
        cameraFile = File.createTempFile(imageFileName,  /* prefix */
                                         ".jpg",         /* suffix */
                                         getCacheDir()      /* directory */
                                         );

        // Save a file: path for use with ACTION_VIEW intents
        currentPhotoPath = cameraFile.getAbsolutePath();
        String  exif; 
        ExifInterface  = ExifInterface(imageFileName.getAbsolutePath());
        exif.setAttribute(ExifInterface.TAG_ORIENTATION, "" + getPhotoOrientation(configurationProvider.getSensorPosition()));
        exif.saveAttributes();
        return cameraFile;
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        Log.d(TAG, "onActivityResult()");
        Log.d(TAG, "request: "+requestCode);
        Log.d(TAG, "resultCode: "+resultCode);
        if (requestCode == CAMERA_CODE && resultCode == Activity.RESULT_OK) {

            Log.d(TAG, "tmp exists: "+cameraFile.exists());
            Log.d(TAG, "tmp path: "+cameraFile.getAbsolutePath());
            try{
                copyFile(cameraFile, new File(targetPath, cameraFile.getName()));
                if (data == null) {
                    data = getIntent();                
                }
                data.putExtra("__RESULT__", cameraFile.getAbsolutePath());
                setResult(Activity.RESULT_OK, data);   
                
            }catch(IOException e){
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
    }

    private void copyFile(File src, File dst) throws IOException {
        Log.d(TAG, "Copied file: "+src.getAbsolutePath()+" to file: "+dst.getAbsolutePath());
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
        } catch(IOException e) {
            throw new IOException("Cannot copy a photo to working directory.");
        }
        finally {
          if (in != null)
            in.close();
          if (out != null)
            out.close();
        }
        
    }    

}

