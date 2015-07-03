
load $Conf::libpath/tclmixer.so

namespace eval ::Mixer {
  mixer m /dev/mixer0
  
  set devs [ m list ]

  foreach x $devs {
    set ::Mixer::$x [ lindex [ split [ lindex [ m get $x ] 0 ] ":" ] 0 ]
    trace variable ::Mixer::$x wr ::Mixer::mixer_watch
  }

  proc mixer_watch { n element op } {
    set name [ namespace tail $n ]
    upvar $n var
    if { "$op" == "w" } {
      m set $name $var      
    } elseif { "$op" == "r" } {
      set var [ lindex [ split [ lindex [ m get $name ] 0 ] ":" ] 0 ]
    }
  }


  proc audio_panel { root devs } {
    foreach x $devs {
        scale $root.$x -from 0 -to 100 -length 200 -variable ::Mixer::$x -orient vertical -label $x -showvalue true
        pack $root.$x -side left
    }
  }

  proc fadestep { device target startval starttime endtime } {
    global sched_data
    set nowtime [ clock milliseconds ]
    set nowtime [ expr { $nowtime / 1000.0 } ]
    set fade_duration [ expr { $endtime - $starttime } ]
    set fade_remaining [ expr { $endtime - $nowtime } ]

    if { $fade_remaining < 0 } {
      set fade_remaining 0
    }

    # what percentage of the way should we be?
    set fade_percentage [ expr { double( $fade_duration - $fade_remaining ) / $fade_duration } ]

    set fade_difference [ expr { $target - $startval } ]
    set desired_value [ expr { ( $fade_difference * $fade_percentage ) + $startval } ]
    
    set ::Mixer::$device [ expr { int($desired_value) } ]

    if { int($desired_value) == $target } {
      return -code ok
    }

    if { $fade_remaining != 0 } {
      set ::Mixer::${device}_schedule [ after 250 [ list ::Mixer::fadestep $device $target $startval $starttime $endtime ] ]
    }
  }

  proc fade { device target seconds } { 
    if { [ info exists ::Mixer::${device}_schedule ] } {
      upvar 0 ::Mixer::${device}_schedule stuff
      after cancel $stuff
    }

    # upvar ::Mixer::$device now 
    set now [ set ::Mixer::$device ]
    set timenow [ clock seconds ]
    ::Mixer::fadestep $device $target $now $timenow [ expr $timenow + $seconds ]
  }
}
