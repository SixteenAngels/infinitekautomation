# Register the command 'Antiburn'
# Module loaded in memory only when the command is first used
# rm Antiburn.tapp; zip Antiburn.tapp -j -0 antiburn/*

var wd = Infinitek.wd

lv.antiburn = def()
    import sys
    var path = sys.path()
    path.push(wd)
    import antiburn
    path.pop()
    antiburn.start()
end

Infinitek.add_cmd("Antiburn",
    def ()
        lv.antiburn()
        Infinitek.resp_cmnd_done()
    end
)
