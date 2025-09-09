#- ------------------------------------------------------------ -#
#  Module `lv_Infinitek` - piggybacks on `lv` to extend it
#- ------------------------------------------------------------ -#
lv_Infinitek = module("lv_Infinitek")

# rename `lv` to `lv_ntv` and replace `lv` with `lv_Infinitek`
#@ solidify:lv_Infinitek.init,weak
def init(lv_Infinitek)
  import lv
  lv.start = lv_Infinitek.start
  lv._constants = lv_Infinitek._constants
  lv.splash_init = lv_Infinitek.splash_init
  lv.splash_remove = lv_Infinitek.splash_remove
  lv.splash = lv_Infinitek.splash
  lv._splash = nil                            # store a reference for the current splash screen parent

  lv.font_montserrat = lv_Infinitek.font_montserrat
  lv.montserrat_font = lv_Infinitek.font_montserrat

  lv.font_seg7 = lv_Infinitek.font_seg7
  lv.seg7_font = lv_Infinitek.font_seg7
  lv.font_embedded = lv_Infinitek.font_embedded

  lv.load_freetype_font = lv_Infinitek.load_freetype_font

  lv.register_button_encoder = lv_Infinitek.register_button_encoder
  lv.screenshot = lv_Infinitek.screenshot
  lv.set_paint_cb = lv_Infinitek.set_paint_cb

  # add version information
  lv.version = lv.version_major()
  # use the following to retrofit the version:
  #-
    def fix_lv_version()
      import introspect
      var v = introspect.get(lv, "version")
      # if `lv.version` does not exist, v is `module('undefined')`
      if type(v) != 'int'  lv.version = 8 end
    end
  -#

  # add widgets
  lv.clock = lv_clock
  lv.clock_icon = lv_clock_icon
  lv.signal_arcs = lv_signal_arcs
  lv.signal_bars = lv_signal_bars
  lv.wifi_arcs_icon = lv_wifi_arcs_icon
  lv.wifi_arcs = lv_wifi_arcs
  lv.wifi_bars_icon = lv_wifi_bars_icon
  lv.wifi_bars = lv_wifi_bars

  # display splash screen unless `SetOption135 1` is set
  if Infinitek.get_option(135) == 0
    lv.splash_init()
  end

  return nil
end
lv_Infinitek.init = init

# run splash now or schedlue later
#@ solidify:lv_Infinitek.splash_init,weak
def splash_init()
  import display
  if display.started()
    lv.splash()                     # if display is ready, just do splash now
  else
    # register a driver until display starts

    # create a class for deferred run
    class splash_runner
      def init()
        Infinitek.add_driver(self)    # register driver
      end

      def display(cmd, idx, payload, raw)
        import display
        if cmd == "dim" && display.started()
          Infinitek.remove_driver(self)
          lv.splash()
        end
      end
    end

    splash_runner()     # create an instance, it auto-registers
  end
end
lv_Infinitek.splash_init = splash_init

# remove splash
#@ solidify:lv_Infinitek.splash_remove,weak
def splash_remove()
  var _splash = lv._splash
  if _splash
    lv._splash = nil
    _splash.del()
  end
end
lv_Infinitek.splash_remove = splash_remove

#@ solidify:lv_Infinitek.splash,weak
def splash()
  import display

  if !display.started() return end

  lv.start()                        # just in case it was not already started

  var bg = lv.obj(lv.scr_act())     # create a parent object for splash screen
  var f28 = lv.montserrat_font(28)  # load embedded Montserrat 28
  var f20 = lv.montserrat_font(20)  # load embedded Montserrat 20
  var white = lv.color(lv.COLOR_WHITE)

  bg.set_style_bg_color(lv.color(0x000066), 0) # lv.PART_MAIN | lv.STATE_DEFAULT
  bg.set_style_radius(0, 0)
  bg.set_style_pad_all(0, 0)
  bg.set_style_border_width(0, 0)
  bg.set_size(lv.pct(100), lv.pct(100))
  bg.refr_pos()
  bg.refr_size()
  # 0x53706C68 = 'Splh' indicating the screen is Splash screen
  bg.set_user_data(0x53706C68)

  var tas_logo = lv.img(bg)
  tas_logo.set_Infinitek_logo36()
  tas_logo.align(lv.ALIGN_LEFT_MID, 0, 0)

  var tas = lv.label(bg)
  # tas.set_style_bg_opa(lv.OPA_TRANSP, lv.PART_MAIN | lv.STATE_DEFAULT)
  tas.set_style_text_color(white, 0)          # lv.PART_MAIN | lv.STATE_DEFAULT
  tas.set_text("Infinitek")
  if lv.get_hor_res() >= 200
    if f28 != nil tas.set_style_text_font(f28, 0) end
  else
    if f20 != nil tas.set_style_text_font(f20, 0) end
  end
  tas.set_align(lv.ALIGN_LEFT_MID)
  tas.set_x(42)

  var driver_name = display.driver_name()
  var disp
  if size(driver_name) > 0
    disp = lv.label(bg)
    disp.set_align(lv.ALIGN_BOTTOM_MID)
    # disp.set_style_bg_opa(lv.OPA_TRANSP, lv.PART_MAIN | lv.STATE_DEFAULT)
    disp.set_style_text_color(lv.color(0xFFFFFF), 0)    # lv.PART_MAIN | lv.STATE_DEFAULT
    disp.set_text(driver_name)
  end

  # force full refresh now and not at next tick
  lv.refr_now(0)

  lv._splash = bg                          # keep a reference to the splash screen

  Infinitek.set_timer(5000, lv.splash_remove)    # delete the object in the future
end
lv_Infinitek.splash = splash

return lv_Infinitek
