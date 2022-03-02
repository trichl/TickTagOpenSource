package com.timm.ticktagandroidapp;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.AppCompatButton;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.InsetDrawable;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.text.InputType;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

import java.io.IOException;
import java.util.List;

public class MainActivity extends AppCompatActivity implements SerialInputOutputManager.Listener {
    private String INTENT_ACTION_GRANT_USB = BuildConfig.APPLICATION_ID + ".GRANT_USB";
    SerialInputOutputManager usbIoManager;

    Button bStartStop, bStoreOutput, bOpenDownloads, bClearOutput;
    Button b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, bEnter, bReadMemory, bReset, bSetVoltage, bSetFrequency, bSetAccuracy, bSetDelay, bSetHour, bToggleGeofencing, bToggleBlinking, bSetBurstDuration, bExit;
    TextView tOutput, tHeadline;
    RelativeLayout rLoadingPanel;
    LinearLayout lAlertPressButton, lAlertNotConnected, lButtons, lButtons2;
    ScrollView scScroll;
    UsbSerialPort usbPort = null;
    Thread thread;

    boolean tickTagConnected = false;

    private void checkPermissions() {
        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if(this.checkSelfPermission(Manifest.permission.WRITE_SETTINGS) != PackageManager.PERMISSION_GRANTED) {
                if(!this.shouldShowRequestPermissionRationale(Manifest.permission.WRITE_SETTINGS)) {
                    final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setTitle("This app needs background location access");
                    builder.setMessage("Please grant location access so this app can detect beacons in the background.");
                    builder.setPositiveButton(android.R.string.ok, null);
                    builder.setOnDismissListener(new DialogInterface.OnDismissListener() {

                        @TargetApi(23)
                        @Override
                        public void onDismiss(DialogInterface dialog) {
                            requestPermissions(new String[]{Manifest.permission.WRITE_SETTINGS}, 0);
                        }

                    });
                    builder.show();
                }
                else {
                    final AlertDialog.Builder builder = new AlertDialog.Builder(this);
                    builder.setTitle("Functionality limited");
                    builder.setMessage("Since background location access has not been granted, this app will not be able to discover beacons in the background.  Please go to Settings -> Applications -> Permissions and grant background location access to this app.");
                    builder.setPositiveButton(android.R.string.ok, null);
                    builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
                        @Override
                        public void onDismiss(DialogInterface dialog) {}
                    });
                    builder.show();
                }
            }
        }
    }

    public void showMessage(String headline, String text) {
        final AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(headline);
        builder.setMessage(text);
        builder.setPositiveButton(android.R.string.ok, null);
        builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {}
        });
        builder.show();
    }

    void setButtonConnected() {
        bStartStop.setText("DISCONNECT");
        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.LOLLIPOP && bStartStop instanceof AppCompatButton) {
            // don't change color
        } else {
            bStartStop.setBackgroundTintList(ColorStateList.valueOf(Color.parseColor("#B9F6CA")));
        }
        lAlertPressButton.setVisibility(View.VISIBLE);
        lAlertNotConnected.setVisibility(View.GONE);
        //lButtons.setVisibility(View.VISIBLE);
        lButtons2.setVisibility((View.VISIBLE));
    }

    void setButtonDisconnected() {
        bStartStop.setText("CONNECT");
        if(Build.VERSION.SDK_INT <= Build.VERSION_CODES.LOLLIPOP && bStartStop instanceof AppCompatButton) {
            // don't change color
        } else {
            bStartStop.setBackgroundTintList(ColorStateList.valueOf(Color.parseColor("#FF80AB")));
        }
        lAlertPressButton.setVisibility(View.GONE);
        lAlertNotConnected.setVisibility(View.VISIBLE);
        //lButtons.setVisibility(View.GONE);
        lButtons2.setVisibility((View.GONE));
    }

    @Override
    protected void onPause() {
        //Toast.makeText(this, "Gateway is still running in background!", Toast.LENGTH_LONG).show();
        super.onPause();
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    public void updateHeadlineText() {
        if(tHeadline != null)
            tHeadline.setText("TickTag Android App\nFree memory: " + StorageGeneral.getAvailableInternalMemorySizeAsStringInMByte() + " MByte");
    }

    private void setAlertWindows() {
        lAlertPressButton.setVisibility(View.GONE);
        lAlertNotConnected.setVisibility(View.GONE);
    }

    @Override
    public void onNewData(final byte[] data) {
        //System.out.println(new String(data));
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                tOutput.append(new String(data));
                lAlertPressButton.setVisibility(View.GONE);
                scScroll.post(new Runnable() {
                    public void run() {
                        scScroll.fullScroll(ScrollView.FOCUS_DOWN);
                    }
                });
            }
        });
    }

    @Override
    public void onRunError(Exception e) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                showMessage("Disconnected", "USB disconnected!");
                tickTagConnected = false;
                setButtonDisconnected();
            }
        });
    }

    void checkIfUSBConnectable() {
        setButtonDisconnected();

        // Find all available drivers from attached devices.
        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        List<UsbSerialDriver> availableDrivers = UsbSerialProber.getDefaultProber().findAllDrivers(manager);
        if(availableDrivers.isEmpty()) {
            showMessage("UIB not connected", "Could not detect a user interface board at the USB port of the phone! Please attach the TickTag to the user interface board and then connect it via USB OTG cable to your phone.");
            return;
        }

        // Open a connection to the first available driver.
        UsbSerialDriver driver = availableDrivers.get(0);
        UsbDeviceConnection connection = manager.openDevice(driver.getDevice());
        if(connection == null) {
            PendingIntent usbPermissionIntent = PendingIntent.getBroadcast(MainActivity.this, 0, new Intent(INTENT_ACTION_GRANT_USB), 0);
            manager.requestPermission(driver.getDevice(), usbPermissionIntent);
        }

        usbPort = driver.getPorts().get(0); // Most devices have just one port (port 0)
        try {
            usbPort.open(connection);
        } catch (IOException e) {
            e.printStackTrace();
            showMessage("Error", "Could not open the serial port!");
            return;
        }
        try {
            usbPort.setParameters(9600, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE);
        } catch (IOException e) {
            e.printStackTrace();
            showMessage("Error", "Could not set serial parameters!");
            return;
        }

        usbIoManager = new SerialInputOutputManager(usbPort, MainActivity.this);
        usbIoManager.start();

        setButtonConnected();
        tickTagConnected = true;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.getSupportActionBar().hide();
        setContentView(R.layout.activity_main);

        bStartStop = findViewById(R.id.bstartstop);
        bStoreOutput = findViewById(R.id.bstoreoutput);
        bClearOutput = findViewById(R.id.bclearoutput);
        bOpenDownloads = findViewById(R.id.bopendownloads);
        tOutput = findViewById(R.id.toutput);
        rLoadingPanel = findViewById(R.id.rloadingpanel);
        tHeadline = findViewById(R.id.theadline);
        scScroll = findViewById(R.id.scscroll);
        lAlertPressButton = findViewById(R.id.lalertpressbutton);
        lAlertNotConnected = findViewById(R.id.lalertnotconnected);
        lButtons = findViewById(R.id.lbuttons);
        lButtons2 = findViewById(R.id.lbuttons2);

        b0 = findViewById(R.id.b0);
        b1 = findViewById(R.id.b1);
        b2 = findViewById(R.id.b2);
        b3 = findViewById(R.id.b3);
        b4 = findViewById(R.id.b4);
        b5 = findViewById(R.id.b5);
        b6 = findViewById(R.id.b6);
        b7 = findViewById(R.id.b7);
        b8 = findViewById(R.id.b8);
        b9 = findViewById(R.id.b9);
        bEnter = findViewById(R.id.benter);

        bReadMemory = findViewById(R.id.bread);
        bReset  = findViewById(R.id.breset);
        bSetVoltage = findViewById(R.id.bsetvoltage);
        bSetFrequency = findViewById(R.id.bsetfrequency);
        bSetAccuracy = findViewById(R.id.bsetaccuracy);
        bSetDelay = findViewById(R.id.bsetdelay);
        bSetHour = findViewById(R.id.bsethour);
        bToggleGeofencing = findViewById(R.id.btogglegeofencing);
        bToggleBlinking = findViewById(R.id.btoggleblinking);
        bSetBurstDuration = findViewById(R.id.bsetburstduration);
        bExit = findViewById(R.id.bexit);

        rLoadingPanel.setVisibility(View.GONE);

        setAlertWindows();
        updateHeadlineText();
        checkIfUSBConnectable();

        /*thread = new Thread() {
            @Override
            public void run() {
                try {
                    while(!thread.isInterrupted()) {
                        Thread.sleep(1000);
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                scScroll.post(new Runnable() {
                                    public void run() {
                                        scScroll.fullScroll(ScrollView.FOCUS_DOWN);
                                    }
                                });
                            }
                        });
                    }
                } catch (InterruptedException e) {}
            }
        };
        thread.start();*/

        bStartStop.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(tickTagConnected) {
                    if(usbPort != null) {
                        try {
                            usbPort.close(); // triggers onRunError

                        } catch (IOException e) {
                            e.printStackTrace();
                            showMessage("Error", "Could not close USB port!");
                        }
                    }
                }
                else {
                    checkIfUSBConnectable();
                }
            }
        });

        bClearOutput.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                tOutput.setText("");
                //tOutput.append("Hallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\nHallo\n");
                Toast.makeText(MainActivity.this, "Output cleared!", Toast.LENGTH_LONG).show();
            }
        });

        bStoreOutput.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(tOutput.getText().toString().isEmpty()) {
                    Toast.makeText(MainActivity.this, "Output is empty, nothing to store!", Toast.LENGTH_LONG).show();
                }
                else {
                    String outputString = tOutput.getText().toString();
                    outputString = outputString.replace("\r", "");
                    String storedTo = LogDataStorage.logText(MainActivity.this, outputString);
                    if (storedTo.length() > 0) {
                        showMessage("Saved", "Output stored to: " + storedTo);
                    } else {
                        showMessage("Error", "Could not store output!");
                    }
                }
            }
        });

        bOpenDownloads.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Uri selectedUri = Uri.parse(MainActivity.this.getExternalFilesDir(null).getPath());
                Intent intent = new Intent(Intent.ACTION_VIEW);
                intent.setDataAndType(selectedUri, "resource/folder");
                if(intent.resolveActivityInfo(getPackageManager(), 0) != null) {
                    startActivity(intent);
                }
                else {
                    Toast.makeText(MainActivity.this, "No app found to view folders, please download a file manager", Toast.LENGTH_LONG).show();
                }
            }
        });


        b0.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "0";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        b1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "1";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        b2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "2";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        b3.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "3";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        b4.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "4";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        b5.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "5";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        b6.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "6";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        b7.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "7";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        b8.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "8";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        b9.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "9";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        bEnter.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "\n\r";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                    }
                });
            }
        });

        bReadMemory.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if(tickTagConnected) {
                            String input = "0\n\r";
                            tOutput.append(input);
                            try {
                                usbPort.write(input.getBytes(), 100);
                                Toast.makeText(MainActivity.this, "Download might take a while! Press 'Store Output' afterwards to store downloaded data as text file.", Toast.LENGTH_LONG).show();
                            } catch (IOException e) {
                                e.printStackTrace();
                                Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        }
                        else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                    }
                });
            }
        });

        bReset.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder alert = new AlertDialog.Builder(MainActivity.this);
                alert.setTitle("Do you really want to reset the memory of the TickTag? All GPS data will be deleted.");
                alert.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(tickTagConnected) {
                                    String input = "1\n\r";
                                    tOutput.append(input);
                                    try {
                                        usbPort.write(input.getBytes(), 100);
                                    } catch (IOException e) {
                                        e.printStackTrace();
                                        Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                    }
                                }
                                else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                });
                alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) { }
                });
                alert.show();
            }
        });

        bSetVoltage.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder alert = new AlertDialog.Builder(MainActivity.this);
                alert.setTitle("Enter mV");
                final EditText input = new EditText(MainActivity.this);
                input.setInputType(InputType.TYPE_CLASS_NUMBER);
                input.setRawInputType(Configuration.KEYBOARD_12KEY);
                input.setHint("3000 - 4250");
                alert.setView(input);
                alert.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        final String sendText = input.getText().toString();
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(tickTagConnected) {
                                    if(sendText.length() > 0) {
                                        String input = "2\n\r";
                                        tOutput.append(input);
                                        try {
                                            usbPort.write(input.getBytes(), 100);
                                        } catch (IOException e) {
                                            e.printStackTrace();
                                            Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                            return;
                                        }
                                        final String input2 = sendText + "\n\r";
                                        new Handler().postDelayed(new Runnable() {
                                            @Override
                                            public void run() {
                                                tOutput.append(input2);
                                                try {
                                                    usbPort.write(input2.getBytes(), 100);
                                                } catch (IOException e) {
                                                    e.printStackTrace();
                                                    Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                                }
                                            }
                                        }, 500);
                                    }
                                    else Toast.makeText(MainActivity.this, "Invalid input!", Toast.LENGTH_LONG).show();
                                }
                                else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                });
                alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) { }
                });
                alert.show();
            }
        });

        bSetFrequency.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder alert = new AlertDialog.Builder(MainActivity.this);
                alert.setTitle("Enter GPS frequency in seconds (1 - 5 = stay-on)");
                final EditText input = new EditText(MainActivity.this);
                input.setInputType(InputType.TYPE_CLASS_NUMBER);
                input.setRawInputType(Configuration.KEYBOARD_12KEY);
                input.setHint("1 - 16382");
                alert.setView(input);
                alert.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        final String sendText = input.getText().toString();
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(tickTagConnected) {
                                    if(sendText.length() > 0) {
                                        String input = "3\n\r";
                                        tOutput.append(input);
                                        try {
                                            usbPort.write(input.getBytes(), 100);
                                        } catch (IOException e) {
                                            e.printStackTrace();
                                            Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                            return;
                                        }
                                        final String input2 = sendText + "\n\r";
                                        new Handler().postDelayed(new Runnable() {
                                            @Override
                                            public void run() {
                                                tOutput.append(input2);
                                                try {
                                                    usbPort.write(input2.getBytes(), 100);
                                                } catch (IOException e) {
                                                    e.printStackTrace();
                                                    Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                                }
                                            }
                                        }, 500);
                                    }
                                    else Toast.makeText(MainActivity.this, "Invalid input!", Toast.LENGTH_LONG).show();
                                }
                                else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                });
                alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) { }
                });
                alert.show();
            }
        });

        bSetAccuracy.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder alert = new AlertDialog.Builder(MainActivity.this);
                alert.setTitle("Enter accuracy (HDOP x 10, 30 = 3.0)");
                final EditText input = new EditText(MainActivity.this);
                input.setInputType(InputType.TYPE_CLASS_NUMBER);
                input.setRawInputType(Configuration.KEYBOARD_12KEY);
                input.setHint("10 - 250");
                alert.setView(input);
                alert.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        final String sendText = input.getText().toString();
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(tickTagConnected) {
                                    if(sendText.length() > 0) {
                                        String input = "4\n\r";
                                        tOutput.append(input);
                                        try {
                                            usbPort.write(input.getBytes(), 100);
                                        } catch (IOException e) {
                                            e.printStackTrace();
                                            Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                            return;
                                        }
                                        final String input2 = sendText + "\n\r";
                                        new Handler().postDelayed(new Runnable() {
                                            @Override
                                            public void run() {
                                                tOutput.append(input2);
                                                try {
                                                    usbPort.write(input2.getBytes(), 100);
                                                } catch (IOException e) {
                                                    e.printStackTrace();
                                                    Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                                }
                                            }
                                        }, 500);
                                    }
                                    else Toast.makeText(MainActivity.this, "Invalid input!", Toast.LENGTH_LONG).show();
                                }
                                else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                });
                alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) { }
                });
                alert.show();
            }
        });

        bSetDelay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder alert = new AlertDialog.Builder(MainActivity.this);
                alert.setTitle("Enter activation delay in seconds");
                final EditText input = new EditText(MainActivity.this);
                input.setInputType(InputType.TYPE_CLASS_NUMBER);
                input.setRawInputType(Configuration.KEYBOARD_12KEY);
                input.setHint("10 - 16382");
                alert.setView(input);
                alert.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        final String sendText = input.getText().toString();
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(tickTagConnected) {
                                    if(sendText.length() > 0) {
                                        String input = "5\n\r";
                                        tOutput.append(input);
                                        try {
                                            usbPort.write(input.getBytes(), 100);
                                        } catch (IOException e) {
                                            e.printStackTrace();
                                            Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                            return;
                                        }
                                        final String input2 = sendText + "\n\r";
                                        new Handler().postDelayed(new Runnable() {
                                            @Override
                                            public void run() {
                                                tOutput.append(input2);
                                                try {
                                                    usbPort.write(input2.getBytes(), 100);
                                                } catch (IOException e) {
                                                    e.printStackTrace();
                                                    Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                                }
                                            }
                                        }, 500);
                                    }
                                    else Toast.makeText(MainActivity.this, "Invalid input!", Toast.LENGTH_LONG).show();
                                }
                                else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                });
                alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) { }
                });
                alert.show();
            }
        });

        bSetHour.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder alert = new AlertDialog.Builder(MainActivity.this);
                alert.setTitle("Set ON/OFF hour");

                final EditText inputOn = new EditText(MainActivity.this);
                inputOn.setInputType(InputType.TYPE_CLASS_NUMBER);
                inputOn.setRawInputType(Configuration.KEYBOARD_12KEY);
                inputOn.setHint("UTC time to turn ON (format: HHMM)");

                final EditText inputOff = new EditText(MainActivity.this);
                inputOff.setInputType(InputType.TYPE_CLASS_NUMBER);
                inputOff.setRawInputType(Configuration.KEYBOARD_12KEY);
                inputOff.setHint("UTC time to turn OFF (format: HHMM)");

                LinearLayout lay = new LinearLayout(MainActivity.this);
                lay.setOrientation(LinearLayout.VERTICAL);
                lay.addView(inputOn);
                lay.addView(inputOff);

                alert.setView(lay);
                alert.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        final String onText = inputOn.getText().toString();
                        final String offText = inputOff.getText().toString();
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(MainActivity.this, "TEST: " + onText, Toast.LENGTH_LONG).show();
                                if(tickTagConnected) {
                                    if((onText.length() > 0) && (offText.length() > 0)) {
                                        final String input1 = "6\n\r";
                                        final String input2 = onText + "\n\r";
                                        final String input3 = offText + "\n\r";

                                        // input 1
                                        tOutput.append(input1);
                                        try {
                                            usbPort.write(input1.getBytes(), 100);
                                        } catch (IOException e) {
                                            e.printStackTrace();
                                            Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                            return;
                                        }

                                        // input 2
                                        new Handler().postDelayed(new Runnable() {
                                            @Override
                                            public void run() {
                                                tOutput.append(input2);
                                                try {
                                                    usbPort.write(input2.getBytes(), 100);
                                                } catch (IOException e) {
                                                    e.printStackTrace();
                                                    Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                                    return;
                                                }
                                                // input 3
                                                new Handler().postDelayed(new Runnable() {
                                                    @Override
                                                    public void run() {
                                                        tOutput.append(input3);
                                                        try {
                                                            usbPort.write(input3.getBytes(), 100);
                                                        } catch (IOException e) {
                                                            e.printStackTrace();
                                                            Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                                            return;
                                                        }
                                                    }
                                                }, 500);
                                            }
                                        }, 500);
                                    }
                                    else Toast.makeText(MainActivity.this, "Invalid input!", Toast.LENGTH_LONG).show();
                                }
                                else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                });
                alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) { }
                });
                alert.show();
            }
        });

        bToggleGeofencing.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder alert = new AlertDialog.Builder(MainActivity.this);
                alert.setTitle("Do you want to toggle geofencing mode ON/OFF?");
                alert.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(tickTagConnected) {
                                    String input = "7\n\r";
                                    tOutput.append(input);
                                    try {
                                        usbPort.write(input.getBytes(), 100);
                                    } catch (IOException e) {
                                        e.printStackTrace();
                                        Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                    }
                                }
                                else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                });
                alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) { }
                });
                alert.show();
            }
        });

        bSetBurstDuration.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder alert = new AlertDialog.Builder(MainActivity.this);
                alert.setTitle("Enter burst duration in seconds");
                final EditText input = new EditText(MainActivity.this);
                input.setInputType(InputType.TYPE_CLASS_NUMBER);
                input.setRawInputType(Configuration.KEYBOARD_12KEY);
                input.setHint("0 - 250");
                alert.setView(input);
                alert.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        final String sendText = input.getText().toString();
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(tickTagConnected) {
                                    if(sendText.length() > 0) {
                                        String input = "8\n\r";
                                        tOutput.append(input);
                                        try {
                                            usbPort.write(input.getBytes(), 100);
                                        } catch (IOException e) {
                                            e.printStackTrace();
                                            Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                            return;
                                        }
                                        final String input2 = sendText + "\n\r";
                                        new Handler().postDelayed(new Runnable() {
                                            @Override
                                            public void run() {
                                                tOutput.append(input2);
                                                try {
                                                    usbPort.write(input2.getBytes(), 100);
                                                } catch (IOException e) {
                                                    e.printStackTrace();
                                                    Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                                }
                                            }
                                        }, 500);
                                    }
                                    else Toast.makeText(MainActivity.this, "Invalid input!", Toast.LENGTH_LONG).show();
                                }
                                else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                });
                alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) { }
                });
                alert.show();
            }
        });

        bToggleBlinking.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder alert = new AlertDialog.Builder(MainActivity.this);
                alert.setTitle("Do you want to toggle blinking mode ON/OFF?");
                alert.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(tickTagConnected) {
                                    String input = "b\n\r";
                                    tOutput.append(input);
                                    try {
                                        usbPort.write(input.getBytes(), 100);
                                    } catch (IOException e) {
                                        e.printStackTrace();
                                        Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                    }
                                }
                                else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                });
                alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) { }
                });
                alert.show();
            }
        });

        bExit.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AlertDialog.Builder alert = new AlertDialog.Builder(MainActivity.this);
                alert.setTitle("Do you want to exit the menu (TickTag goes to sleep until activation)?");
                alert.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(tickTagConnected) {
                                    String input = "9\n\r";
                                    tOutput.append(input);
                                    try {
                                        usbPort.write(input.getBytes(), 100);
                                    } catch (IOException e) {
                                        e.printStackTrace();
                                        Toast.makeText(MainActivity.this, "Failed to send command to TickTag!", Toast.LENGTH_LONG).show();
                                    }
                                }
                                else Toast.makeText(MainActivity.this, "Not connected to TickTag!", Toast.LENGTH_LONG).show();
                            }
                        });
                    }
                });
                alert.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) { }
                });
                alert.show();
            }
        });
    }
}
