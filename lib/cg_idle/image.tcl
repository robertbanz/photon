
proc run { p } {
  variable advertimage
  variable nextevent

  canvas $p.advert -width [ winfo reqwidth $p ] -height [ winfo reqheight $p ]

  set advertimage [
    image create photo -file ${Conf::gamepath}/images/cg_idle/image.tif ]
  
  $p.advert create image 0 0 -image $advertimage -anchor nw
  
  place $p.advert -relx 0.0 -rely 0.0

  label $p.version -text $::version::version_string
      
  place $p.version -relx 1.0 -rely 1.0 -anchor se

  # wait 1 minute
  set nextevent [ after 45000 "::dm_next_IDLE $p" ]
}

proc end { p } {
    variable nextevent
    variable advertimage
    catch [ list after cancel $nextevent ]
    catch [ list image delete $advertimage ]
    destroy $p
}

