package com.timm.ticktagandroidapp;

import android.content.Context;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;

public class LogDataStorage {

    private static final String LOGFILENAME = "tickTagLog.txt";

    public static String logText(Context context, String text) {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd_HHmmss", Locale.getDefault());
        String currentDateandTime = sdf.format(new Date());
        String fullFileName = StorageGeneral.getStorageDirectory(context) + "/" + currentDateandTime + "_" + LOGFILENAME;
        File fileToLogTo = new File(fullFileName);

        if(fileToLogTo.exists()) { return ""; }

        try {
            fileToLogTo.createNewFile();
        } catch (IOException e) {
            return "";
        }
        try {
            FileOutputStream stream = new FileOutputStream(fullFileName, true);
            stream.write(text.getBytes(Charset.forName("UTF-8")));
            stream.close();
        } catch (IOException e1) {
            return "";
        }

        return fullFileName;
    }
}
