package com.example.cloudstorageapp;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONObject;

public class LoginActivity extends AppCompatActivity implements RequestHandler
{
    EditText username;
    EditText password;
    ProgressBar loading;

    EditText secret_code;

    String strUsername;
    String strPassword;

    private static int AUTH_SCREEN_CODE_LOGIN = 0;
    private static int AUTH_SCREEN_CODE_VERIFY = 1;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        getSupportActionBar().hide();

        username = (EditText)findViewById(R.id.username);
        password = (EditText)findViewById(R.id.password);
        loading = (ProgressBar) findViewById(R.id.loading);
        secret_code = (EditText)findViewById(R.id.secret_code);

    }

    public void onButtonLoginClicked(View view)
    {
        strUsername = username.getText().toString();
        strPassword = password.getText().toString();

        loading.setVisibility(View.VISIBLE);

        CloudStorageHttpClient.login(strUsername, strPassword, this);
    }

    public void onButtonSendCodeClicked(View view)
    {
        CloudStorageHttpClient.send_code(strUsername,this);
    }

    public void onButtonVerifyClicked(View view)
    {
        String strSecretCode = secret_code.getText().toString();

        loading.setVisibility(View.VISIBLE);
        CloudStorageHttpClient.verify(strUsername, strSecretCode, this);
    }


    private void setAuthScreen(int authScreenCode)
    {
        if (authScreenCode == AUTH_SCREEN_CODE_VERIFY)
        {
            username.setVisibility(View.INVISIBLE);
            username.setEnabled(false);
            password.setVisibility(View.INVISIBLE);
            password.setEnabled(false);
            secret_code.setVisibility(View.VISIBLE);
            secret_code.setEnabled(true);

            Button buttonLogin = (Button) findViewById(R.id.login);
            Button buttonSendCode = (Button) findViewById(R.id.send_code);
            Button buttonVerify = (Button) findViewById(R.id.verify);

            buttonLogin.setVisibility(View.INVISIBLE);
            buttonLogin.setEnabled(false);

            buttonSendCode.setVisibility(View.VISIBLE);
            buttonSendCode.setEnabled(true);

            buttonVerify.setVisibility(View.VISIBLE);
            buttonVerify.setEnabled(true);
        }
        else
        {
            username.setVisibility(View.VISIBLE);
            username.setEnabled(true);
            password.setVisibility(View.VISIBLE);
            password.setEnabled(true);
            secret_code.setVisibility(View.INVISIBLE);
            secret_code.setEnabled(false);

            Button buttonLogin = (Button) findViewById(R.id.login);
            Button buttonSendCode = (Button) findViewById(R.id.send_code);
            Button buttonVerify = (Button) findViewById(R.id.verify);

            buttonLogin.setVisibility(View.VISIBLE);
            buttonLogin.setEnabled(true);

            buttonSendCode.setVisibility(View.INVISIBLE);
            buttonSendCode.setEnabled(false);

            buttonVerify.setVisibility(View.INVISIBLE);
            buttonVerify.setEnabled(false);
        }
    }

    public void OnRequestResult(int resultCode, int requestCode, JSONObject response)
    {
        Log.d("MYTAG", "OnRequestResult: " + response.toString());
        switch (requestCode)
        {
            case CloudStorageHttpClient.REQUEST_CODE_LOGIN:
            {
                loading.setVisibility(View.GONE);

                if (resultCode == RESPONSE_RESULT_OK)
                {
                    Log.d("MYTAG", "OnRequestResult: Login : OK");

                    String strStat = "";
                    try
                    {
                        strStat = response.getString("status");
                    }
                    catch(Exception e) {}

                    if (strStat.equals("ok"))
                    {
                        Log.d("MYTAG", "<-: Login: stat:ok");
                        //Toast.makeText(this, "Login Success", Toast.LENGTH_SHORT).show();

                        setAuthScreen(AUTH_SCREEN_CODE_VERIFY);

                        CloudStorageHttpClient.send_code(strUsername,this);
                    }
                    else
                    {
                        Log.d("MYTAG", "<-: Login: stat:error");
                        Toast.makeText(this, "Login Error", Toast.LENGTH_SHORT).show();
                    }
                }
                else if (resultCode == RESPONSE_RESULT_ERROR)
                {
                    Log.d("MYTAG", "OnRequestResult: Login: ERROR");
                }
            }
            break;

            case CloudStorageHttpClient.REQUEST_CODE_VERIFY:
            {
                loading.setVisibility(View.GONE);

                if (resultCode == RESPONSE_RESULT_OK)
                {
                    Log.d("MYTAG", "OnRequestResult: Verify : OK");

                    String strStat = "";
                    try
                    {
                        strStat = response.getString("status");
                    }
                    catch(Exception e) {}

                    if (strStat.equals("ok"))
                    {
                        Log.d("MYTAG", "<-: Verify: stat:ok");

                        //Toast.makeText(this, "Login Success", Toast.LENGTH_SHORT).show();

                        Intent resultIntent = new Intent();
                        resultIntent.putExtra("username", strUsername);
                        setResult(RESULT_OK, resultIntent);
                        finish();
                    }
                    else
                    {
                        Log.d("MYTAG", "<-: Verify: stat:error");
                        Toast.makeText(this, "Login Error", Toast.LENGTH_SHORT).show();
                    }
                }
                else if (resultCode == RESPONSE_RESULT_ERROR)
                {
                    Log.d("MYTAG", "OnRequestResult: Verify : ERROR");
                }
            }
            break;
            //case ...:
        }

    }

    @Override
    public Context getRequestContext()
    {
        return this;
    }

    @Override public void onBackPressed()
    {
        setAuthScreen(AUTH_SCREEN_CODE_LOGIN);
    }
}
