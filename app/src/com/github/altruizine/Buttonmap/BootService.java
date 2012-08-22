package com.github.altruizine.Buttonmap;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import android.app.Service;
import android.content.Intent;
import android.content.Context;
import android.os.IBinder;

public class BootService extends Service
{
    private static final String TAG = "Buttonmap";

    @Override
    public void onStart(Intent intent, int startId)
    {
        super.onStart(intent, startId);
        
	String daemon = getFilesDir().getAbsolutePath() + "/pgdn";

	try {
	    // Make sure we've unpacked the pgdn daemon.
	    File f = new File(daemon);
	    if (! f.exists()) {
		InputStream is = getAssets().open("pgdn");
		byte[] buffer = new byte[is.available()];
		is.read(buffer);
		is.close();

		FileOutputStream os = new FileOutputStream(f);
		os.write(buffer);
		os.close();

		// f.setExecutable(true); -- not available in Froyo
		Runtime.getRuntime().exec("chmod 755 " + daemon).waitFor();
	    }

	    // Make sure we have the configuration file on the SD card.
	    // Should we wait till SD Card is mounted?  Probably not --
	    // the internal SD card should be mounted at this point.

	    // Unpack the configuration file unconditionally to
	    // /sdcard/.pgdn.dist.  Copy it to /sdcard/.pgdn if that does
	    // not exist yet.
	    InputStream is = getAssets().open("dot.pgdn");
	    byte[] buffer = new byte[is.available()];
	    is.read(buffer);
	    is.close();

	    f = new File("/sdcard/.pgdn.dist");
	    FileOutputStream os = new FileOutputStream(f);
	    os.write(buffer);
	    os.close();

	    f = new File("/sdcard/.pgdn");
	    if (! f.exists()) {
		os = new FileOutputStream(f);
		os.write(buffer);
		os.close();
	    }

	    // Now run the daemon as root.

	    String[] args = new String[] { "su", "-c", daemon };
				       
	    Process proc = Runtime.getRuntime().exec(args);
	    proc.waitFor();

	}
	catch (Exception e) {
	    throw new RuntimeException(e);
	}
	finally {
	    stopSelf();
	}
    }

    @Override
    public IBinder onBind(Intent arg0) {
        return null;
    }
}
