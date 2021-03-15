package mp.paschalis.llvm.demo;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Typeface;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class MainActivity extends AppCompatActivity {
  private volatile boolean run_once = false;
  final RunTests tests = new RunTests(this);

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    final ListView listView = findViewById(R.id.listViewHfOptions);

    String [] unitTestsArr =
      getResources().getStringArray(R.array.unit_test_labels);
    final List<String> unitTests= new ArrayList<String>(Arrays.asList(unitTestsArr));

    // Create an ArrayAdapter from List
    final ArrayAdapter<String> arrayAdapter = new ArrayAdapter<String>
      (this, android.R.layout.simple_list_item_1, unitTests){
      @Override
      public View getView(int position, View convertView, ViewGroup parent){
        View view = super.getView(position,convertView,parent);
        TextView textView = (TextView) super.getView(position, convertView, parent);
        ModifyView(getApplicationContext(), view,textView, unitTests.get(position));
        return textView;
      }

      @Override
      public boolean isEnabled(int position) {
        return true;
        // String tuDivisor=  getApplicationContext().getString(R.string.DIVISOR_LINE);
        // return !unitTests.get(position).equals(tuDivisor);
      }
    };

    listView.setAdapter(arrayAdapter);

    listView.setOnItemClickListener(
      new AdapterView.OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
          handleClick(position, false);
        }
      });
    listView.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
      @Override
      public boolean onItemLongClick(AdapterView<?> parent, View view, int position, long id) {
        handleClick(position, true);
        return true;
      }
    });
  }

  void handleClick(int position, boolean isLongPress) {
    String value = getResources().getStringArray(R.array.unit_test_labels)[position];
    if (RunTests.Match(getApplicationContext(), value, R.string.TEST_ALL)) {
      tests.RunAll();
    } else {
      tests.RunSingle(value);
    }
    int duration = Toast.LENGTH_SHORT;
    String msg = CheckTest.GetReport();
    if(CheckTest.HasFailedTests()) {
      duration = Toast.LENGTH_LONG;
      msg+="\n" + CheckTest.GetErrors();
    }
    Toast toast = Toast.makeText(getApplicationContext(), msg, duration);
    int color= ContextCompat.getColor(getApplicationContext(), android.R.color.holo_blue_dark);
    if(CheckTest.HasFailedTests()) {
      color = ContextCompat.getColor(getApplicationContext(), android.R.color.holo_red_dark);
    }

    toast.getView().setBackgroundColor(color);
    toast.show();
  }

  class RunTasks extends AsyncTask<String, Void, Void> {

    final Button button;

    public RunTasks(Button button) {
      this.button = button;
    }

    @SuppressLint("WrongThread")
    @Override
    protected Void doInBackground(String... s) {
      try {
        Thread.sleep(200);
        button.performClick();
      } catch (InterruptedException e) {
        e.printStackTrace();
      }
      return null;
    }
  }

  void ModifyView(Context ctx, View view, TextView tv, String testUnit) {

    tv.setMinWidth(0);
    tv.setMinimumWidth(0);
    tv.getLayoutParams().width = ViewGroup.LayoutParams.MATCH_PARENT;
    tv.setGravity(Gravity.NO_GRAVITY);

    String tuAll =  ctx.getString(R.string.TEST_ALL);
    String tuDemo=  ctx.getString(R.string.DEMO);
    String tuDivisor=  ctx.getString(R.string.DIVISOR_LINE);

    tv.setTypeface(null, Typeface.NORMAL);
    tv.setTextSize(15);
    if(testUnit.equals(tuAll)) {
      int color = ContextCompat.getColor(ctx, R.color.colorPrimaryDark);
      tv.setTextColor(color);
      tv.setTypeface(null, Typeface.BOLD_ITALIC);
    } else if(testUnit.equals(tuDemo)) {
      int color = ContextCompat.getColor(ctx, R.color.colorAccent);
      tv.setTextSize(40);
      tv.setTextColor(color);
      tv.setTypeface(null, Typeface.BOLD_ITALIC);
    } else if(testUnit.equals(tuDivisor)) {
      int color = ContextCompat.getColor(ctx, android.R.color.darker_gray);
      tv.setTextColor(color);
      tv.setTextSize(20);
    } else if(testUnit.contains("[dies]")) {
      int color = ContextCompat.getColor(ctx, android.R.color.darker_gray);
      tv.setTextColor(color);
    } else {
      int color = ContextCompat.getColor(ctx, android.R.color.black);
      tv.setTextColor(color);
    }
  }
}
