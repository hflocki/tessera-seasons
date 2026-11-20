> in testing and not working


# Seasons

A face for the screensaver's clock: the time large in a night sky of the season, with small things drifting across
the glass.

- **Winter**: a snowy night with a cabin, pine trees and a snowman, and falling snowflakes.
- **Spring**: a tree in blossom and tulips under a crescent moon, and petals that blow by.
- **Summer**: a lighthouse by the sea in the afterglow, and fireflies that wander and blink.
- **Autumn**: glowing pumpkins under an orange moon, and leaves that tumble down.

Under the time stand the date and the outside temperature, and under them the row of entities you chose for the
clock. The scenes are dark on purpose, so the glass gives little light at night.

## Set up

1. In Tessera, open **Plugins**, **Add with a link**, and paste the link of this repository. Add the plugin to a
   screen; the screen builds once.
2. Open the screen's **Settings** and the **Screensaver** card. Turn on **Show in standby** (the screen needs **Auto
   standby** on too).
3. Open the **Clock** step and choose **Seasons** under **Face**.
4. Optional, in its options:
   - **Season**: **Automatic** (the default) follows the calendar; or pick one season to keep all year.
   - **Seasons start**: **Astronomical** (20 March, 21 June, 22 September, 21 December) or **Meteorological** (the
     first of March, June, September and December).
   - **Dates of your own**: a start for any season, such as `1.11.` for winter from the first of November. An empty
     field keeps the start above.
   - **Southern hemisphere**: summer around Christmas.
   - **Drifting**: off, a few or many.
   - **Picture**: off for the clock and the drifting on black.
   - **Darker at night**: a veil over the picture while the screen's night levels hold (on by default).

Options change without a build.

## Good to know

- The season changes at midnight on the screen's own calendar.
- The drifting moves once a second: a screensaver has no smooth animation, so it never keeps the screen busy.
- Boards without pictures (the CYD, the 3.5-inch Waveshare and the 4-inch Hosyond) show the clock and the drifting
  on the black glass.
- The Waveshare 4.3, 5 and 7-inch never go into standby, so they show no screensaver at all.
- The pictures are in `motifs/`, drawn by `motifs/draw.py`.

## Privacy

With the picture on, the Tessera app in Home Assistant fetches the season's picture from this repository on GitHub
(`raw.githubusercontent.com`) and sends it to the screen; GitHub sees the address of your Home Assistant, as any
visitor. Nothing else leaves your home: the season is worked out on the screen.

## A hobby project

This plugin is a private, non-commercial hobby project. I build and maintain it in my spare time and share it
free of charge as open source under the [MIT License](LICENSE). It is free and will stay free: there is no paid
version, no paid feature and no paid support.

Bug reports and ideas are welcome, but please keep in mind:

- There is no support and no guaranteed response time. I answer when I find the time, which can take days or weeks.
- Not every request gets built. I add what fits the plugin and what I can keep working over time.
- Pull requests are welcome; for a larger change, open an issue first so we can agree on the way.

### No warranty, no liability

The plugin is provided "as is", without any warranty and without guaranteed support, updates or fixes. You use it at
your own risk. To the extent permitted by law, I accept no liability for any damage or data loss resulting from its
use. The full terms are in the [MIT License](LICENSE).

### Buy me a coffee

If the plugin is useful to you and you would like to say thanks, you are welcome to buy me a coffee. A donation is
entirely voluntary: it is a thank-you, not a payment. It does not buy support, features, a response time or any other
service, and the plugin stays exactly the same for everyone, whether they donate or not.

[![Buy me a coffee](https://img.buymeacoffee.com/button-api/?text=Buy%20me%20a%20coffee&emoji=&slug=hflocki&button_colour=FFDD00&font_colour=000000&font_family=Cookie&outline_colour=000000&coffee_colour=ffffff)](https://www.buymeacoffee.com/hflocki)

☕ [buymeacoffee.com/hflocki](https://www.buymeacoffee.com/hflocki)
