#include <pebble.h>
#define WORKING_TIME_DEFAULT 3120
#define RESTING_TIME_DEFAULT 1020

static Window *window;
static TextLayer *time_text_layer;
static TextLayer *status_text_layer;
static char timer_text[5];
static int timer_addition = WORKING_TIME_DEFAULT;
static bool timer_running = false;
static WakeupId s_wakeup_id = 1;
static time_t s_wakeup_timestamp;
static int timer_status;
static time_t time_remaining;
static time_t wakeup_time;

enum Storage {
  PERSIST_WAKEUP,
  PERSIST_PAUSED,
  PERSIST_STATUS
};

enum Status {
  WORKING,
  RESTING,
  PAUSED_WORKING,
  PAUSED_RESTING
};

static void set_defaults () {
  timer_status = WORKING;
  timer_addition = WORKING_TIME_DEFAULT;
  time_remaining = WORKING_TIME_DEFAULT;
  timer_running = false;
}

static void set_background_color() {
  if (timer_status == WORKING || timer_status == PAUSED_WORKING) {
    window_set_background_color(window, GColorBlack);
    text_layer_set_background_color(status_text_layer, GColorBlack);
    text_layer_set_background_color(time_text_layer, GColorBlack);
    text_layer_set_text_color(status_text_layer, GColorWhite);
    text_layer_set_text_color(time_text_layer, GColorWhite);
  }
  else if (timer_status == RESTING || timer_status == PAUSED_RESTING) {
    window_set_background_color(window, GColorWhite);
    text_layer_set_background_color(status_text_layer, GColorWhite);
    text_layer_set_background_color(time_text_layer, GColorWhite);
    text_layer_set_text_color(status_text_layer, GColorBlack);
    text_layer_set_text_color(time_text_layer, GColorBlack);
  }
}

static void add_status_text() {
  if (timer_status == PAUSED_WORKING || timer_status == PAUSED_RESTING) {
    text_layer_set_text(status_text_layer, "Paused");
  }
  else if (timer_status == WORKING) {
    text_layer_set_text(status_text_layer, "Working");
  }
  else if (timer_status == RESTING) {
    text_layer_set_text(status_text_layer, "Resting");
  }
}

static void toggle_status() {
  if (timer_status == WORKING) {
    timer_status = RESTING;
  }
  else if (timer_status == RESTING) {
    timer_status = WORKING;
  }
}

static void set_timer_length () {
  if (timer_status == WORKING) {
    timer_addition = WORKING_TIME_DEFAULT; 
  }
  else if (timer_status == RESTING) {
    timer_addition = RESTING_TIME_DEFAULT;
  }
}

static void display_time_text() {
  if (wakeup_query(s_wakeup_id, &s_wakeup_timestamp)) {
    int countdown_display = s_wakeup_timestamp - time(NULL);
    if ((countdown_display % 60) <= 10) {
      snprintf(timer_text, sizeof(timer_text), "%d", countdown_display / 60); // Set the minutes displayed so that they don't drop too quickly
    }
    else {
      int countdown_display_rounded_up = (countdown_display / 60) + 1;
      snprintf(timer_text, sizeof(timer_text), "%d", countdown_display_rounded_up);
    }
    text_layer_set_text(time_text_layer, timer_text);
    layer_mark_dirty(text_layer_get_layer(time_text_layer));
  }
  else if (!(timer_status == PAUSED_WORKING || timer_status == PAUSED_RESTING)) {
    snprintf(timer_text, sizeof(timer_text), "%d", timer_addition / 60);
    text_layer_set_text(time_text_layer, timer_text);
    layer_mark_dirty(text_layer_get_layer(time_text_layer));
  }
}

static void set_working_text() {
  text_layer_set_text(status_text_layer, "Time to Work");
  display_time_text();
}

static void set_resting_text() {
  text_layer_set_text(status_text_layer, "Time to Rest");
  display_time_text();
}

void register_time (struct tm *tick_time, TimeUnits units_changed) {
  display_time_text();
}

static void set_timer () {
  timer_running = true;
    if (timer_status == PAUSED_RESTING || timer_status == PAUSED_WORKING) {
      wakeup_time = time(NULL) + time_remaining;
      s_wakeup_id = wakeup_schedule(wakeup_time, time_remaining, true);
      if (timer_status == PAUSED_WORKING) {
        timer_status = WORKING;
      }
      else if (timer_status == PAUSED_RESTING) {
        timer_status = RESTING;
      }
    }
    else {
      wakeup_time = time(NULL) + timer_addition;
      s_wakeup_id = wakeup_schedule(wakeup_time, timer_addition, true);
    }
    set_background_color();
    add_status_text();
    display_time_text();
}

static void start_and_pause_timer() {
  if (timer_running == false) {
    set_timer();    
  }
  else {
    if (wakeup_query(s_wakeup_id, &s_wakeup_timestamp)) {
      timer_running = false;
      if (timer_status == WORKING) {
        timer_status = PAUSED_WORKING;
      }
      else if (timer_status == RESTING) {
        timer_status = PAUSED_RESTING;
      }
      add_status_text();
      time_remaining = s_wakeup_timestamp - time(NULL);
      wakeup_cancel_all();
      s_wakeup_id = 1;
    }
  }
}

