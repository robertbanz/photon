
proc run { p } {

    variable advertimage
    variable nextevent

    frame $p.advert
    $p.advert configure -width [ winfo reqwidth $p ] -height [ winfo reqheight $p ]
    label $p.advert.ad -relief flat
    place $p.advert.ad -relx 0.0 -rely 0.0 -relwidth 1.0 -relheight 1.0
    place $p.advert -relx 0.0 -rely 0.0
    
    set advert_width [ winfo reqwidth $p.advert ]
    set advert_height [ winfo reqheight $p.advert ]

    image create photo advertimage -file $Conf::gamepath/images/cg_idle/support/support.tif

    $p.advert.ad configure -image advertimage

    place $p.advert -relx 0.0 -rely 0.0

    bind $p.advert <<Destroy>> "::Idlescreen::end"
    
    set nextevent [ after 20000 "::Idlescreen::end ; ::dm_next_IDLE $p" ]

}

proc end { } {

    variable advertimage
    variable nextevent

    catch [ list image delete advertimage ]
    
    catch [ list after cancel $nextevent ]

}

