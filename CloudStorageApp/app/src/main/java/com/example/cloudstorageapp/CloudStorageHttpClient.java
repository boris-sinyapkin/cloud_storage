package com.example.cloudstorageapp;

import android.content.Context;
import android.content.Intent;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

import androidx.core.content.ContextCompat;

import com.android.volley.AuthFailureError;
import com.android.volley.Cache;
import com.android.volley.Network;
import com.android.volley.NetworkResponse;
import com.android.volley.Request;
import com.android.volley.RequestQueue;
import com.android.volley.Response;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.JsonObjectRequest;
import com.android.volley.toolbox.StringRequest;
import com.android.volley.toolbox.Volley;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.net.CookieHandler;
import java.net.CookieManager;
import java.net.CookieStore;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLEncoder;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutionException;


interface RequestHandler
{
    public static final int RESPONSE_RESULT_OK = -1;
    public static final int RESPONSE_RESULT_ERROR = 0;

    void OnRequestResult(int resultCode, int requestCode, JSONObject response);

    Context getRequestContext();
}

public class CloudStorageHttpClient
{
    public static int  session;
    public static final String HOME_URL = "http://study-web2.germanywestcentral.cloudapp.azure.com";
    //public static final String HOME_URL = "http://10.210.12.154:8001";
    public static final String TLG_URL = "http://192.168.1.42:8001";
    //public static final String TLG_URL = "http://192.168.1.42:8001";

    public static final int REQUEST_CODE_TEST = 0;
    public static final int REQUEST_CODE_LOGIN = 1;
    public static final int REQUEST_CODE_TLG_CODE = 2;
    public static final int REQUEST_CODE_VERIFY = 3;
    public static final int REQUEST_CODE_LIST = 4;
    public static final int REQUEST_CODE_REMOVE = 5;
    public static final int REQUEST_CODE_MKDIR = 6;

    public static CookieManager manager;

    public static int startSession()
    {
        manager = new CookieManager();
        CookieHandler.setDefault( manager );
        //DefaultHttpClient httpclient = new DefaultHttpClient();
        //HttpStack httpStack = new HttpClientStack( httpclient );
        //RequestQueue requestQueue = Volley.newRequestQueue( context, httpStack  );

        session = 0;
        return 0;
    }

