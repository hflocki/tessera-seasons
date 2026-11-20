#include "seasons.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace esphome::seasons {

using tessera::Font;
namespace ui = tessera::ui;

// A face of the screensaver's clock (plugin API 0.14): a night scene of the season over the whole glass, the time large
// in the sky with the date, the temperature and the clock's row of entities under it, and small things that drift
// across the glass: snowflakes in winter, petals in spring, fireflies in summer, leaves in autumn.
//
// The season follows the screen's own calendar (astronomical or meteorological starts, or dates of the person's own,
// and the southern half of the world turned round), or the person picks one. The scene is a picture of motifs/ in
// this repository, which the app fetches; a screen without pictures shows the clock and the drift on the black glass.
//
// A saver has no animation of its own: the drift moves once a second in on_tick, a few small blocks at a time, so the
// glass only draws those small boxes again. The screen moves the whole saver a millimetre every minute against burn-in.

static const char *const PICTURES = "https://raw.githubusercontent.com/hflocki/tessera-seasons/main/motifs/";

enum Season : uint8_t { WINTER, SPRING, SUMMER, AUTUMN };
static const char *const NAMES[] = {"winter", "spring", "summer", "autumn"};
// When each season starts, as month * 100 + day, in the order of Season.
static const int ASTRONOMICAL[] = {1221, 320, 621, 922};
static const int METEOROLOGICAL[] = {1201, 301, 601, 901};

static constexpr int MAX_DRIFT = 16;
static constexpr int MAX_ITEMS = 4;

// A day of the year as month * 100 + day from "21.12.", "21.12" or "12-21"; 0 when it is none.
static int parse_day(const char *text) {
  int a = 0, b = 0, d = 0, m = 0;
  if (sscanf(text, "%d.%d", &a, &b) == 2) d = a, m = b;
  else if (sscanf(text, "%d-%d", &a, &b) == 2) m = a, d = b;
  else return 0;
  return m >= 1 && m <= 12 && d >= 1 && d <= 31 ? m * 100 + d : 0;
}

