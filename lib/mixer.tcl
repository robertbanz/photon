
package require snack

namespace eval ::Mixer {

variable field_volume 0
variable background_volume 0
variable pacmd_fd -1
variable pacmd "/usr/local/bin/pacmd"

# Init the mixer with the mixer device, lines to control -- e.g.
# /dev/mixer0 Vol Mic
proc init { device field_line background_line } {
  variable pacmd_fd
  variable pacmd

  # Binding the same variable to both left and right channels
  # allows one var to control both.
  snack::mixer volume ${field_line} \
      ::Mixer::field_volume ::Mixer::field_volume

  set m [ menu .menubar.audio -tearoff 0 ]
  .menubar add cascade -label Audio -menu ${m}
  ${m} add command -label "Audio Control Panel" \
      -command [ namespace code { control_panel } ]

  if {[open_pacmd] == -1} {
    puts "Could not open pipe to pacmd."
    return --code error
  }
  
  trace variable ::Mixer::background_volume w background_volume_trace
}

proc open_pacmd {} {
  variable pacmd_fd
  variable pacmd
  catch {close $pacmd}
  if {[catch {open |[list $pacmd] w+} pacmd_fd]} {
    puts "Could not open pipe to pacmd: $pacmd_fd"
    return -1 --code ok
  }
  fconfigure $pacmd_fd -blocking 0 -buffering line
  ::warninglog "(re)opened pipe to pacmd"
  return $pacmd_fd
}

proc background_volume_trace {name args} {
  variable background_volume
  set_background_volume $background_volume 
}

proc set_background_volume { percent } {
  variable pacmd_fd
  set pacmdlevel [expr {65536 * $percent / 100}]
  if {[catch {puts $pacmd_fd "set-sink-input-volume 0 $pacmdlevel"}]} {
    open_pacmd
  }
}

proc control_panel { } {
  if { [ winfo exists .audio ] } {
    return -code ok
  }

  toplevel .audio
  scale .audio.field -from 0 -to 100 -length 200 \
      -variable ::Mixer::field_volume \
      -orient horizontal -label "Field Volume" -showvalue true
  scale .audio.background -from 0 -to 100 -length 200 -variable \
      ::Mixer::background_volume -orient horizontal \
      -label "Background Audio (current)" \
      -showvalue true \
      -command [namespace code {background_volume_trace background_volume 0}]
  scale .audio.backgroundmax -from 0 -to 100 -length 200 \
      -variable Conf::max_background_audio -orient horizontal \
      -label "Background Audio (max)" -showvalue true
  pack .audio.field .audio.background .audio.backgroundmax -side top
}


proc is_valid_device { device } {
  if { $device != "field" && $device != "background" } {
    return 0
  }
  return 1
}

proc fadestep { device target startval starttime endtime } {
  global sched_data
  variable field_volume
  variable background_volume
  
  if { ! [ is_valid_device $device ] } {
    return -code error "Invalid device name: $device"
  }
  
  set nowtime [ clock milliseconds ]
  set nowtime [ expr { $nowtime / 1000.0 } ]
  set fade_duration [ expr { $endtime - $starttime } ]
  set fade_remaining [ expr { $endtime - $nowtime } ]

  if { $fade_remaining < 0 } {
    set fade_remaining 0
  }

  # what percentage of the way should we be?
  set fade_percentage [ expr \
      { double( $fade_duration - $fade_remaining ) / $fade_duration } ]

  set fade_difference [ expr { $target - $startval } ]
  set desired_value [ expr \
      { ( $fade_difference * $fade_percentage ) + $startval } ]
  
  set ::Mixer::${device}_volume [ expr { int($desired_value) } ]

  if { int($desired_value) == $target } {
    return -code ok
  }

  if { $fade_remaining != 0 } {
    set ::Mixer::${device}_schedule [ after 250 [ list \
        ::Mixer::fadestep $device $target $startval $starttime $endtime ] ]
  }
}

proc fade { device target seconds } {
  variable field_volume
  variable background_volume

  if { ! [ is_valid_device $device ] } {
    return -code error "Invalid device name: $device"
  }

  if { [ info exists ::Mixer::${device}_schedule ] } {
    upvar 0 ::Mixer::${device}_schedule stuff
    after cancel $stuff
  }

  set now [ set ::Mixer::${device}_volume ]
  set timenow [ clock seconds ]
  ::Mixer::fadestep $device $target $now $timenow [ expr $timenow + $seconds ]
}

}
# namespace