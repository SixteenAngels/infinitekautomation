# register the command 'DisplayCalibrate'
#
# load the module in memory only when the command is first used
#
var wd = Infinitek.wd                 # capture value of the first run
Infinitek.add_cmd("DisplayCalibrate",
  def ()
    import sys
    var path = sys.path()

    path.push(wd)
    import ts_calibrate
    path.pop()

    Infinitek.set_timer(0, /-> ts_calibrate.start())
    Infinitek.resp_cmnd_done()
  end
)
