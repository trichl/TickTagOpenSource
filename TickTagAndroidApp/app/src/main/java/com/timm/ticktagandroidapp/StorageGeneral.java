package com.timm.ticktagandroidapp;

import android.content.Context;
import android.os.Environment;
import android.os.StatFs;

import java.io.File;

public class StorageGeneral {

    private static final long MINIMUM_MEMORY = (16L * 1024L * 1024L); // 16 MByte

    public static long getMinimumMemory() {
        return MINIMUM_MEMORY;
    }

    public static boolean enoughFreeMemoryToStoreShit() {
        if(getAvailableInternalMemorySize() < MINIMUM_MEMORY) {
            return false;
        }
        return true;
    }

    public static long getAvailableInternalMemorySize() {
        File path = Environment.getDataDirectory();
        StatFs stat = new StatFs(path.getPath());
        long blockSize = stat.getBlockSizeLong();
        long availableBlocks = stat.getAvailableBlocksLong();
        return availableBlocks * blockSize;
    }

    public static String getAvailableInternalMemorySizeAsStringInMByte() {
        long mByteFree = (getAvailableInternalMemorySize() / (1024 * 1024));
        String mbyteWithPoint = String.format("%,d", mByteFree);
        return mbyteWithPoint;
    }

    public static String getStorageDirectory(Context context) {
        return context.getExternalFilesDir(null).getPath().toString();
    }

}
