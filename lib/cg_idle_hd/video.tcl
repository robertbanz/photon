
set mplayer_pid 0

set clipdir "/usr/photon/lib/clips/"

proc pick_clip { dir } {
  set clips [ glob ${dir}/*.mp4 ]
  set which [ expr { int(rand() * [llength $clips]) }]
  return [ lindex $clips $which ]
}

proc run { p } {
  variable mplayer_pid
  variable check_done_event
  variable clipdir
  
  canvas $p.video -width [ winfo reqwidth $p ] -height [ winfo reqheight $p ]

  image create photo video_background_image -file \
    ${::Conf::gamepath}/images/cg_idle_hd/video_background.tif
  
  $p.video create image 0 0 -image video_background_image -anchor nw

  # create a widget to place our video in
  label $p.video.clip -height 540 -width 720
  $p.video create window 381 90 -height 540 -width 720 -anchor nw -window $p.video.clip
  
  place $p.video -relx 0.0 -rely 0.0

  set mplayer_pid [fork]

  set window_id [ winfo id $p.video.clip ]

  set clip [ pick_clip $clipdir ]

  if { $mplayer_pid == 0 } {
    execl "mplayer" [ list "-nosound" "-quiet" "-wid" $window_id ${clip} ]
    kill [id process]
  }
  
#  bind $p.video <<Destroy>> "::Idlescreen::end"
  
  set check_done_event [ after 1000 [ list ::Idlescreen::check_done $p ] ]
}

proc check_done { root } {
  variable mplayer_pid
  if { $mplayer_pid != 0 } {
    set result [ wait -nohang $mplayer_pid ]
    if { [ llength $result ] != 0 } {
      set mplayer_pid 0
      ::dm_next_IDLE $root
    } else {
      set check_done_event [ after 1000 [ list ::Idlescreen::check_done $root ] ]
    }
  }
}

proc end { p } {
  variable check_done_event
  variable mplayer_pid
  catch [ list after cancel $check_done_event ]
  catch [ image delete video_background_image ]

  if { $mplayer_pid != 0 } {
    kill $mplayer_pid
    wait $mplayer_pid
  }
  
  destroy $p
}

