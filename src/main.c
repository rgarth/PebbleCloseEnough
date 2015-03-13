#include <pebble.h>
#include <ctype.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_UNITS 2
#define KEY_JSREADY 3
  
#define MyTupletCString(_key, _cstring) \
((const Tuplet) { .type = TUPLE_CSTRING, .key = _key, .cstring = { .data = _cstring, .length = strlen(_cstring) + 1 }})

// Default length for extra watch faces 2 seconds
static int default_timeout = (2 * 1000);

static Window *s_main_window; 
static TextLayer *s_text_layer;
static TextLayer *s_numeral_layer;
AppTimer *shake_timer;
static int face_counter = 0;

// Making this global
static int temperature;
static char conditions_buffer[32];
// First time we run there is no stored value
static char temp_units[9] = "us";

static void v_align() {
  layer_set_frame(text_layer_get_layer(s_text_layer), GRect(0, 0, 144, 168));  
  GSize text_size = (text_layer_get_content_size(s_text_layer));
  int text_height = text_size.h + 5;
  int y_origin = (168/2) - (text_height/2);
  text_layer_set_size(s_text_layer, text_size);
  layer_set_frame(text_layer_get_layer(s_text_layer), GRect(0, y_origin, 144, text_height)); 
}

static void show_time() {
  layer_set_hidden(text_layer_get_layer(s_text_layer), 0);
  layer_set_hidden(text_layer_get_layer(s_numeral_layer), 1);
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
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentLeft);
  text_layer_set_text(s_text_layer, buffer);
  v_align();
} 

static void show_date(){  // and weather
  layer_set_hidden(text_layer_get_layer(s_text_layer), 0);
  layer_set_hidden(text_layer_get_layer(s_numeral_layer), 1);
  char *mon_text[12];
    mon_text[0] = "Jan ";
    mon_text[1] = "Feb ";
    mon_text[2] = "Mar ";
    mon_text[3] = "Apr ";
    mon_text[4] = "May ";
    mon_text[5] = "Jun ";
    mon_text[6] = "Jul ";
    mon_text[7] = "Aug ";
    mon_text[8] = "Sep ";
    mon_text[9] = "Oct ";
    mon_text[10] = "Nov ";
    mon_text[11] = "Dec ";
  time_t temp = time(NULL);
  
  int mday = localtime(&temp)->tm_mday;
  int mon = localtime(&temp)->tm_mon;

  static char buffer[25]= "";
  if (temperature) {
    snprintf(buffer, 25, "%s%i\n%i\u00B0", mon_text[mon], mday, temperature);
  } else {
    snprintf(buffer, 25, "%s%i", mon_text[mon], mday);    
  }
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentRight);
  text_layer_set_text(s_text_layer, buffer);
  v_align();
}

static void show_real_time() {
  // Get a tm structure
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

  // Display this time on the TextLayer
  text_layer_set_text(s_numeral_layer, buffer);
  layer_set_hidden(text_layer_get_layer(s_text_layer), 1);
  layer_set_hidden(text_layer_get_layer(s_numeral_layer), 0);
}

static void shake_timer_callback(void *date) {
  switch (face_counter) {
  case 1:
    show_date();
    face_counter = 2;
    shake_timer = app_timer_register(default_timeout, (AppTimerCallback) shake_timer_callback, NULL);
    break;
  case 2:
    show_time();
    face_counter = 3;
    shake_timer = app_timer_register(default_timeout, (AppTimerCallback) shake_timer_callback, NULL);
    break;
  default:
    face_counter = 0;
    //light_enable(0);
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (face_counter == 0) {
    // Turn on the Back light explicitly, will return it to pebble control in the timer_callback
    //light_enable(1);
    show_real_time();
    face_counter = 1;
    shake_timer = app_timer_register(default_timeout, (AppTimerCallback) shake_timer_callback, NULL);
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
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      APP_LOG(APP_LOG_LEVEL_INFO, "Conditions %s!", conditions_buffer);
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

static void main_window_load(Window *window) {
  // Create time TextLayer
  s_text_layer = text_layer_create(GRect(0, 6, 144, 156));
  text_layer_set_background_color(s_text_layer, GColorBlack);
  text_layer_set_text_color(s_text_layer, GColorWhite);
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentLeft);

  // Create hidden tet layer for showing accurate time in digits
  s_numeral_layer = text_layer_create(GRect(0, 60, 144, 49));
  layer_set_hidden(text_layer_get_layer(s_numeral_layer), 1);
  text_layer_set_background_color(s_numeral_layer, GColorBlack);
  text_layer_set_text_color(s_numeral_layer, GColorWhite);
  text_layer_set_font(s_numeral_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_numeral_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_text_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_numeral_layer));

}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_text_layer);
  text_layer_destroy(s_numeral_layer);
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
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  window_set_background_color(s_main_window, GColorBlack); 

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  show_time();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register with  AccelTimeService
  // A time will show to date
  accel_tap_service_subscribe(tap_handler); 
  
  // Register with the BluetoothConnectionService
  bluetooth_connection_service_subscribe(bt_handler); 
}

static void deinit () {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}