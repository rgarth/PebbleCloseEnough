#include <pebble.h>
#include <ctype.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_UNITS 2
#define KEY_JSREADY 3
  
#define MyTupletCString(_key, _cstring) \
((const Tuplet) { .type = TUPLE_CSTRING, .key = _key, .cstring = { .data = _cstring, .length = strlen(_cstring) + 1 }})
  
static Window *s_main_window; 
static TextLayer *s_time_layer;
AppTimer *shake_timer;
static int shake_counter = 0;

// Making this global
static int temperature;
static char conditions_buffer[32];
// First time we run there is no stored value
static char temp_units[9] = "imperial";

static void show_time() {
  // Setup arrays for text time
  char *minute_text[12];
    minute_text[0] = "";
    minute_text[1] = "\nFive\npast\n";
    minute_text[2] = "\nTen\npast\n";
    minute_text[3] = "\nQuarter\npast\n";
    minute_text[4] = "\nTwenty\npast\n";
    minute_text[5] = "\nTwenty\nfive\npast\n";
    minute_text[6] = "\nHalf\npast\n";
    minute_text[7] = "\nTwenty\nfive\nto\n";
    minute_text[8] = "\nTwenty\nto\n";
    minute_text[9] = "\nQuarter\nto\n";
    minute_text[10] = "\nTen\nto\n";
    minute_text[11] = "\nFive\nto\n";
  
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
    snprintf(buffer, 25, "\n\n%s o\'clock", hour_text[hours]);
  } else {
    snprintf(buffer, 25, "%s%s", minute_text[minutes / 5], hour_text[hours]);
  }
  
  // Display this time on the TextLayer
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  text_layer_set_text(s_time_layer, buffer);
} 

static void show_date(){
  
  char *mon_text[12];
    mon_text[0] = "\n\nJan ";
    mon_text[1] = "\n\nFeb ";
    mon_text[2] = "\n\nMar ";
    mon_text[3] = "\n\nApr ";
    mon_text[4] = "\n\nMay ";
    mon_text[5] = "\n\nJun ";
    mon_text[6] = "\n\nJul ";
    mon_text[7] = "\n\nAug ";
    mon_text[8] = "\n\nSep ";
    mon_text[9] = "\n\nOct ";
    mon_text[10] = "\n\nNov ";
    mon_text[11] = "\n\nDec ";
  // We're going to say it's midnight at 11:58, so we might as well lie about the date as well
  // Simplified this. Easiest to think that we just run 3 minutes early all the time.
  time_t temp = time(NULL) + 180;
  
  int mday = localtime(&temp)->tm_mday;
  int mon = localtime(&temp)->tm_mon;

  static char buffer[25]= "";
  snprintf(buffer, 25, "%s%i", mon_text[mon], mday);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  text_layer_set_text(s_time_layer, buffer); 
}

static void show_weather(){
  static char buffer[25]= "";
  snprintf(buffer, 25, "\n\n%i\u00B0\n%s", temperature, conditions_buffer);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);
  text_layer_set_text(s_time_layer, buffer); 
}

static void shake_timer_callback(void *date) {
  shake_counter = 0;
  app_timer_cancel(shake_timer);
  show_time();
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  switch(shake_counter){
  case 0: 
    // Lets assume the first shake was for the back light
    shake_timer = app_timer_register(10000, (AppTimerCallback) shake_timer_callback, NULL);
    break;
  case 1:
    show_date();
    // reset the time to replace date with time after 4 seconds
    app_timer_reschedule(shake_timer, 4000);
    break;
  case 2:
    show_weather();
    // reset the timer to replace date with time after 4 seconds
    app_timer_reschedule(shake_timer, 4000);
    break;
  }
  shake_counter = shake_counter + 1;
}

static void update_weather() {
    
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  
  Tuplet tuple = MyTupletCString(KEY_UNITS, temp_units);

  // Add a key-value pair
  dict_write_tuplet(iter, &tuple);

  // Send the message!
  app_message_outbox_send();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  show_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0){
    update_weather();
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
  s_time_layer = text_layer_create(GRect(0, 6, 144, 156));
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorWhite);

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentLeft);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
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