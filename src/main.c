#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_UNITS 2
#define KEY_JSREADY 3

#define MyTupletCString(_key, _cstring) \
  ((const Tuplet) { .type = TUPLE_CSTRING, .key = _key, .cstring = { .data = _cstring, .length = strlen(_cstring) + 1 }})

static Window *s_main_window, *s_time_window, *s_date_window;
static TextLayer *s_main_text_layer, *s_time_text_layer, *s_date_text_layer, *s_weather_text_layer;
static bool tap_registered = false;
AppTimer *shake_timer;

// Variables for storing weather information
char *temp_units = "us";
int temperature;
char conditions[32];

// default time to show extra windows
int default_timeout = 3000;


static void v_align_text_layer(TextLayer *text_layer) {
  // simple function take a texy_layer and align it vertically
  // on the window. The assummption is that the layer's parent is
  // the window
  layer_set_frame(text_layer_get_layer(text_layer), GRect(0, 0, 144, 168));
  GSize text_size = (text_layer_get_content_size(text_layer));
  // Cutting the bottom of 'g'
  int text_height = text_size.h + 5;
  int y_origin = (168/2) - (text_height/2);
  if (y_origin > 0) {
    text_layer_set_size(text_layer, text_size);
    layer_set_frame(text_layer_get_layer(text_layer), GRect(0, y_origin, 144, text_height));
  }
}

static void update_weather() {
  // Only grab the weather if we can talk to phone
  if (bluetooth_connection_service_peek()) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone is connected!");

    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    Tuplet tuple = MyTupletCString(KEY_UNITS, temp_units);

    // Add a key-value pair
    dict_write_tuplet(iter, &tuple);

    // Send the message!
    app_message_outbox_send();

  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone is not connected!");
  }  
}

static void show_time() {
  // Setup arrays for text time
  char *minute_text[12];
    minute_text[0] = "";
    minute_text[1] = "Five\npast\n";
    minute_text[2] = "Ten\npast\n";
    minute_text[3] = "Quarter\npast\n";
    minute_text[4] = "Twenty\npast\n";
    minute_text[5] = "Twenty\nfive\npast\n";
    minute_text[6] = "Half\npast\n";
    minute_text[7] = "Twenty\nfive\nto\n";
    minute_text[8] = "Twenty\nto\n";
    minute_text[9] = "Quarter\nto\n";
    minute_text[10] = "Ten\nto\n";
    minute_text[11] = "Five\nto\n";

  char *hour_text[12];
    hour_text[0] = "Twelve";
    hour_text[1] = "One";
    hour_text[2] = "Two";
    hour_text[3] = "Three";
    hour_text[4] = "Four";
    hour_text[5] = "Five";
    hour_text[6] = "Six";
    hour_text[7] = "Seven";
    hour_text[8] = "Eight";
    hour_text[9] = "Nine";
    hour_text[10] = "Ten";
    hour_text[11] = "Eleven";
  
  // Get a tm structure
  time_t temp = time(NULL);

  // We want minutes to the nearest 5 minute interval
  int minutes = localtime(&temp)->tm_min;
  int remainder = ( minutes % 5 );
  minutes = (minutes - remainder);
  if ( remainder > 2 ) {
    minutes = minutes + 5;
  }

  int hours = localtime(&temp)->tm_hour;
  // 12 Hour Time
  if (hours > 11) { hours = hours - 12; }
  // Inrease hour if we are counting toward
  if (minutes > 30) { hours = hours + 1; }
  // 11 is the new 12, better run that again
  if (hours > 11) { hours = hours - 12; }
  // 60 passed the hour looks a little borked
  if (minutes > 55) { minutes = 0; }

  // Create a long-lived buffer
  static char buffer[25] = "";
  if ((minutes / 5) == 0) {
    snprintf(buffer, 25, "%s o\'clock", hour_text[hours]);
  } else {
    snprintf(buffer, 25, "%s%s", minute_text[minutes / 5], hour_text[hours]);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_main_text_layer, buffer);
  v_align_text_layer(s_main_text_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      temperature = t->value->int32;
      APP_LOG(APP_LOG_LEVEL_INFO, "Temperature %d!", temperature);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions, sizeof(conditions), "%s", t->value->cstring);
      APP_LOG(APP_LOG_LEVEL_INFO, "Conditions %s!", conditions);
      break;
    case KEY_UNITS:
      // units returned from configuration.js
      // we should regrab the weather in the correct units
      snprintf(temp_units, sizeof(temp_units), "%s", t->value->cstring);
      persist_write_string(KEY_UNITS, temp_units);
      APP_LOG(APP_LOG_LEVEL_INFO, "Storing temperature units: %s", temp_units);
      update_weather();
      break;
    case KEY_JSREADY:
      APP_LOG(APP_LOG_LEVEL_INFO, "PebbleJS is ready!");
      if (t->value->int16) {
        update_weather();
      }
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  show_time();

  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0){
    update_weather();
  }

}

static void bt_handler(bool connected) {
  if (connected) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone has connected!");
    vibes_short_pulse();
    update_weather();

  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "Phone has disconnected!");
    vibes_short_pulse();
  }
}

