package com.example.cloudstorageapp;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.os.Bundle;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileFilter;

public class FileSystemActivity extends AppCompatActivity {

    final int MY_PERMISSIONS_REQUEST_ALL_PERM = 1;

    private String ROOT_DIRECTORY = "";
    private File currentDirectory = null;
    private LinearLayout mainLayout = null;

    private final int CONTEXT_MENU_REMOVE = 2;
    private final int CONTEXT_MENU_DOWNLOAD = 3;

    public static float convertPixelsToDp(float px) {
        DisplayMetrics metrics = Resources.getSystem().getDisplayMetrics();
        float dp = px / (metrics.densityDpi / 160f);
        return Math.round(dp);
    }

    public static float convertDpToPixel(float dp) {
        DisplayMetrics metrics = Resources.getSystem().getDisplayMetrics();
        float px = dp * (metrics.densityDpi / 160f);
        return Math.round(px);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_file_system);

        getSupportActionBar().hide();

        this.ROOT_DIRECTORY = Environment.getExternalStorageDirectory().getAbsolutePath();

        String[] PERMISSIONS = {
                Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
        };

        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED)
        {
            ActivityCompat.requestPermissions(
                    this,
                    PERMISSIONS,
                    MY_PERMISSIONS_REQUEST_ALL_PERM
            );
        }
        else
        {
            File extStorage = Environment.getExternalStorageDirectory();

            if (extStorage.exists()) {
                Toast.makeText(getApplicationContext(), "Upload from: " + extStorage.getAbsolutePath(), Toast.LENGTH_LONG).show();
                // should print storage/emulated/0 directory
                this.printDirectory(extStorage.getAbsolutePath());
            }
            else
            {
                Toast.makeText(getApplicationContext(), "No external storage", Toast.LENGTH_LONG).show();
                finish();
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults)
    {
        switch (requestCode)
        {
            case MY_PERMISSIONS_REQUEST_ALL_PERM:
            {
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
                {
                    Toast.makeText(getApplicationContext(), "Access Granded", Toast.LENGTH_SHORT).show();

                    File extStorage = Environment.getExternalStorageDirectory();

                    if (extStorage.exists())
                    {
                        Toast.makeText(getApplicationContext(), "Upload from: " + extStorage.getAbsolutePath(), Toast.LENGTH_LONG).show();
                        // should print storage/emulated/0 directory
                        this.printDirectory(extStorage.getAbsolutePath());
                    }
                    else
                    {
                        Toast.makeText(getApplicationContext(), "No external storage", Toast.LENGTH_LONG).show();
                        finish();
                    }
                }
                else
                {
                    Toast.makeText(getApplicationContext(), "No storage access permission", Toast.LENGTH_LONG).show();
                    finish();
                }
            }
        }
    }

    public void printDirectory(String directoryPath) {
        // R.id.directory_layout

        if (this.mainLayout == null)
            this.mainLayout = (LinearLayout) findViewById(R.id.directory_layout);
        else this.mainLayout.removeAllViews();

        File directory = new File(directoryPath);
        this.currentDirectory = directory;

        LinearLayout.LayoutParams generalParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);

        generalParams.setMargins(
                (int) convertDpToPixel(5),
                (int) convertDpToPixel(5),
                (int) convertDpToPixel(5),
                (int) convertDpToPixel(10)
        );

        Log.d("MYTAG", "TRYING");

        int idCount = 0;


        File[] dirs = directory.listFiles(new FileFilter() {
            @Override
            public boolean accept(File file) {
                return file.isDirectory();
            }
        });

        File[] files = directory.listFiles(new FileFilter() {
            @Override
            public boolean accept(File file) {
                return file.isFile();
            }
        });

        if (dirs.length != 0)
        {
            for (final File i : dirs)
            {
                Log.d("MYTAG", (i.isFile() ? "File: " : "Dir: ") + '/' + i.getName());
                TextView nTextView = new TextView(getApplicationContext());

                nTextView.setText(i.getName());
                nTextView.setBackgroundColor(0x60018577);
                nTextView.setPadding((int) convertDpToPixel(10), (int) convertDpToPixel(10), (int) convertDpToPixel(10), (int) convertDpToPixel(10));
                nTextView.setLayoutParams(generalParams);

                nTextView.setOnClickListener(new View.OnClickListener()
                {
                    @Override
                    public void onClick(View view)
                    {
//                    Toast.makeText(getApplicationContext(), "You click on " + i.getName(), Toast.LENGTH_SHORT).show();
                        printDirectory(i.getAbsolutePath());
                    }
                });

                this.mainLayout.addView(nTextView);

                nTextView.setId(idCount);
                idCount++;
            }
        }
        if (files.length != 0)
        {
            for (final File i : files)
            {
                Log.d("MYTAG", (i.isFile() ? "File: " : "Dir: ") + '/' + i.getName());
                TextView nTextView = new TextView(getApplicationContext());

                nTextView.setText(i.getName());
                nTextView.setBackgroundColor(0x20018577);
                nTextView.setPadding((int) convertDpToPixel(10), (int) convertDpToPixel(10), (int) convertDpToPixel(10), (int) convertDpToPixel(10));
                nTextView.setLayoutParams(generalParams);

                nTextView.setOnClickListener(new View.OnClickListener()
                {
                    @Override
                    public void onClick(View view)
                    {
//                    Toast.makeText(getApplicationContext(), "You click on " + i.getName(), Toast.LENGTH_SHORT).show();
                        Intent resultIntent = new Intent();
                        resultIntent.putExtra("file_path", i.getAbsolutePath());
                        setResult(RESULT_OK, resultIntent);
                        finish();
                    }
                });

                this.mainLayout.addView(nTextView);

                nTextView.setId(idCount);
                idCount++;
            }
        }

        if (dirs.length == 0 && files.length == 0)
        {
            TextView nTextView = new TextView(getApplicationContext());

            nTextView.setText("empty folder");
            nTextView.setPadding((int) convertDpToPixel(10), (int) convertDpToPixel(10), (int) convertDpToPixel(10), (int) convertDpToPixel(10));
            nTextView.setTextAlignment(View.TEXT_ALIGNMENT_CENTER);
            nTextView.setLayoutParams(
                    new LinearLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                            ViewGroup.LayoutParams.MATCH_PARENT)
            );
            this.mainLayout.addView(nTextView);
        }
    }

    @Override public void onBackPressed() {
        if (this.currentDirectory != null &&
                !this.currentDirectory.getAbsolutePath().equals(this.ROOT_DIRECTORY)
        )
        {
            printDirectory(this.currentDirectory.getParentFile().getAbsolutePath());
        }
        else {
            Intent resultIntent = new Intent();
            resultIntent.putExtra("file_path", "");
            setResult(RESULT_CANCELED, resultIntent);
            finish();
        }
    }
}