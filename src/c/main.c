/*
Copyright (C) 2017 Mark Reed

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include "pebble.h"

enum {
  KEY_NOTE = 0,
  KEY_REQUEST_UPDATE
};

static Layer *window_layer;
Window *window;

TextLayer *time_layer;
TextLayer *day_layer;
TextLayer *text_layer;

BitmapLayer *layer_batt_img;
GBitmap *img_battery_100;
GBitmap *img_battery_90;
GBitmap *img_battery_80;
GBitmap *img_battery_70;
GBitmap *img_battery_60;
GBitmap *img_battery_50;
GBitmap *img_battery_40;
GBitmap *img_battery_30;
GBitmap *img_battery_20;
GBitmap *img_battery_10;
GBitmap *img_battery_charge;
int charge_percent = 0;

static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_layer;
static GBitmap *no_bluetooth_image;
static BitmapLayer *no_bluetooth_layer;

static GBitmap *background_image;
static BitmapLayer *background_layer;


void handle_battery(BatteryChargeState charge_state) {

    if (charge_state.is_charging) {
        bitmap_layer_set_bitmap(layer_batt_img, img_battery_charge);

    } else {
        if (charge_state.charge_percent <= 10) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_10);
        } else if (charge_state.charge_percent <= 20) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_20);
        } else if (charge_state.charge_percent <= 30) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_30);
		} else if (charge_state.charge_percent <= 40) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_40);
		} else if (charge_state.charge_percent <= 50) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_50);
    	} else if (charge_state.charge_percent <= 60) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_60);	
        } else if (charge_state.charge_percent <= 70) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_70);
		} else if (charge_state.charge_percent <= 80) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_80);
		} else if (charge_state.charge_percent <= 90) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_90);
		} else if (charge_state.charge_percent <= 100) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
    }
    charge_percent = charge_state.charge_percent;
  }
}

void handle_bluetooth(bool connected) {
 if(!connected) {
    //vibe!
    vibes_long_pulse();
   layer_set_hidden(bitmap_layer_get_layer(no_bluetooth_layer), !connected);	
   layer_set_hidden(text_layer_get_layer(text_layer), !connected);	
	 
  }
   layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), !connected);	
   layer_set_hidden(bitmap_layer_get_layer(no_bluetooth_layer), connected);	
   layer_set_hidden(text_layer_get_layer(text_layer), !connected);	
}

void force_update(void) {
    handle_battery(battery_state_service_peek());
    handle_bluetooth(bluetooth_connection_service_peek());

}

static void requestUpdate();

// Called once per minute
static void handle_tick(struct tm* tick_time, TimeUnits units_changed) {

  static char time_text[] = "00:00 P";
  static char day_text[] = "xxxx-xx-xx";

  char *time_format;

  if (units_changed & MINUTE_UNIT) {
    if (clock_is_24h_style()) {
      time_format = "%R";
    } else {
#ifdef PBL_PLATFORM_EMERY
      time_format = "%I:%M %p";
#else
      time_format = "%I:%M";
#endif
	}

    strftime(time_text, sizeof(time_text), time_format, tick_time);
    requestUpdate();

    // Kludge to handle lack of non-padded hour format string
    // for twelve hour clock.
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
      memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }
    text_layer_set_text(time_layer, time_text);
  }

  if (units_changed & DAY_UNIT) {
    strftime(day_text, sizeof(day_text), "%F", tick_time);
    text_layer_set_text(day_layer, day_text);
  }
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Received...");

    Tuple *tuple = dict_find(iter, KEY_NOTE);
  if (!(tuple && tuple->type == TUPLE_CSTRING)) return;

  text_layer_set_text(text_layer, tuple->value->cstring);
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped  %i", reason);
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
  /*APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sent!");*/
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!  %i", reason);
}

static void requestUpdate()
{
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, KEY_REQUEST_UPDATE, 1);
  app_message_outbox_send();
}

static void app_message_init(void) {
  // Register message handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  // Init buffers
  app_message_open(512, 64);
}

// Handle the start-up of the app
static void do_init(void) {

  // Create our app's base window
  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);
  window_layer = window_get_root_layer(window);
  
	
  background_image = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND_IMAGE);
  background_layer = bitmap_layer_create(layer_get_frame(window_layer));
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));

    img_battery_100   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_090_100);
    img_battery_90   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_080_090);
    img_battery_80   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_070_080);
    img_battery_70   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_060_070);
    img_battery_60   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_050_060);
    img_battery_50   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_040_050);
    img_battery_40   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_030_040);
    img_battery_30    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_020_030);
    img_battery_20    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_010_020);
    img_battery_10    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_000_010);
    img_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_CHARGING);


#if PBL_PLATFORM_CHALK
	layer_batt_img  = bitmap_layer_create(GRect(68, 165, 22, 7));