static void shake_timer_callback(void *date) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Entered timer callback");
  if (window_stack_contains_window(s_time_window)) {
    shake_timer = app_timer_register(default_timeout, (AppTimerCallback) shake_timer_callback, NULL);
    window_stack_push(s_date_window, true);
    window_stack_remove(s_time_window, false); 
  } else if (window_stack_contains_window(s_date_window)) {
    window_stack_pop(true);
    tap_registered = false;
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (! tap_registered) {
    tap_registered = true;
    
    if (! temperature) {
      update_weather();
    }
    window_stack_push(s_time_window, true);
    shake_timer = app_timer_register(default_timeout, (AppTimerCallback) shake_timer_callback, NULL);
  }
}


static void main_window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Loaded main window");
  
  // Create time TextLayer
  s_main_text_layer = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_background_color(s_main_text_layer, GColorClear);
  text_layer_set_text_color(s_main_text_layer, GColorWhite);
  text_layer_set_font(s_main_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_main_text_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(s_main_text_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_main_text_layer));
  v_align_text_layer(s_main_text_layer);
}

static void main_window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Unloaded main window");
  text_layer_destroy(s_main_text_layer);
}

static void time_window_load(Window *window){
  APP_LOG(APP_LOG_LEVEL_INFO, "Loaded time window");
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
    // Want to strip the leading 0. Not just replace it with a *space*
    // which is what %l does
    if (buffer[0] == '0')
      memmove(buffer, buffer+1, strlen(buffer));
  }
  
  s_time_text_layer = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_background_color(s_time_text_layer, GColorClear);
  text_layer_set_text_color(s_time_text_layer, GColorWhite);
  text_layer_set_font(s_time_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_time_text_layer, GTextAlignmentCenter);
  // Display this time on the TextLayer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_text_layer));
  text_layer_set_text(s_time_text_layer, buffer);
  v_align_text_layer(s_time_text_layer);

}

static void time_window_unload(Window *window){
  APP_LOG(APP_LOG_LEVEL_INFO, "Unloaded time window");
  text_layer_destroy(s_time_text_layer);
}

static void date_window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Loaded date window");
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char d_buffer[7];
  static char w_buffer[32];
  strftime(d_buffer, sizeof(d_buffer), "%b %d", tick_time);
  s_date_text_layer = text_layer_create(GRect(0, 50, 144, 34));
  text_layer_set_background_color(s_date_text_layer, GColorClear);
  text_layer_set_text_color(s_date_text_layer, GColorWhite);
  text_layer_set_font(s_date_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_date_text_layer, GTextAlignmentCenter);
  // Display the date on the TextLayer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_text_layer));
  text_layer_set_text(s_date_text_layer, d_buffer);
  
  s_weather_text_layer = text_layer_create(GRect(0,84,144,84));
  text_layer_set_background_color(s_weather_text_layer, GColorClear);
  text_layer_set_text_color(s_weather_text_layer, GColorWhite);
  text_layer_set_font(s_weather_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_weather_text_layer, GTextAlignmentCenter);
  // Display the weather on the TextLayer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_text_layer));
  APP_LOG(APP_LOG_LEVEL_INFO, "Temperature: %i!!", temperature);
  if (temperature) {
    snprintf(w_buffer, sizeof(w_buffer), "%i\u00B0", temperature);
    if (conditions[0] != '\0') {
      snprintf(w_buffer, sizeof(w_buffer), "%i\u00B0 and %s", temperature, conditions);
    }
    text_layer_set_text(s_weather_text_layer, w_buffer);
  }
  
}

static void date_window_unload(Window *window) {
  text_layer_destroy(s_date_text_layer);
  text_layer_destroy(s_weather_text_layer);
  
}

static void init () {
  // Load stored values
  if (persist_exists(KEY_UNITS)) {
    persist_read_string(KEY_UNITS, temp_units, sizeof(temp_units));
    APP_LOG(APP_LOG_LEVEL_INFO, "Reading temperature units: %s", temp_units);
  }

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Create main Window element and assign to pointer.
  // And push it, push it real good.
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  }); 
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  show_time();

  // Create Time Window, but don't push it
  s_time_window = window_create();
  window_set_background_color(s_time_window, GColorBlack);
  window_set_window_handlers(s_time_window, (WindowHandlers) {
    .load = time_window_load,
    .unload = time_window_unload
  });
  
  //Creat Date Window, but don't push it
  s_date_window = window_create();
  window_set_background_color(s_date_window, GColorBlack);
  window_set_window_handlers(s_date_window, (WindowHandlers) {
    .load = date_window_load,
    .unload = date_window_unload
  });

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // Register with the BluetoothConnectionService
  bluetooth_connection_service_subscribe(bt_handler);  
  // Register with  AccelTimeService
  accel_tap_service_subscribe(tap_handler);
}

static void deinit () {
  // Destroy Window
  window_destroy(s_main_window);
  window_destroy(s_time_window);
  window_destroy(s_date_window);
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  accel_tap_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
