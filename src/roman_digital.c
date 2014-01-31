#include "pebble.h"

static struct {
  Window *window;
  struct tm time;
  int32_t config;
} g;

#define APPMESSAGE_UPDATE_CONFIG 0xCC
#define CONFIG_PERSIST_KEY 0xC0
#define CONFIG_SHOW_SECONDS 1
#define CONFIG_DATE_MD 2

// these things used to be constants
int SEP_HEIGHT;
TimeUnits TICK_UNIT;
GRect DATE_RECT, HOUR_RECT, MINUTE_RECT, SECOND_RECT;

void draw_l(GContext *ctx, GRect rect) {
  int16_t w = rect.size.w / 4.f;
  graphics_fill_rect(ctx, GRect(rect.origin.x,rect.origin.y,w,rect.size.h), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(rect.origin.x,rect.origin.y+rect.size.h-w,rect.size.w,w), 0, GCornerNone);
}

void draw_x(GContext *ctx, GRect rect) {
  int16_t w1 = rect.size.w / 4;
  int16_t w2 = rect.size.w / 4;
  APP_LOG(255, "draw_x %d,%d,%d*%d (%d,%d)", rect.origin.x, rect.origin.y, rect.size.w, rect.size.h, w1, w2);
  GPath path1 = {
    .num_points = 4,
    .points = (GPoint[]) { {0, 0}, {rect.size.w - w1, rect.size.h-1}, {rect.size.w, rect.size.h-1}, {w1, 0}},
    .rotation = 0,
    .offset = rect.origin
  };
  GPath path2 = {
    .num_points = 4,
    .points = (GPoint[]) { {0, rect.size.h-1}, {rect.size.w - w1, 0}, {rect.size.w, 0}, {w2, rect.size.h-1}},
    .rotation = 0,
    .offset = rect.origin
  };
  gpath_draw_filled(ctx, &path1);
  gpath_draw_filled(ctx, &path2);
}

void draw_v(GContext *ctx, GRect rect) {
  APP_LOG(255, "draw_v %d,%d,%d*%d", rect.origin.x, rect.origin.y, rect.size.w, rect.size.h);
  int16_t w1 = rect.size.w / 4.f;
  int16_t w2 = rect.size.w / 4.f;
  int16_t mp1 = (rect.size.w + w1) / 2;
  GPath path1 = {
    .num_points = 4,
    .points = (GPoint[]) { {0, 0}, {mp1 - w1, rect.size.h-1}, {mp1, rect.size.h-1}, {w1, 0}},
    .rotation = 0,
    .offset = rect.origin
  };
  GPath path2 = {
    .num_points = 4,
    .points = (GPoint[]) { {mp1 - w2, rect.size.h-1}, {rect.size.w - w2, 0}, {rect.size.w, 0}, {mp1, rect.size.h-1}},
    .rotation = 0,
    .offset = rect.origin
  };
  gpath_draw_filled(ctx, &path1);
  gpath_draw_filled(ctx, &path2);
}

void draw_i(GContext *ctx, GRect rect) {
  graphics_fill_rect(ctx, rect, 0, GCornerNone);
}

void to_roman(uint8_t i, char *c) {
  i %= 100;
  char text[2][4] = {"CLX", "XVI"};
  uint8_t units[2] = {i/10, i%10};
  *c = '\0';
  int u;
  for(u=0; u < 2; u++) {
    switch(units[u]) {
      case 9: *c++ = text[u][2]; *c++ = text[u][0]; break;
      case 8: *c++ = text[u][1]; *c++ = text[u][2]; *c++ = text[u][2]; *c++ = text[u][2]; break;
      case 7: *c++ = text[u][1]; *c++ = text[u][2]; *c++ = text[u][2]; break;
      case 6: *c++ = text[u][1]; *c++ = text[u][2]; break;
      case 4: *c++ = text[u][2];
      case 5: *c++ = text[u][1]; break;
      case 3: *c++ = text[u][2];
      case 2: *c++ = text[u][2];
      case 1: *c++ = text[u][2];
    }
  }
  *c = '\0';
}

int16_t get_char_widths(char *str, int16_t *widths, GSize boxSize, int16_t sepWidth) {
  int i;
  int16_t xWidth = boxSize.h / 1.35f;
  int16_t iWidth = xWidth / 4.f;
  int16_t totalSepWidth = sepWidth * (strlen(str) - 1);
  
  // get max width
  int16_t w = 0;
  for(i = 0; str[i]; i++) {
    int16_t charWidth = str[i] == 'I' ? iWidth : xWidth; // width for character I or X,L,V
    w += charWidth;
    widths[i] = charWidth;
  }
  w += totalSepWidth;
  
  if (w <= boxSize.w) return w;
  
  // scale
  float scale = (boxSize.w - totalSepWidth) / (float)(w - totalSepWidth);
  for(i = 0; str[i]; i++) {
    widths[i] *= scale;
  }
  
  return ((w - totalSepWidth) * scale) + totalSepWidth;
}

