package com.example.cloudstorageapp;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.ContextMenu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONObject;

import java.io.File;
import java.util.Iterator;

public class MainActivity extends AppCompatActivity implements RequestHandler
{
    private static final int REQ_CODE_LOGIN_ACTIVITY = 42;
    private static final int REQ_CODE_FS_ACTIVITY = 43;

    private final int MY_PERMISSIONS_REQUEST_ALL_PERM = 1;

    private final int CONTEXT_MENU_REMOVE = 2;
    private final int CONTEXT_MENU_DOWNLOAD = 3;

    private TextView currentPath = null;
    private String strCurrentPath = "";

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        currentPath = (TextView) findViewById(R.id.textViewPath);
        currentPath.setText("#");

        getSupportActionBar().hide();

        CloudStorageHttpClient.startSession();

        String[] PERMISSIONS = {
                Manifest.permission.INTERNET,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.READ_EXTERNAL_STORAGE
        };

        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.INTERNET)
                != PackageManager.PERMISSION_GRANTED
                || ActivityCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED
                || ActivityCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED
        )
        {
            ActivityCompat.requestPermissions(
                    this,
                    PERMISSIONS,
                    MY_PERMISSIONS_REQUEST_ALL_PERM
            );
        }
        else
        {
            Intent intentLoginActivity = new Intent(this, LoginActivity.class);
            startActivityForResult(intentLoginActivity, REQ_CODE_LOGIN_ACTIVITY);
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

                    Intent intentLoginActivity = new Intent(this, LoginActivity.class);
                    startActivityForResult(intentLoginActivity, REQ_CODE_LOGIN_ACTIVITY);
                }
                else
                {
                    Toast.makeText(getApplicationContext(), "No internet permission", Toast.LENGTH_LONG).show();
                    finish();
                }
            }
        }
    }

    public void buttonUploadClicked(View view)
    {
        Intent intentFileSystemActivity = new Intent(this, FileSystemActivity.class);
        startActivityForResult(intentFileSystemActivity, REQ_CODE_FS_ACTIVITY);
    }

    public void buttonSendClicked(View view)
    {
        EditText editTextURL = (EditText) findViewById(R.id.editTextNewDir);
        String strURL = editTextURL.getText().toString();

        CloudStorageHttpClient.request(this);
    }

    public void buttonListClicked(View view)
    {
        EditText editTextURL = (EditText) findViewById(R.id.editTextNewDir);

        String strPath = editTextURL.getText().toString();

        /*###*/
        CloudStorageHttpClient.list(strPath, this);
    }

    public void listServerDir(String strPath)
    {
        CloudStorageHttpClient.list(strPath, this);
    }

    public void buttonMkDirClicked(View view)
    {
        EditText editTextNewDir = (EditText) findViewById(R.id.editTextNewDir);

        String strDir = editTextNewDir.getText().toString();

        CloudStorageHttpClient.mkdir(strCurrentPath + "/" + strDir, this);
    }

    public void buttonDownloadClicked(View view)
    {
        EditText editTextURL = (EditText) findViewById(R.id.editTextNewDir);
        String strFullName = editTextURL.getText().toString();

        /*###*/
        CloudStorageHttpClient.download("/hello.jpg", "hello.jpg",this);
    }


    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data)
    {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode)
        {
            case REQ_CODE_LOGIN_ACTIVITY:
            {
                if (resultCode == RESULT_OK)
                {
                    Log.d("MYTAG", "onActivityResult: LOGIN: RESULT_OK");
                    String strExtraData = data.getStringExtra("login_stat");
                    Toast.makeText(this, "Login Success!", Toast.LENGTH_SHORT).show();
                    listServerDir("/");
                }
                else
                {
                    Log.d("MYTAG", "onActivityResult: LOGIN: RESULT_ERROR");
                }
            }
            break;

            case REQ_CODE_FS_ACTIVITY:
            {
                if (resultCode == RESULT_OK)
                {
                    String strFilePath = data.getStringExtra("file_path");
                    Log.d("MYTAG", "onActivityResult: RESULT_OK: " + strFilePath);
                    //strFilePath, "/0RiGinAln0.txt"

                    File f = new File(strFilePath);
                    CloudStorageHttpClient.upload(strFilePath, strCurrentPath + "/", f.getName());

                    if (strCurrentPath.equals(""))
                    {
                        listServerDir("/");
                    }
                    else
                    {
                        listServerDir(strCurrentPath);
                    }

                    Toast.makeText(this, "Upload: " + strFilePath, Toast.LENGTH_SHORT).show();
                }
                else
                {
                    Toast.makeText(this, "Upload Canceled", Toast.LENGTH_SHORT).show();
                }
            }
            break;

        }
    }

    public void OnRequestResult(int resultCode, int requestCode, JSONObject response)
    {
        switch (requestCode)
        {
            case CloudStorageHttpClient.REQUEST_CODE_TEST:
            {
                if (resultCode == RESPONSE_RESULT_OK)
                {
                    Log.d("MYTAG", "responseHandler: OK");
                    //TextView textView = (TextView)findViewById(R.id.textViewResponse);
                    //textView.setText(response.toString());
                }
                else if (resultCode == RESPONSE_RESULT_ERROR)
                {
                    Log.d("MYTAG", "responseHandler: ERROR");
                }
            }
            break;

            case CloudStorageHttpClient.REQUEST_CODE_LIST:
            {
                if (resultCode == RESPONSE_RESULT_OK)
                {
                    Log.d("MYTAG", "OnRequestResult: LIST: OK" + response.toString());
                    //TextView textView = (TextView)findViewById(R.id.textViewResponse);
                    //textView.setText(response.toString());
                    printDirectory(response);
                }
                else if (resultCode == RESPONSE_RESULT_ERROR)
                {
                    Log.d("MYTAG", "OnRequestResult: LIST: ERROR");
                }
            }
            break;

            case CloudStorageHttpClient.REQUEST_CODE_REMOVE:
            {
                if (resultCode == RESPONSE_RESULT_OK)
                {
                    Log.d("MYTAG", "OnRequestResult: REMOVE: OK");
                    if (strCurrentPath.equals(""))
                    {
                        listServerDir("/");
                    }
                    else
                    {
                        listServerDir(strCurrentPath);
                    }
                }
                else if (resultCode == RESPONSE_RESULT_ERROR)
                {
                    Log.d("MYTAG", "OnRequestResult: REMOVE: ERROR");
                }
            }
            break;

            case CloudStorageHttpClient.REQUEST_CODE_MKDIR:
            {
                if (resultCode == RESPONSE_RESULT_OK)
                {
                    Log.d("MYTAG", "OnRequestResult: MKDIR: OK");
                    if (strCurrentPath.equals(""))
                    {
                        listServerDir("/");
                    }
                    else
                    {
                        listServerDir(strCurrentPath);
                    }
                }
                else if (resultCode == RESPONSE_RESULT_ERROR)
                {
                    Log.d("MYTAG", "OnRequestResult: MKDIR: ERROR");
                }
            }
            break;

            //case ...:
        }
        Log.d("MYTAG", "responseHandler: " + response);
    }

    public Context getRequestContext()
    {
        return this;
    }

    public void printDirectory(final JSONObject fileList)
    {
        LinearLayout mainLayout = (LinearLayout) findViewById(R.id.directory_layout);
        mainLayout.removeAllViews();

        LinearLayout.LayoutParams generalParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);

        generalParams.setMargins(
                (int) convertDpToPixel(5),
                (int) convertDpToPixel(5),
                (int) convertDpToPixel(5),
                (int) convertDpToPixel(10)
        );

        int idCount = 0;

        Iterator<?> keys = fileList.keys();

        while( keys.hasNext() )
        {
            final String fileName = (String) keys.next();

            JSONObject fileObject = null;
            String fileType = "";
            try
            {
                fileObject = fileList.getJSONObject(fileName);
                fileType = fileObject.getString("type");
            }
            catch (Exception e){}

            if (fileType.equals("d"))
            {
                TextView nTextView = new TextView(getApplicationContext());

                nTextView.setText(fileName);
                nTextView.setBackgroundColor(0x60018577);
                nTextView.setPadding((int) convertDpToPixel(10), (int) convertDpToPixel(10), (int) convertDpToPixel(10), (int) convertDpToPixel(10));
                nTextView.setLayoutParams(generalParams);

                nTextView.setOnClickListener(new View.OnClickListener()
                {
                    @Override
                    public void onClick(View view)
                    {
                        currentPath.append("/" + fileName);
                        strCurrentPath += "/" + fileName;
                        listServerDir(strCurrentPath);
                    }
                });

                mainLayout.addView(nTextView);

                nTextView.setId(idCount);
                registerForContextMenu(nTextView);
                idCount++;
            }

            Log.d("MYTAG", fileName + " : " + fileType);
        }


        keys = fileList.keys();

        while( keys.hasNext() )
        {
            String fileName = (String) keys.next();

            JSONObject fileObject = null;
            String fileType = "";
            try
            {
                fileObject = fileList.getJSONObject(fileName);
                fileType = fileObject.getString("type");
            }
            catch (Exception e){}

            if (fileType.equals("f"))
            {
                TextView nTextView = new TextView(getApplicationContext());

                nTextView.setText(fileName);
                nTextView.setBackgroundColor(0x20018577);
                nTextView.setPadding((int) convertDpToPixel(10), (int) convertDpToPixel(10), (int) convertDpToPixel(10), (int) convertDpToPixel(10));
                nTextView.setLayoutParams(generalParams);

                mainLayout.addView(nTextView);

                nTextView.setId(idCount);
                registerForContextMenu(nTextView);
                idCount++;
            }

            Log.d("MYTAG", fileName + " : " + fileType);
        }

        if (idCount == 0)
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
            mainLayout.addView(nTextView);
        }
    }

    public static float convertDpToPixel(float dp) {
        DisplayMetrics metrics = Resources.getSystem().getDisplayMetrics();
        float px = dp * (metrics.densityDpi / 160f);
        return Math.round(px);
    }

    int idOfSelectedItemOfContextMenu;

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, v, menuInfo);

        idOfSelectedItemOfContextMenu = v.getId();

        menu.add(0, CONTEXT_MENU_DOWNLOAD, 0, "Download");
        menu.add(0, CONTEXT_MENU_REMOVE, 0, "Remove");

        //getMenuInflater().inflate(R.menu.context_menu,menu);
    }

    @Override
    public boolean onContextItemSelected(@NonNull MenuItem item) {
        switch (item.getItemId()){
            case CONTEXT_MENU_REMOVE:
            {
                TextView textView = (TextView) findViewById(idOfSelectedItemOfContextMenu);

                //Toast.makeText(this, textView.getText().toString(), Toast.LENGTH_LONG).show();

                String strFilePath = textView.getText().toString();

                CloudStorageHttpClient.remove(strCurrentPath + "/" + strFilePath, this);

                Log.d("MYTAG", "REMOVE");
                return true;
            }
            case CONTEXT_MENU_DOWNLOAD:
            {
                Log.d("MYTAG", "DOWNLOAD");

                TextView textView = (TextView) findViewById(idOfSelectedItemOfContextMenu);

                int colorCode = 0;

                if (textView.getBackground() instanceof ColorDrawable)
                {
                    ColorDrawable cd = (ColorDrawable) textView.getBackground();
                    colorCode = cd.getColor();
                }

                String strNativeFileName = textView.getText().toString();
                String strAndroidFileName = strNativeFileName;

                if (colorCode == 0x60018577) //dir
                {
                    strAndroidFileName += ".zip";
                }

                Toast.makeText(this, "Downloading: " + strAndroidFileName, Toast.LENGTH_LONG).show();

                CloudStorageHttpClient.download(
                        strCurrentPath + "/" + strNativeFileName,
                        strAndroidFileName,
                        this);

                return true;
            }
            default:
                Log.d("MYTAG", "DEFAULT");
                return super.onContextItemSelected(item);
        }
    }

    @Override public void onBackPressed() {

        if (strCurrentPath.equals(""))
        {
            Toast.makeText(this, "You are in root directory", Toast.LENGTH_LONG).show();
        }
        else
        {
            strCurrentPath = currentPath.getText().toString();

            File f = new File(strCurrentPath);
            f = f.getParentFile();

            strCurrentPath = f.getPath();

            currentPath.setText(strCurrentPath);
            strCurrentPath = strCurrentPath.substring(1);

            if (strCurrentPath.equals(""))
            {
                listServerDir("/");
            }
            else
            {
                listServerDir(strCurrentPath);
            }

            Log.d("MYTAG", "path:" + strCurrentPath);
        }
    }

}