static void end_countdown() {
  vibes_double_pulse();
  toggle_status();
  set_timer_length();
  set_background_color();
  if (timer_status == WORKING) {
    set_working_text();
  }
  else if (timer_status == RESTING) {
    set_resting_text();
  }
  if (persist_exists(PERSIST_WAKEUP)) {
    persist_delete(PERSIST_WAKEUP);
  }
  if (persist_exists(PERSIST_PAUSED)){
    persist_delete(PERSIST_PAUSED);
  }
  timer_running = false;
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {  
  start_and_pause_timer();
}

static void long_press_handler(ClickRecognizerRef recognizer, void *context) {
  set_defaults();
  if (persist_exists(PERSIST_WAKEUP)) {
    persist_delete(PERSIST_WAKEUP);
  }
  if (persist_exists(PERSIST_STATUS)) {
    persist_delete(PERSIST_STATUS);
  }
  if (persist_exists(PERSIST_PAUSED)) {
    persist_delete(PERSIST_PAUSED);
  }
  wakeup_cancel_all();
  s_wakeup_id = 1;
  set_background_color();
  set_working_text();
}

static void up_handler (ClickRecognizerRef recognizer, void *context) {
  if (timer_running == false && timer_addition < 6000 && !(timer_status == PAUSED_RESTING || timer_status == PAUSED_WORKING)) {
    timer_addition += 60;
    display_time_text();
  }
}

static void down_handler (ClickRecognizerRef recognizer, void *context) {
  if (timer_running == false && timer_addition > 60 && !(timer_status == PAUSED_RESTING || timer_status == PAUSED_WORKING)) {
    timer_addition -= 60;
    display_time_text();
  }
}

static void multi_click_handler (ClickRecognizerRef recognizer, void *context) {
  wakeup_cancel_all();
  s_wakeup_id = 1;
  timer_running = false;
  if (timer_status == PAUSED_WORKING) {
    timer_status = WORKING;
  }
  else if (timer_status == PAUSED_RESTING) {
    timer_status = RESTING;
  }
  toggle_status();
  if (timer_status == WORKING) {
    set_working_text();
  }
  else if (timer_status == RESTING) {
    set_resting_text();
  }
  set_background_color();
  set_timer_length();
  display_time_text();
}


static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, long_press_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, up_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_handler);
  window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 0, 0, false, multi_click_handler);
}

static void wakeup_handler(WakeupId id, int32_t reason) {
  end_countdown();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  time_text_layer = text_layer_create(GRect(0, 35, bounds.size.w, 90));
  text_layer_set_text_alignment(time_text_layer, GTextAlignmentCenter);
  text_layer_set_font(time_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  layer_add_child(window_layer, text_layer_get_layer(time_text_layer));
  
  status_text_layer = text_layer_create(GRect(0, 95, bounds.size.w, 40));
  text_layer_set_text_alignment(status_text_layer, GTextAlignmentCenter);
  text_layer_set_font(status_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(status_text_layer));
  
  text_layer_set_text(time_text_layer, timer_text);
  if (launch_reason() == APP_LAUNCH_WAKEUP) {
    end_countdown();
  }
  else if (persist_exists(PERSIST_PAUSED)) {
    int countdown_display = time_remaining / 60;
    set_background_color();
    if ((time_remaining % 60) <= 10) {
      snprintf(timer_text, sizeof(timer_text), "%d", countdown_display); 
    }
    else {
      int countdown_display_rounded_up = (time_remaining / 60) + 1;
      snprintf(timer_text, sizeof(timer_text), "%d", countdown_display_rounded_up); // When the tick_timer would register a minute change right after setting the timer, the number on-screen would drop unnaturally quickly. This makes the number displayed drop only when there is 10 secnods or less remaining in the current minute.
    }
    text_layer_set_text(time_text_layer, timer_text);
    layer_mark_dirty(text_layer_get_layer(time_text_layer));
    add_status_text();
  }
  else if (timer_running == false) {
    if (timer_status == WORKING) {
      set_working_text();
    }
    else if (timer_status == RESTING) {
      set_resting_text();
    }
    set_background_color();
  }
  else {
    set_background_color();
    add_status_text();
    display_time_text();
  }
  tick_timer_service_subscribe(MINUTE_UNIT, register_time);
}

static void window_unload(Window *window) {
  text_layer_destroy(time_text_layer);
}

static void init(void) {
  set_defaults();
  if (persist_exists(PERSIST_WAKEUP)) {
    s_wakeup_id = persist_read_int(PERSIST_WAKEUP);
    timer_running = true;
  }
  else if (persist_exists(PERSIST_PAUSED)) {
    time_remaining = persist_read_int(PERSIST_PAUSED);
  }
  if (persist_exists(PERSIST_STATUS)) {
    timer_status = persist_read_int(PERSIST_STATUS);
    if (timer_running == false && !(timer_status == PAUSED_WORKING || timer_status == PAUSED_RESTING)) {
      set_timer_length();
    }
  }
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  const bool animated = true;
  window_stack_push(window, animated);
  wakeup_service_subscribe(wakeup_handler);
}

static void deinit(void) {
  persist_write_int(PERSIST_STATUS, timer_status);
  if (wakeup_query(s_wakeup_id, &s_wakeup_timestamp)) {
    persist_write_int(PERSIST_WAKEUP, s_wakeup_id);
  }
  else {
    persist_delete(PERSIST_WAKEUP);
  }
  if (timer_status == PAUSED_RESTING || timer_status == PAUSED_WORKING) {
    persist_write_int(PERSIST_PAUSED, time_remaining);
  }
  else {
    persist_delete(PERSIST_PAUSED);
  }
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}