void draw_roman_number(int8_t n, GContext *ctx, GRect rect, int align) {
  char str[8];
  int16_t charWidth[8];
  int16_t sepWidth = rect.size.h > 40 ? 4 : 2;
  
  // convert to roman
  to_roman(n, str);
  
  // get widths
  int16_t maxWidth = get_char_widths(str, charWidth, rect.size, sepWidth);
  
  // draw chracters
  int16_t start = rect.origin.x;
  if (align == 0) start += (rect.size.w - maxWidth) / 2;
  if (align == 1) start += (rect.size.w - maxWidth);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  for(int i=0; str[i]; i++) {
    switch(str[i]) {
      case 'L': draw_l(ctx, GRect(start, rect.origin.y, charWidth[i], rect.size.h)); break;
      case 'X': draw_x(ctx, GRect(start, rect.origin.y, charWidth[i], rect.size.h)); break;
      case 'V': draw_v(ctx, GRect(start, rect.origin.y, charWidth[i], rect.size.h)); break;
      case 'I': draw_i(ctx, GRect(start, rect.origin.y, charWidth[i], rect.size.h));
    }
    start += charWidth[i] + sepWidth;
  }
}

void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  
  // date
  bounds = DATE_RECT;
  graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, SEP_HEIGHT), 0, GCornerNone);
  GRect rect = GRect(bounds.origin.x + 4, bounds.origin.y + (SEP_HEIGHT-1), (bounds.size.w - 8)/2, bounds.size.h - (SEP_HEIGHT*2) + 1);
  draw_roman_number(g.config & CONFIG_DATE_MD ? g.time.tm_mon + 1 : g.time.tm_mday, ctx, rect, -1);
  rect.origin.x += (bounds.size.w - 8)/2;
  draw_roman_number(g.config & CONFIG_DATE_MD ? g.time.tm_mday : g.time.tm_mon + 1, ctx, rect, 1);
  
  // hour
  bounds = HOUR_RECT;
  graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.origin.y, bounds.size.w, SEP_HEIGHT), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.origin.y + (bounds.size.h - SEP_HEIGHT), bounds.size.w, SEP_HEIGHT), 0, GCornerNone);
  int8_t hour = g.time.tm_hour;
  if (!clock_is_24h_style()) hour %= 12;
  if (hour == 0) hour = clock_is_24h_style() ? 24 : 12;
  draw_roman_number(hour, ctx, GRect(bounds.origin.x + 8, bounds.origin.y + (SEP_HEIGHT-1), bounds.size.w - 16, bounds.size.h - (SEP_HEIGHT*2) + 1), 0);
  
  // minute
  bounds = MINUTE_RECT;
  graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.origin.y + (bounds.size.h - SEP_HEIGHT), bounds.size.w, SEP_HEIGHT), 0, GCornerNone);
  draw_roman_number(g.time.tm_min, ctx, GRect(bounds.origin.x, bounds.origin.y + (SEP_HEIGHT-1), bounds.size.w, bounds.size.h - (SEP_HEIGHT*2) + 1), 0);
  
  // second
  if (g.config & CONFIG_SHOW_SECONDS) {
    bounds = SECOND_RECT;
    graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.origin.y + (bounds.size.h - SEP_HEIGHT), bounds.size.w, SEP_HEIGHT), 0, GCornerNone);
    draw_roman_number(g.time.tm_sec, ctx, GRect(bounds.origin.x, bounds.origin.y + (SEP_HEIGHT-1), bounds.size.w, bounds.size.h - (SEP_HEIGHT*2) + 1), 0);
  }
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (tick_time == NULL) {
    time_t tm = time(NULL);
    g.time = *(localtime(&tm));
    return;
  }
  g.time = *tick_time;
  
  layer_mark_dirty(window_get_root_layer(g.window));
}

void configure() {
  // load configuration
  if (persist_exists(CONFIG_PERSIST_KEY)) {
    g.config = persist_read_int(CONFIG_PERSIST_KEY);
  } else {
    g.config = 0;
  }
  
  // set things
  if (g.config & CONFIG_SHOW_SECONDS) {
    SEP_HEIGHT = 5;
    TICK_UNIT = SECOND_UNIT;
    DATE_RECT = GRect(4, 4, 136, 34);
    HOUR_RECT = GRect(4, 33, 136, 83);
    MINUTE_RECT = GRect(4, 111, 136, 29);
    SECOND_RECT = GRect(4, 135, 136, 29);
  } else {
    SEP_HEIGHT = 7;
    TICK_UNIT = MINUTE_UNIT;
    DATE_RECT = GRect(4, 8, 136, 40);
    HOUR_RECT = GRect(4, 41, 136, 91);
    MINUTE_RECT = GRect(4, 125, 136, 35);
  }
  
  tick_timer_service_unsubscribe();
  tick_timer_service_subscribe(TICK_UNIT, handle_tick);
}

void app_message_rcv(DictionaryIterator *iter, void *context) {
  Tuple *config_tuple = dict_find(iter, APPMESSAGE_UPDATE_CONFIG);
  
  if (config_tuple) {
    persist_write_int(CONFIG_PERSIST_KEY, config_tuple->value->uint8);
    configure();
  }
}

void init() {
  // load settings
  configure();
  
  // init app message
  app_message_register_inbox_received(app_message_rcv);
  app_message_open(64, 64);
  
  // create window
  g.window = window_create();
  window_set_background_color(g.window, GColorBlack);
  Layer *rootLayer = window_get_root_layer(g.window);
  layer_set_update_proc(rootLayer, update_proc);
  window_stack_push(g.window, true);
  
  // initial tick notification
  handle_tick(NULL, TICK_UNIT);
}

void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(g.window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
