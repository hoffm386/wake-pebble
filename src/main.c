#include <pebble.h>
  
#define KEY_ASLEEP 0
#define KEY_BUTTON_PRESSED 1
#define KEY_USER_NODDED 2
  
static Window *s_main_window;
static TextLayer *s_time_layer;

static GFont s_time_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap_logo;
static GBitmap *s_background_bitmap_wake_up;

int num_queries = 0;

static void main_window_load(Window *window) {
  //Create GBitmaps, then set logo one to created BitmapLayer
  s_background_bitmap_logo = gbitmap_create_with_resource(RESOURCE_ID_LOGO_IMAGE);
  s_background_bitmap_wake_up = gbitmap_create_with_resource(RESOURCE_ID_WAKE_UP_IMAGE);
  
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap_logo);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Number of total queries: %d", num_queries);
}

static void main_window_unload(Window *window) {
  
  //Destroy GBitmaps
  gbitmap_destroy(s_background_bitmap_logo);
  gbitmap_destroy(s_background_bitmap_wake_up);

  //Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  
  // Get sleep status update every 10 seconds
  if(tick_time->tm_sec % 10 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Tell the JS to query for ASLEEP
    dict_write_uint8(iter, KEY_ASLEEP, 5);

    // Send the message!
    app_message_outbox_send();
    
    // Log
    num_queries += 1;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Number of total queries: %d", num_queries);

  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_ASLEEP: {
      int asleep = (int8_t)t->value->int8;
      if (asleep) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Asleep was true");
        bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap_wake_up);
        // Vibe pattern: ON for 1000ms, OFF for 800ms, ON for 1000ms:
        static uint32_t const segments[] = { 1000, 800, 1000, 800, 1000, 800, 1000 };
        VibePattern pat = {
          .durations = segments,
          .num_segments = ARRAY_LENGTH(segments),
        };
        vibes_enqueue_custom_pattern(pat);
      } else {
        APP_LOG(APP_LOG_LEVEL_INFO, "Asleep was false");
        bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap_logo);
      }
      
      break;
    }
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

static void click_handler(ClickRecognizerRef recognizer, void *context) {
  vibes_cancel();
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  // Tell the JS to inform the server that the user pressed a button
  dict_write_uint8(iter, KEY_BUTTON_PRESSED, 5);

  // Send the message!
  app_message_outbox_send();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, click_handler);
}

static void data_handler(AccelData *data, uint32_t num_samples) {
  uint32_t above_zero_count = 0;
  uint32_t i;
  for (i=0; i<num_samples; i++) {
    if (data[i].z > 0) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Z above 0: %d", data[i].z);
      above_zero_count++;
    }
  }
  
  if (above_zero_count > (4*num_samples/5)) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Tell the JS to inform the server that the user nodded their head
    dict_write_uint8(iter, KEY_USER_NODDED, 5);
  
    // Send the message!
    app_message_outbox_send();
  }

}
  
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  window_set_click_config_provider(s_main_window, click_config_provider);
  
  uint32_t num_samples = 20;
  accel_data_service_subscribe(num_samples, data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
  
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