class SeasonSaver : public tessera::Saver {
 public:
  void show(const tessera::SaverContext &c) override {
    width_ = c.width;
    height_ = c.height;

    // An option the person never touched may not be in the word: read each with its default.
    const char *season = c.options["season"] | "auto";
    fixed_ = -1;
    for (int i = 0; i < 4; i++)
      if (strcmp(season, NAMES[i]) == 0) fixed_ = i;
    const bool meteorological = strcmp(c.options["start"] | "astronomical", "meteorological") == 0;
    static const char *const STARTS[] = {"winter_start", "spring_start", "summer_start", "autumn_start"};
    for (int i = 0; i < 4; i++) {
      const int own = parse_day(c.options[STARTS[i]] | "");
      starts_[i] = own ? own : (meteorological ? METEOROLOGICAL[i] : ASTRONOMICAL[i]);
    }
    south_ = c.options["south"] | false;
    const char *drift = c.options["drift"] | "few";
    count_ = strcmp(drift, "none") == 0 ? 0 : strcmp(drift, "many") == 0 ? MAX_DRIFT : MAX_DRIFT / 2;
    const bool scene = c.options["scene"] | true;
    const bool dim = c.options["dim"] | true;

    // The scene over the whole glass, under everything else.
    if (scene && ui::pictures()) {
      picture_ = ui::picture(c.parent, tessera::Placeholder::NONE);
      lv_obj_set_size(picture_, width_, height_);
      lv_obj_set_pos(picture_, 0, 0);
    }
    // At night a veil over the scene, so the glass gives less light; the words stay bright over it.
    if (picture_ && dim && c.night) {
      lv_obj_t *veil = ui::block(c.parent, theme::PAGE);
      lv_obj_set_style_radius(veil, 0, 0);
      lv_obj_set_style_bg_opa(veil, LV_OPA_40, 0);
      lv_obj_set_size(veil, width_, height_);
      lv_obj_set_pos(veil, 0, 0);
    }

    // The drift, behind the words.
    for (int i = 0; i < count_; i++) drift_[i].part = ui::block(c.parent, theme::INK);

    // The time in the upper part of the sky, the date with the temperature under it, and the clock's row under that.
    const int gap = ui::px(ui::large() ? 10 : 6);
    time_y_ = height_ * (ui::large() ? 10 : 8) / 100;
    time_ = ui::label(c.parent, Font::DIGITS, theme::INK);
    line_y_ = time_y_ + ui::line_height(Font::DIGITS) + gap;
    date_ = ui::label(c.parent, Font::TITLE, theme::INK_SOFT);
    temperature_ = ui::label(c.parent, Font::TITLE, theme::MUTED);
    items_y_ = line_y_ + ui::line_height(Font::TITLE) + gap;
    // Until the time is known, the words' box takes the whole width.
    words_ = {0, time_y_, width_, items_y_};
    for (int i = 0; i < MAX_ITEMS; i++) {
      icons_[i] = ui::label(c.parent, Font::ICON_SMALL, theme::MUTED);
      texts_[i] = ui::label(c.parent, Font::BODY, theme::MUTED);
      lv_obj_add_flag(icons_[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(texts_[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  // The clock step's contract: `temperature` ("21°", "" for none) and `items`, the row of entities the person chose.
  void on_state(JsonObjectConst data) override {
    snprintf(temperature_text_, sizeof temperature_text_, "%s", data["temperature"] | "");
    item_count_ = 0;
    for (JsonObjectConst item : data["items"].as<JsonArrayConst>()) {
      if (item_count_ == MAX_ITEMS) break;
      const uint32_t icon = item["icon"] | 0u;
      snprintf(item_icons_[item_count_], sizeof item_icons_[0], "%s", icon ? ui::icon(icon).c_str() : "");
      snprintf(item_texts_[item_count_], sizeof item_texts_[0], "%s", item["text"] | "");
      if (item_icons_[item_count_][0] || item_texts_[item_count_][0]) item_count_++;
    }
    paint_line();
    paint_items();
  }

  void on_tick(uint32_t epoch) override {
    if (epoch && epoch / 60 != minute_) {
      minute_ = epoch / 60;
      epoch_ = epoch;
      const tessera::LocalTime now = tessera::local_time(epoch);
      // The season, again whenever the day changes.
      const int day = now.month * 100 + now.day;
      if (day != day_) {
        day_ = day;
        set_season(season_for(day));
      }
      const std::string time = tessera::clock_text(epoch);
      // "2:05 PM" has letters the digits' font lacks.
      const Font face = ui::covers(time, Font::DIGITS) ? Font::DIGITS : Font::VALUE;
      ui::set_font(time_, face);
      ui::set_text(time_, time);
      const int tw = ui::text_width(time, face);
      lv_obj_set_width(time_, tw > 0 ? tw : 1);
      lv_obj_set_pos(time_, (width_ - tw) / 2, time_y_);
      time_w_ = tw;
      fit_words();
      paint_line();
    }
    step_drift();
  }

 private:
  struct Flake {
    lv_obj_t *part{};
    int x = 0, y = 0;      // where it stands, kept here: LVGL reads a position as 0 until it lays out
    int fall = 0, wind = 0, size = 0;
    int w = 0, h = 0;      // its size, kept here too
    int phase = 0, angle = 0, spin = 0;
    bool lit = true;
  };

  uint32_t random() {
    seed_ = seed_ * 1664525u + 1013904223u;
    return seed_ >> 8;
  }
  int between(int low, int high) { return high <= low ? low : low + static_cast<int>(random() % (high - low + 1)); }

  // The season whose start is the latest on or before this day; before the first start of the year, the one that
  // started last year.
  Season season_for(int day) const {
    if (fixed_ >= 0) return static_cast<Season>(fixed_);
    int best = -1, latest = -1, last = 0;
    for (int i = 0; i < 4; i++) {
      if (starts_[i] <= day && starts_[i] > latest) latest = starts_[i], best = i;
      if (starts_[i] > starts_[last]) last = i;
    }
    if (best < 0) best = last;
    // South of the equator the seasons are the other way round.
    if (south_) best = (best + 2) % 4;
    return static_cast<Season>(best);
  }

  void set_season(Season season) {
    if (season == season_) return;
    season_ = season;
    if (picture_) ui::set_picture(picture_, std::string(PICTURES) + NAMES[season] + ".png");
    seed_ ^= 0x9e3779b9u * (season + 1);
    for (int i = 0; i < count_; i++) shape(drift_[i], true);
  }

  // A flake of this season: its size, colour and how it moves; `anywhere` places it over the whole glass (when the
  // saver comes, or the season changes), else it comes in again at the top.
  void shape(Flake &f, bool anywhere) {
    lv_obj_t *p = f.part;
    const int h = height_;
    switch (season_) {
      case WINTER:
        f.size = ui::px(between(3, 7));
        f.w = f.h = f.size;
        lv_obj_set_size(p, f.w, f.h);
        lv_obj_set_style_radius(p, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(p, ui::color(theme::INK), 0);
        lv_obj_set_style_bg_opa(p, between(0, 1) ? LV_OPA_COVER : LV_OPA_70, 0);
        f.fall = between(h / 40, h / 22);
        f.wind = ui::px(between(2, 6));
        f.spin = 0;
        break;
      case SPRING:
        f.size = ui::px(between(6, 9));
        f.w = f.size;
        f.h = f.size * 3 / 5;
        lv_obj_set_size(p, f.w, f.h);
        lv_obj_set_style_radius(p, f.size / 2, 0);
        lv_obj_set_style_bg_color(p, ui::card_color(between(0, 3) ? tessera::CardColor::PINK
                                                                   : tessera::CardColor::PURPLE), 0);
        lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
        f.fall = between(h / 60, h / 35);
        f.wind = ui::px(between(3, 9));
        f.spin = between(15, 40);
        break;
      case SUMMER:
        f.size = ui::px(between(4, 6));
        f.w = f.h = f.size;
        lv_obj_set_size(p, f.w, f.h);
        lv_obj_set_style_radius(p, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(p, ui::card_color(tessera::CardColor::YELLOW), 0);
        f.lit = between(0, 1);
        lv_obj_set_style_bg_opa(p, f.lit ? LV_OPA_COVER : LV_OPA_20, 0);
        f.fall = 0;
        f.wind = ui::px(8);
        f.spin = 0;
        break;
      case AUTUMN: {
        f.size = ui::px(between(9, 13));
        f.w = f.size;
        f.h = f.size * 3 / 5;
        lv_obj_set_size(p, f.w, f.h);
        lv_obj_set_style_radius(p, f.size / 3, 0);
        static const tessera::CardColor LEAVES[] = {tessera::CardColor::ORANGE, tessera::CardColor::RED,
                                                    tessera::CardColor::YELLOW, tessera::CardColor::ORANGE};
        lv_obj_set_style_bg_color(p, ui::card_color(LEAVES[between(0, 3)]), 0);
        lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
        f.fall = between(h / 40, h / 25);
        f.wind = ui::px(between(4, 12));
        f.spin = between(25, 60);
        break;
      }
    }
    f.phase = between(0, 359);
    f.angle = between(0, 359);
    f.x = between(0, width_ - f.size);
    if (season_ == SUMMER) f.y = between(h * 40 / 100, h * 92 / 100);
    else f.y = anywhere ? between(-f.size, h) : -f.size - between(0, h / 6);
    if (f.spin) ui::rotate(p, static_cast<float>(f.angle));
    place(f);
  }

  // One step a second: everything falls a little and sways, fireflies wander and blink.
  void step_drift() {
    if (season_ > AUTUMN) return;  // no season yet: the clock is not set
    for (int i = 0; i < count_; i++) {
      Flake &f = drift_[i];
      f.phase = (f.phase + 35) % 360;
      if (season_ == SUMMER) {
        f.x += between(-f.wind, f.wind);
        f.y += between(-f.wind, f.wind);
        if (f.x < 0) f.x = 0;
        if (f.x > width_ - f.size) f.x = width_ - f.size;
        if (f.y < height_ * 35 / 100) f.y = height_ * 35 / 100;
        if (f.y > height_ - f.size) f.y = height_ - f.size;
        if (between(0, 9) < 3) {
          f.lit = !f.lit;
          lv_obj_set_style_bg_opa(f.part, f.lit ? LV_OPA_COVER : LV_OPA_20, 0);
        }
      } else {
        f.y += f.fall;
        // Leaves and petals blow to the side; snow only sways.
        const int sway = static_cast<int>(f.wind * sinf(f.phase * 3.14159265f / 180.0f));
        f.x += sway + (season_ == WINTER ? 0 : f.wind / 2);
        if (f.x > width_) f.x = -f.size;
        if (f.x < -f.size) f.x = width_;
        if (f.y > height_) {
          shape(f, false);
          continue;
        }
        if (f.spin) {
          f.angle = (f.angle + f.spin) % 360;
          ui::rotate(f.part, static_cast<float>(f.angle));
        }
      }
      place(f);
    }
  }

  // A flake shows only while it lies whole on the glass and clear of the words: it comes in at the top edge, leaves at
  // the bottom and passes behind the clock, as if behind it.
  void place(Flake &f) {
    lv_obj_set_pos(f.part, f.x, f.y);
    const int w = f.w, h = f.h;
    const int m = ui::px(4);
    const bool on_glass = f.x >= 0 && f.y >= 0 && f.x + w <= width_ && f.y + h <= height_;
    const bool clear = f.x + w + m < words_.x1 || f.x - m > words_.x2 || f.y + h + m < words_.y1 || f.y - m > words_.y2;
    if (on_glass && clear) lv_obj_remove_flag(f.part, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(f.part, LV_OBJ_FLAG_HIDDEN);
  }

  // The date and the temperature, side by side in the middle under the time.
  void paint_line() {
    const std::string date = epoch_ ? tessera::date_text(epoch_) : "";
    const std::string degrees = temperature_text_;
    const int space = degrees.empty() || date.empty() ? 0 : ui::px(ui::large() ? 16 : 10);
    const int dw = ui::text_width(date, Font::TITLE);
    const int tw = degrees.empty() ? 0 : ui::text_width(degrees, Font::TITLE);
    const int x = (width_ - dw - space - tw) / 2;
    ui::set_text(date_, date);
    lv_obj_set_width(date_, dw > 0 ? dw : 1);
    lv_obj_set_pos(date_, x, line_y_);
    ui::set_text(temperature_, degrees);
    lv_obj_set_width(temperature_, tw > 0 ? tw : 1);
    lv_obj_set_pos(temperature_, x + dw + space, line_y_);
    line_w_ = dw + space + tw;
    fit_words();
  }

  // The clock's row of entities: an icon and its words each, in the middle; what does not fit the glass is left out.
  void paint_items() {
    const int inner = ui::px(4), apart = ui::px(ui::large() ? 18 : 12);
    const int icon_h = ui::line_height(Font::ICON_SMALL), text_h = ui::line_height(Font::BODY);
    const int row_h = icon_h > text_h ? icon_h : text_h;
    int widths[MAX_ITEMS] = {}, total = 0, shown = 0;
    for (int i = 0; i < item_count_; i++) {
      const int iw = item_icons_[i][0] ? ui::text_width(item_icons_[i], Font::ICON_SMALL) : 0;
      const int tw = ui::text_width(item_texts_[i], Font::BODY);
      widths[i] = iw + (iw && tw ? inner : 0) + tw;
      const int more = widths[i] + (shown ? apart : 0);
      if (total + more > width_ - 2 * ui::px(12)) break;
      total += more;
      shown++;
    }
    int x = (width_ - total) / 2;
    for (int i = 0; i < MAX_ITEMS; i++) {
      if (i >= shown) {
        lv_obj_add_flag(icons_[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(texts_[i], LV_OBJ_FLAG_HIDDEN);
        continue;
      }
      const int iw = item_icons_[i][0] ? ui::text_width(item_icons_[i], Font::ICON_SMALL) : 0;
      const int tw = ui::text_width(item_texts_[i], Font::BODY);
      ui::set_text(icons_[i], item_icons_[i]);
      ui::set_text(texts_[i], item_texts_[i]);
      lv_obj_set_width(icons_[i], iw > 0 ? iw : 1);
      lv_obj_set_width(texts_[i], tw > 0 ? tw : 1);
      lv_obj_set_pos(icons_[i], x, items_y_ + (row_h - icon_h) / 2);
      lv_obj_set_pos(texts_[i], x + iw + (iw && tw ? inner : 0), items_y_ + (row_h - text_h) / 2);
      if (iw) lv_obj_remove_flag(icons_[i], LV_OBJ_FLAG_HIDDEN);
      else lv_obj_add_flag(icons_[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_remove_flag(texts_[i], LV_OBJ_FLAG_HIDDEN);
      x += widths[i] + apart;
    }
    items_w_ = total;
    items_h_ = shown ? row_h : 0;
    fit_words();
  }

  // The box of the words: as wide as the widest of the time, the date line and the row, from the time's top to the
  // foot of the last line that shows.
  void fit_words() {
    int widest = time_w_ > line_w_ ? time_w_ : line_w_;
    if (items_w_ > widest) widest = items_w_;
    words_.x1 = (width_ - widest) / 2;
    words_.x2 = words_.x1 + widest;
    words_.y1 = time_y_;
    words_.y2 = items_h_ ? items_y_ + items_h_ : items_y_;
  }

  int width_ = 0, height_ = 0, time_y_ = 0, line_y_ = 0, items_y_ = 0;
  // Where the words stand (the time, the date line and the row of entities); the drift keeps clear of it.
  struct Box { int x1 = 0, y1 = 0, x2 = 0, y2 = 0; } words_;
  int time_w_ = 0, line_w_ = 0, items_w_ = 0, items_h_ = 0;
  int fixed_ = -1, starts_[4] = {}, count_ = 0, day_ = 0;
  bool south_ = false;
  uint8_t season_ = 0xff;
  uint32_t minute_ = 0, epoch_ = 0, seed_ = 0x2545f491u;
  lv_obj_t *picture_{}, *time_{}, *date_{}, *temperature_{};
  lv_obj_t *icons_[MAX_ITEMS]{}, *texts_[MAX_ITEMS]{};
  // Short texts in char arrays, so they stay in the saver's own memory (PSRAM) instead of the chip's heap.
  char temperature_text_[16] = "", item_icons_[MAX_ITEMS][8] = {}, item_texts_[MAX_ITEMS][48] = {};
  int item_count_ = 0;
  Flake drift_[MAX_DRIFT];
};

void Seasons::setup() {
  // The id is the saver's id in the manifest, and its step must be the manifest's (clock).
  add_saver("seasons", tessera::SaverStep::CLOCK, []() { return new SeasonSaver(); });
}

}  // namespace esphome::seasons
