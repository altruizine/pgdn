package com.github.altruizine.Buttonmap;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class BootReceiver extends BroadcastReceiver
{
    @Override
    public void onReceive(Context context, Intent intent)
    {
	// Run the actual task as a Service, not as a
	// BroadcastReceiver, to avoid us being killed after 10s
        Intent i = new Intent(context, BootService.class);
        context.startService(i);
    }
}
