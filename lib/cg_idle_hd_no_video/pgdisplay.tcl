proc run { p } {
    variable nextevent

    variable pid
    
    frame $p.pgdisplay -width [winfo reqwidth $p] -height [winfo reqheight $p]
    place $p.pgdisplay -relx 0.0 -rely 0.0
    
    set pid [exec /usr/local/bin/wish8.6 /usr/photon/bin/pgdisplay \
        -use [winfo id $p.pgdisplay] -- -h &]
  
    # wait 1 minute
    set nextevent [ after 120000 "::dm_next_IDLE $p" ]
}

proc end { p } {
    variable nextevent
    variable pid
    catch [ list after cancel $nextevent ]
    catch { kill $pid }
    destroy $p
    return -code ok
}

