


proc run { p } {

    variable advertimage
    variable nextevent

    variable mplayer_pid

    frame $p.video
    $p.video configure -width [ winfo reqwidth $p ] -height [ winfo reqheight $p ]
    
    label $p.video.ad -relief flat
    place $p.video.ad -relx 0.0 -rely 0.0 -width 640 -height 480
    
    place $p.video -relx 0.0 -rely 0.0

    set pid [fork]
    if { $pid == 0 } {
      exec "/usr/local/bin/mplayer" "-nosound" "-wid" [ winfo id $p.video.ad ]  "/usr/photon/git/photon/game/images/video.mp4"
    } else {
      set mplayer_pid $pid
    }

    bind $p.video <<Destroy>> "::Idlescreen::end"

    set nextevent [ after 45000 "::Idlescreen::end ; ::dm_next_IDLE $p" ]

}

proc end { } {

    variable advertimage
    variable nextevent

    variable mplayer_pid
    
    exec kill $mplayer_pid

    catch [ list image delete advertimage ]
    
    catch [ list after cancel $nextevent ]

}