#elif PBL_PLATFORM_EMERY
	layer_batt_img  = bitmap_layer_create(GRect(2, 19, 22, 7));
#else
	layer_batt_img  = bitmap_layer_create(GRect(2, 19, 22, 7));
#endif
    bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
	layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));

	
#if PBL_PLATFORM_CHALK
  day_layer = text_layer_create(GRect(0, 5, 180, 16));
#elif PBL_PLATFORM_EMERY
  day_layer = text_layer_create(GRect(2, -2, 142, 16));
#else
  day_layer = text_layer_create(GRect(2, -2, 142, 16));
#endif	
  text_layer_set_text_color(day_layer, GColorWhite);
  text_layer_set_background_color(day_layer, GColorClear);
  text_layer_set_font(day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
#ifdef PBL_PLATFORM_CHALK
  text_layer_set_text_alignment(day_layer, GTextAlignmentCenter);
#else
  text_layer_set_text_alignment(day_layer, GTextAlignmentLeft);
#endif 
  layer_add_child(window_layer, text_layer_get_layer(day_layer));
	
	

#if PBL_PLATFORM_CHALK
  time_layer = text_layer_create(GRect(0, 18, 180, 34));
#elif PBL_PLATFORM_EMERY
  time_layer = text_layer_create(GRect(0, -1, 198, 34));
#else
  time_layer = text_layer_create(GRect(0, -1, 142, 34));
#endif 
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_background_color(time_layer, GColorClear);
#ifdef PBL_COLOR
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM ));
#else
  text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));	
#endif
#ifdef PBL_PLATFORM_CHALK
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
#else
  text_layer_set_text_alignment(time_layer, GTextAlignmentRight);
#endif
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

	
	
#ifdef PBL_PLATFORM_CHALK
  text_layer = text_layer_create(GRect(12, 48, 156, 120));  
#elif PBL_PLATFORM_EMERY
  text_layer = text_layer_create(GRect(15, 30, 185, 200));  
#else
  text_layer = text_layer_create(GRect(15, 30, 130, 138));  
#endif 
  text_layer_set_text_color(text_layer, GColorBlack);
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
#ifdef PBL_ROUND
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
#else
  text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
#endif
  text_layer_set_overflow_mode(text_layer, GTextOverflowModeTrailingEllipsis);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));


  bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  GRect bitmap_bounds2 = gbitmap_get_bounds(bluetooth_image);	
#ifdef PBL_ROUND
  GRect frame2 = GRect(100, 162, bitmap_bounds2.size.w, bitmap_bounds2.size.h);
#else
  GRect frame2 = GRect(28, 16, bitmap_bounds2.size.w, bitmap_bounds2.size.h);
#endif	
  bluetooth_layer = bitmap_layer_create(frame2);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));

	
	
  no_bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WARNING);
  GRect bitmap_bounds3 = gbitmap_get_bounds(no_bluetooth_image);	
#ifdef PBL_ROUND
  GRect frame3 = GRect(30, 53, bitmap_bounds3.size.w, bitmap_bounds3.size.h);
#else
  GRect frame3 = GRect(15, 47, bitmap_bounds3.size.w, bitmap_bounds3.size.h);
#endif
  no_bluetooth_layer = bitmap_layer_create(frame3);
  bitmap_layer_set_bitmap(no_bluetooth_layer, no_bluetooth_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(no_bluetooth_layer));

	
	
  app_message_init();
  requestUpdate();

    // Update the screen 
  time_t now = time(NULL);
  handle_tick(localtime(&now), MINUTE_UNIT | HOUR_UNIT | DAY_UNIT );
  // And then every second
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);

	
  // handlers
    battery_state_service_subscribe(&handle_battery);
    bluetooth_connection_service_subscribe(&handle_bluetooth);
	
  // draw first frame
    force_update();
}

static void do_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
	
  text_layer_destroy(time_layer);
  text_layer_destroy(day_layer);
	
  layer_remove_from_parent(bitmap_layer_get_layer(bluetooth_layer));
  bitmap_layer_destroy(bluetooth_layer);
  gbitmap_destroy(bluetooth_image);
  bluetooth_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);
  background_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_batt_img));
  bitmap_layer_destroy(layer_batt_img);
	
  gbitmap_destroy(img_battery_100);
  gbitmap_destroy(img_battery_90);
  gbitmap_destroy(img_battery_80);
  gbitmap_destroy(img_battery_70);
  gbitmap_destroy(img_battery_60);
  gbitmap_destroy(img_battery_50);
  gbitmap_destroy(img_battery_40);
  gbitmap_destroy(img_battery_30);
  gbitmap_destroy(img_battery_20);
  gbitmap_destroy(img_battery_10);
  gbitmap_destroy(img_battery_charge);		
	
  window_destroy(window);

}

// The main event/run loop for our app
int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}
