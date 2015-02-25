#include <pebble.h>
  
static Window *s_main_window; 
static TextLayer *s_time_layer;


static void update_time() {
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
  
  char *hour_text[11];
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
  minutes = minutes - remainder;
  if ( remainder > 2 ) {
    minutes = minutes + 5;
  }
  
  // Only want 12 hour time
  int hours = localtime(&temp)->tm_hour;
  if ( hours > 11) {
    hours = hours - 12;
  }
  // Inrease hour if we are counting toward
  if ( minutes > 30) { hours = hours + 1; }
  
  // Create a long-lived buffer
  static char buffer[25] = "";
  if ((minutes / 5) == 0) {
    snprintf(buffer, 25, "%s o\'clock", hour_text[hours]);
  } else {
    snprintf(buffer, 25, "%s%s", minute_text[minutes / 5], hour_text[hours]);
  }
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
} 

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 36, 144, 168));
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
  update_time();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

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