package com.timm.ticktagandroidapp;

import android.content.Context;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

class RawDataStorage {
    private static final String FILENAME_BASE = "ESP32Data.bin";

    private File fileToStoreTo;
    private String fullFileName = "";
    private String fileNameWithoutDirectory = "";
    private int selectedItem;

    public String getStoragePath() {
        return fullFileName;
    }

    public String getFileNameWithoutDirectory() {
        return fileNameWithoutDirectory;
    }

    public void testWrite() {
        byte[] fileContent = new byte[20];
        for(int i=0; i<20; i++) {
            fileContent[i] = (byte) i;
        }
        writeToFile(fileContent);
    }

    public boolean writeToFile(byte[] data) {
        if(fileToStoreTo == null) return false;
        if(!fileToStoreTo.exists()) {
            try {
                fileToStoreTo.createNewFile();
            } catch (IOException e) {
                return false;
            }
        }
        try {
            FileOutputStream stream = new FileOutputStream(fullFileName, true);
            stream.write(data);
            stream.close();
        } catch (IOException e1) {
            return false;
        }
        return true;
    }

    public RawDataStorage(Context context) {
        SimpleDateFormat sdf = new SimpleDateFormat("yyyyMMdd_HHmmss", Locale.getDefault());
        String currentDateandTime = sdf.format(new Date());
        fullFileName = StorageGeneral.getStorageDirectory(context) + "/" + currentDateandTime + "_" + FILENAME_BASE;
        fileNameWithoutDirectory = currentDateandTime + "_" + FILENAME_BASE;
        fileToStoreTo = new File(fullFileName);
    }
}
