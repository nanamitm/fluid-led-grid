package net.nanamitm.fluidledgrid;

import android.os.Bundle;
import android.view.WindowManager;
import org.qtproject.qt.android.bindings.QtActivity;

public class FluidLedGridActivity extends QtActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // onCreate() で Window が確実に存在するためここで設定するのが最も確実
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }
}
