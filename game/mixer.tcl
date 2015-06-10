
load $Conf::libpath/tclmixer.so

namespace eval ::Mixer {

    
    mixer m /dev/mixer1
    
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

    proc fade { device target seconds } {	

	if { [ info exists ::Mixer::${device}_schedule ] } {
	    upvar ::Mixer::${device}_schedule v
	    catch [ list  sched_delete $v ]
	    upvar ::Mixer::${device}_event e
	    catch [ list  eh delete $e ]
	    
	}

#	upvar ::Mixer::$device now 
	set now [ set ::Mixer::$device ]
	set ::Mixer::${device}_float $now
	set stepsize [ expr (( $target - $now ) / ( $seconds * 4.00 )) ]
#	puts "stepsize for $device is $stepsize"

	set ::Mixer::${device}_schedule [ sched_add forever [ clock seconds ] .25 [ set ::Mixer::${device}_event [ eh add "::Mixer::fadestep $device $target $stepsize" ] ] ]
	
    }
#    Mixer::fadestep vol 0 -3
    proc fadestep { device  target stepsize } {
	global sched_data
	upvar ::Mixer::${device}_float float
#	puts "$device $stepsize $target"

	set now [ set ::Mixer::$device ]
	set float [ expr $float + $stepsize ]
	set next [ expr int($float) ]

#	puts "$device $float"

#	puts "$now $next"
	# outer bounds
	if { $stepsize < 0 } {
	    if { $next < $target } {
		set next $target
	    }
	} elseif { $stepsize > 0 } {
	    if { $next > $target } { 
		set next $target
	    }
	}
	
#	puts $next 
	# kill ourselves if we make our target
	if { $next == $target } {
#	    puts "done fade $device $target"
	    catch [ list sched_delete $sched_data(schid) ]
	    unset ::Mixer::${device}_schedule
	    global ekill
	    lappend ekill $sched_data(id)
	}

	set ::Mixer::$device $next

    }
	    

}

    