    public static void login(String strUsername, String strPassword, final RequestHandler requestHandler)
    {
        RequestQueue  queue = Volley.newRequestQueue(requestHandler.getRequestContext());

        JSONObject jsonObject = new JSONObject();
        try
        {
            jsonObject.put("login", strUsername);
            jsonObject.put("pass", strPassword);
        }
        catch (Exception execution) {}

        String url = HOME_URL + "/android/login/";
        JsonObjectRequest stringRequest = new JsonObjectRequest(
                Request.Method.POST,
                url,
                jsonObject,
                new Response.Listener<JSONObject>()
                {
                    @Override
                    public void onResponse(JSONObject response)
                    {
                        Log.d("mj.volleytesting", response.toString());
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_OK,
                                REQUEST_CODE_LOGIN,
                                response
                        );
                    }
                },
                new Response.ErrorListener()
                {
                    @Override
                    public void onErrorResponse(VolleyError error)
                    {
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_ERROR,
                                REQUEST_CODE_LOGIN,
                                new JSONObject()
                        );
                        Log.d("mj.volleytesting", "that didn't work:" + error.toString());
                    }
                }
        );

        queue.add(stringRequest);
    }

    public static void send_code(String strUsername, final RequestHandler requestHandler)
    {
        RequestQueue queue = Volley.newRequestQueue(requestHandler.getRequestContext());

        String url = HOME_URL + "/android/code_gen?login=" + strUsername;
        StringRequest stringRequest = new StringRequest(
                Request.Method.GET,
                url,
                new Response.Listener<String>()
                {
                    @Override
                    public void onResponse(String response)
                    {
                        Log.d("MYTAG", "-> send_code: " + response.toString());
                    }
                },
                new Response.ErrorListener()
                {
                    @Override
                    public void onErrorResponse(VolleyError error)
                    {
                        Log.d("MYTAG", "-> send_code: " + error.toString());
                    }
                }
        );

        queue.add(stringRequest);
    }


    public static void verify(String strUsername, String strSecretCode, final RequestHandler requestHandler)
    {
        RequestQueue queue = Volley.newRequestQueue(requestHandler.getRequestContext());

        JSONObject jsonObject = new JSONObject();
        try
        {
            jsonObject.put("login", strUsername);
            jsonObject.put("secret_code", strSecretCode);
        }
        catch (Exception execution) {}

        String url = HOME_URL + "/android/verify/";
        JsonObjectRequest stringRequest = new JsonObjectRequest(
                Request.Method.POST,
                url,
                jsonObject,
                new Response.Listener<JSONObject>()
                {
                    @Override
                    public void onResponse(JSONObject response)
                    {
                        Log.d("MYTAG", "-> verify: " + response.toString());
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_OK,
                                REQUEST_CODE_VERIFY,
                                response
                        );
                    }
                },
                new Response.ErrorListener()
                {
                    @Override
                    public void onErrorResponse(VolleyError error)
                    {
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_ERROR,
                                REQUEST_CODE_VERIFY,
                                new JSONObject()
                        );
                        Log.d("MYTAG", "-> verify: " + error.toString());
                    }
                }
        );

        queue.add(stringRequest);
    }

    public static void list(String strPath, final RequestHandler requestHandler)
    {
        RequestQueue queue = Volley.newRequestQueue(requestHandler.getRequestContext());

        JSONObject jsonObject = new JSONObject();
        try
        {
            jsonObject.put("path", strPath);
        }
        catch (Exception execution) {}

        String url = HOME_URL + "/android/list/";
        JsonObjectRequest stringRequest = new JsonObjectRequest(
                Request.Method.POST,
                url,
                jsonObject,
                new Response.Listener<JSONObject>()
                {
                    @Override
                    public void onResponse(JSONObject response)
                    {
                        Log.d("mj.volleytesting", response.toString());
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_OK,
                                REQUEST_CODE_LIST,
                                response
                        );
                    }
                },
                new Response.ErrorListener()
                {
                    @Override
                    public void onErrorResponse(VolleyError error)
                    {
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_ERROR,
                                REQUEST_CODE_LIST,
                                new JSONObject()
                        );
                        Log.d("mj.volleytesting", "that didn't work:" + error.toString());
                    }
                }
        );

        queue.add(stringRequest);
    }

    public static void mkdir(String strPath, final RequestHandler requestHandler)
    {
        RequestQueue queue = Volley.newRequestQueue(requestHandler.getRequestContext());

        JSONObject jsonObject = new JSONObject();
        try
        {
            jsonObject.put("path", strPath);
        }
        catch (Exception execution) {}

        String url = HOME_URL + "/android/mkdir/";
        JsonObjectRequest stringRequest = new JsonObjectRequest(
                Request.Method.POST,
                url,
                jsonObject,
                new Response.Listener<JSONObject>()
                {
                    @Override
                    public void onResponse(JSONObject response)
                    {
                        Log.d("mj.volleytesting", response.toString());
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_OK,
                                REQUEST_CODE_MKDIR,
                                response
                        );
                    }
                },
                new Response.ErrorListener()
                {
                    @Override
                    public void onErrorResponse(VolleyError error)
                    {
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_ERROR,
                                REQUEST_CODE_MKDIR,
                                new JSONObject()
                        );
                        Log.d("mj.volleytesting", "that didn't work:" + error.toString());
                    }
                }
        );

        queue.add(stringRequest);
    }


    public static void remove(String strPath, final RequestHandler requestHandler)
    {
        RequestQueue queue = Volley.newRequestQueue(requestHandler.getRequestContext());

        JSONObject jsonObject = new JSONObject();
        try
        {
            jsonObject.put("path", strPath);
        }
        catch (Exception execution) {}

        String url = HOME_URL + "/android/remove/";
        JsonObjectRequest stringRequest = new JsonObjectRequest(
                Request.Method.POST,
                url,
                jsonObject,
                new Response.Listener<JSONObject>()
                {
                    @Override
                    public void onResponse(JSONObject response)
                    {
                        Log.d("mj.volleytesting", response.toString());
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_OK,
                                REQUEST_CODE_REMOVE,
                                response
                        );
                    }
                },
                new Response.ErrorListener()
                {
                    @Override
                    public void onErrorResponse(VolleyError error)
                    {
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_ERROR,
                                REQUEST_CODE_REMOVE,
                                new JSONObject()
                        );
                        Log.d("mj.volleytesting", "that didn't work:" + error.toString());
                    }
                }
        );



        queue.add(stringRequest);
    }


    public static void request(final RequestHandler requestHandler)
    {
        RequestQueue queue = Volley.newRequestQueue(requestHandler.getRequestContext());

        JSONObject jsonObject = new JSONObject();
        try
        {
            jsonObject.put("test", 0);
        }
        catch (Exception execution) {}

        String url = HOME_URL + "/test";
        JsonObjectRequest stringRequest = new JsonObjectRequest(
                Request.Method.POST,
                url,
                jsonObject,
                new Response.Listener<JSONObject>()
                {
                    @Override
                    public void onResponse(JSONObject response)
                    {
                        Log.d("mj.volleytesting", response.toString());
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_OK,
                                REQUEST_CODE_TEST,
                                response
                        );
                    }
                },
                new Response.ErrorListener()
                {
                    @Override
                    public void onErrorResponse(VolleyError error)
                    {
                        requestHandler.OnRequestResult(
                                requestHandler.RESPONSE_RESULT_ERROR,
                                REQUEST_CODE_TEST,
                                new JSONObject()
                        );
                        Log.d("mj.volleytesting", "that didn't work:" + error.toString());
                    }
                }
            );

        queue.add(stringRequest);
    }


    public static void uploadFile()
    {
        android.os.StrictMode.ThreadPolicy policy = new android.os.StrictMode.ThreadPolicy.Builder().permitAll().build();
        android.os.StrictMode.setThreadPolicy(policy);

        HttpURLConnection httpUrlConnection = null;
        try
        {
            URL url = new URL(HOME_URL + "/android/upload/");
            httpUrlConnection = (HttpURLConnection) url.openConnection();
            httpUrlConnection.setRequestMethod("POST");
            DataOutputStream request = new DataOutputStream(
                    httpUrlConnection.getOutputStream());
        }
        catch (Exception e)
        {
            Log.d("MYTAG", "that didn't work:" + e.getMessage());
        }
    }


    public static int upload(String sourceFileUri, String strPath, String fileName)
    {
        android.os.StrictMode.ThreadPolicy policy = new android.os.StrictMode.ThreadPolicy.Builder().permitAll().build();
        android.os.StrictMode.setThreadPolicy(policy);

        String upLoadServerUri = HOME_URL + "/android/upload/?path=" + strPath;

        Log.d("MYTAG", "TRYDIR: " + strPath + " / " + fileName);
        int serverResponseCode = 0;

        HttpURLConnection conn = null;
        DataOutputStream dos = null;
        String lineEnd = "\r\n";
        String twoHyphens = "--";
        String boundary = "*****";
        int bytesRead, bytesAvailable, bufferSize;
        byte[] buffer;
        int maxBufferSize = 1 * 1024 * 1024;
        File sourceFile = new File(sourceFileUri);
        //errMsg=Environment.getExternalStorageDirectory().getAbsolutePath();
        if (!sourceFile.isFile())
        {
            Log.e("uploadFile", "Source File Does not exist");
            return 0;
        }
        try {
            // open a URL connection to the Servlet
            FileInputStream fileInputStream = new FileInputStream(sourceFile);
            URL url = new URL(upLoadServerUri);
            conn = (HttpURLConnection) url.openConnection(); // Open a HTTP  connection to  the URL
            conn.setDoInput(true); // Allow Inputs
            conn.setDoOutput(true); // Allow Outputs
            conn.setUseCaches(false); // Don't use a Cached Copy
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Connection", "Keep-Alive");
            conn.setRequestProperty("ENCTYPE", "multipart/form-data");
            conn.setRequestProperty("Content-Type", "multipart/form-data;boundary=" + boundary);
            conn.setRequestProperty("uploaded_file", fileName);
            conn.setRequestProperty("path", "/");/*###*/
            //conn.setRequestProperty("pid", "4");
            dos = new DataOutputStream(conn.getOutputStream());

            dos.writeBytes(twoHyphens + boundary + lineEnd);
            dos.writeBytes("Content-Disposition: form-data; name=\"uploaded_file\";filename=\""+ fileName + "\"" + lineEnd);
            dos.writeBytes(lineEnd);

            bytesAvailable = fileInputStream.available(); // create a buffer of  maximum size

            bufferSize = Math.min(bytesAvailable, maxBufferSize);
            buffer = new byte[bufferSize];

            // read file and write it into form...
            bytesRead = fileInputStream.read(buffer, 0, bufferSize);

            while (bytesRead > 0) {
                dos.write(buffer, 0, bufferSize);
                bytesAvailable = fileInputStream.available();
                bufferSize = Math.min(bytesAvailable, maxBufferSize);
                bytesRead = fileInputStream.read(buffer, 0, bufferSize);
            }

            // send multipart form data necesssary after file data...
            dos.writeBytes(lineEnd);
            dos.writeBytes(twoHyphens + boundary + twoHyphens + lineEnd);

            // Responses from the server (code and message)
            serverResponseCode = conn.getResponseCode();
            String serverResponseMessage = conn.getResponseMessage();

            Log.i("uploadFile", "HTTP Response is : " + serverResponseMessage + ": " + serverResponseCode);
            if(serverResponseCode != 200)
            {
                Log.i("MYTAG", "You Suck");
            }

            //close the streams //
            fileInputStream.close();
            dos.flush();
            dos.close();
        } catch (MalformedURLException ex) {
            // dialog.dismiss();
            ex.printStackTrace();
            //Toast.makeText(context, "MalformedURLException", Toast.LENGTH_SHORT).show();
            Log.e("Upload file to server", "error: " + ex.getMessage(), ex);
        } catch (Exception e) {
            //  dialog.dismiss();
            e.printStackTrace();
            //Toast.makeText(context,errMsg+ "Exception : " + e.getMessage(), Toast.LENGTH_SHORT).show();
            Log.e("Upload file to server Exception", "Exception : " + e.getMessage(), e);
        }
        //  dialog.dismiss();
        return serverResponseCode;
    }

    public static void download(String strFullName, final String strAndroidFileName, final RequestHandler requestHandler)
    {
        RequestQueue queue = Volley.newRequestQueue(requestHandler.getRequestContext());

        JSONObject jsonObject = new JSONObject();
        try
        {
            jsonObject.put("path", strFullName);
        }
        catch (Exception execution) {}

        String url = HOME_URL + "/android/download/?path=" + strFullName;
        //String url = "https://static3.depositphotos.com/1004996/226/i/450/depositphotos_2260019-stock-photo-glowing-font-shiny-letter-s.jpg";

        /*
        StringRequest request = new StringRequest(
                Request.Method.POST,
                url,
                new Response.Listener<String>()
                {
                    @Override
                    public void onResponse(String response)
                    {
                        response.getBytes();
                        Log.d("DATA", "Len: " + String.valueOf(response.length()) + " Data: " + response);

                        ByteArrayOutputStream byteArrayOutputStream = new ByteArrayOutputStream();

                        int strLen = response.length();

                        String byteStr = "";

                        for (int i = 0; i < strLen; i++)
                        {
                            int code = (int)response.charAt(i);
                            byteArrayOutputStream.write(code);


                            byteStr += String.valueOf(code) + ", ";
                        }

                        Log.d("ByteDATA", "Len: " + String.valueOf(byteArrayOutputStream.size()));
                        Log.d("ByteDATA", "data: " + byteStr);

                        //writeFileExternalStorage(byteData, strAndroidFileName);
                    }
                },
                new Response.ErrorListener()
                {
                    @Override
                    public void onErrorResponse(VolleyError error)
                    {
                        //Log.d("DATA", error.getMessage());
                    }
                }
            );
*/

        ByteArrayRequest request = new ByteArrayRequest(
                Request.Method.POST,
                url,
                new Response.Listener<byte[]>() {
                    @Override
                    public void onResponse(byte[] response) {
                        Log.i("getBilletCard", response.toString());
                        try {
                            byte[] byteData = response;
                            writeFileExternalStorage(byteData, strAndroidFileName);

                            Log.d("ByteDATA", "Len: " + response.length);
                            //saveToFile(bytes, "card.pdf");
                        }catch (Exception e){
                            Log.d("ByteDATA", "Sosi");
                        }
                    }
                },
                new Response.ErrorListener() {
                    @Override
                    public void onErrorResponse(VolleyError error) {
                        Log.d("ByteDATA", "Sosi Big Hui");
                        //error.printStackTrace();
                    }
                });



        queue.add(request);
    }


    public static void writeFileExternalStorage(byte[] byteData, String strFileName) {

        //Text of the Document
        //String textToWrite = "bla bla bla";

        //Checking the availability state of the External Storage.
        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)) {

            //If it isn't mounted - we can't write into it.
            return;
        }

        //Create a new file that points to the root directory, with the given name:
        File file = new File(
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS),
                strFileName
        );


        //This point and below is responsible for the write operation
        FileOutputStream outputStream = null;
        try {
            file.createNewFile();
            //second argument of FileOutputStream constructor indicates whether
            //to append or create new file if one exists
            outputStream = new FileOutputStream(file, true);

            outputStream.write(byteData);
            outputStream.flush();
            outputStream.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
