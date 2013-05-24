#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xAC, 0xB9, 0x74, 0x69, 0xE8, 0x88, 0x49, 0x15, 0x9E, 0xC6, 0x2B, 0xC6, 0x0F, 0x58, 0x6F, 0x5C }
PBL_APP_INFO(
  MY_UUID,
  "Roman Digital", "namedfork.net",
  1, 0, /* App version */
  RESOURCE_ID_IMAGE_MENU_ICON,
  APP_INFO_WATCH_FACE);

static struct {
  Window window;
  struct { 
    Layer bg, date, hour, minute, second;
    } layers;
  PblTm time;
} g;


void draw_l(GContext *ctx, GRect rect) {
  int16_t w = rect.size.w / 4.f;
  graphics_fill_rect(ctx, GRect(rect.origin.x,rect.origin.y,w,rect.size.h), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(rect.origin.x,rect.origin.y+rect.size.h-w,rect.size.w,rect.size.h), 0, GCornerNone);
}

void draw_x(GContext *ctx, GRect rect) {
  int16_t w1 = rect.size.w / 4.f;
  int16_t w2 = rect.size.w / 4.f;
  GPath path1 = {
    .num_points = 4,
    .points = (GPoint[]) { {0, 0}, {rect.size.w - w1, rect.size.h}, {rect.size.w, rect.size.h}, {w1, 0}},
    .rotation = 0,
    .offset = rect.origin
  };
  GPath path2 = {
    .num_points = 4,
    .points = (GPoint[]) { {0, rect.size.h}, {rect.size.w - w1, 0}, {rect.size.w, 0}, {w2, rect.size.h}},
    .rotation = 0,
    .offset = rect.origin
  };
  gpath_draw_filled(ctx, &path1);
  gpath_draw_filled(ctx, &path2);
}

void draw_v(GContext *ctx, GRect rect) {
  int16_t w1 = rect.size.w / 4.f;
  int16_t w2 = rect.size.w / 4.f;
  int16_t mp1 = (rect.size.w + w1) / 2;
  GPath path1 = {
    .num_points = 4,
    .points = (GPoint[]) { {0, 0}, {mp1 - w1, rect.size.h}, {mp1, rect.size.h}, {w1, 0}},
    .rotation = 0,
    .offset = rect.origin
  };
  GPath path2 = {
    .num_points = 4,
    .points = (GPoint[]) { {mp1 - w2, rect.size.h}, {rect.size.w - w2, 0}, {rect.size.w, 0}, {mp1, rect.size.h}},
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

void bg_update_proc(Layer *me, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, me->bounds, 0, GCornerNone);
}

void date_update_proc(Layer *me, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, me->bounds.size.w, 5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0, me->bounds.size.h - 5, me->bounds.size.w, me->bounds.size.h), 0, GCornerNone);
  
  GRect rect = GRect(4, 4, (me->bounds.size.w - 8)/2, me->bounds.size.h - 9);
  draw_roman_number(g.time.tm_mon + 1, ctx, rect, -1);
  rect.origin.x += (me->bounds.size.w - 8)/2;
  draw_roman_number(g.time.tm_mday, ctx, rect, 1);
}

void hour_update_proc(Layer *me, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, me->bounds.size.w, 5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0, me->bounds.size.h - 5, me->bounds.size.w, me->bounds.size.h), 0, GCornerNone);
  
  int8_t hour = g.time.tm_hour;
  if (!clock_is_24h_style()) {
    hour %= 12;
  }
  if (hour == 0) hour = clock_is_24h_style() ? 24 : 12;
  draw_roman_number(hour, ctx, GRect(8, 4, me->bounds.size.w - 16, me->bounds.size.h - 9), 0);
}

void minute_update_proc(Layer *me, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, me->bounds.size.w, 5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0, me->bounds.size.h - 5, me->bounds.size.w, me->bounds.size.h), 0, GCornerNone);
  
  draw_roman_number(g.time.tm_min, ctx, GRect(0, 4, me->bounds.size.w, me->bounds.size.h - 9), 0);
}

void second_update_proc(Layer *me, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, me->bounds.size.w, 5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0, me->bounds.size.h - 5, me->bounds.size.w, me->bounds.size.h), 0, GCornerNone);
  
  draw_roman_number(g.time.tm_sec, ctx, GRect(0, 4, me->bounds.size.w, me->bounds.size.h - 9), 0);
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&g.window, "Horologium Digitale");
  
  // init layers
  layer_init(&g.layers.bg, g.window.layer.frame);
  g.layers.bg.update_proc = &bg_update_proc;
  layer_add_child(&g.window.layer, &g.layers.bg);
  
  layer_init(&g.layers.date, GRect(4, 4, 136, 34));
  g.layers.date.update_proc = &date_update_proc;
  layer_add_child(&g.layers.bg, &g.layers.date);
  
  layer_init(&g.layers.hour, GRect(4, 33, 136, 83));
  g.layers.hour.update_proc = &hour_update_proc;
  layer_add_child(&g.layers.bg, &g.layers.hour);
  
  layer_init(&g.layers.minute, GRect(4, 111, 136, 29));
  g.layers.minute.update_proc = &minute_update_proc;
  layer_add_child(&g.layers.bg, &g.layers.minute);
  
  layer_init(&g.layers.second, GRect(4, 135, 136, 29));
  g.layers.second.update_proc = &second_update_proc;
  layer_add_child(&g.layers.bg, &g.layers.second);
  
  get_time(&g.time);
  window_stack_push(&g.window, true /* Animated */);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)t;
  get_time(&g.time);
  layer_mark_dirty(&g.layers.bg);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
