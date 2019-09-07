
# Background

My first foray into the territory of Photon Game Software ([photon_gamma](https://github.com/robertbanz/photon_gamma)) was also my first major software development project beyond what the folks at school had me doing. As mentioned in the README for that project, when the first incarnation of what would become XP LaserSport launched it wasn’t with the software that I wrote. Someone else made a pretty good effort in making that happen.

  

However, there was some “professional rivalry” involved, and when I had heard that the software in question was not quite working up to snuff, I had gotten curious if I could do something better on a modern platform. I had become a FreeBSD nerd at the time, and wanted to get my feet we writing kernel device drivers -- so why not write a device driver to interface with a Photon central radio!?

# And So It Begins

## A Lack of Gear

By the time this happened, Photon Rosedale (my Parent’s Basement) had been shut down. The equipment I had available involved a couple of pods and spare boards. Certainly no central radio or entry terminal, I had given those back to SulSa so he could launch his new Photon!

## Entering A Pod

To do this, you need two things:

-   Something to modulate a serial IR signal.
    
-   Something to glob the right bits together to enter up a pod.
    

  

The second one was easy, as at this point I had a copy of the ET ROM source. However, the first one required hardware and I’m mostly a software guy, so I went the math route.

  

The IR signals that Photon uses are 38khz-modulated 8-n-1 serial at 1200 baud. Your run of the mill PC serial port can do 115kbps, and has control over 80% of the bits it outputs when you’ve accounted for the start/stop bits. (you have 8 bits you can control, and two of them you can’t). Assuming that whatever we use to demodulate these signals can accept some error, perhaps that’s enough. So, I wrote some stuff [to post later] that creates an acceptable signal to a Photon phaser board using a serial port with… get this… an IR LED just connected to the RS232 TX and ground.

  

And it worked. And I could enter-up a pod.

## But You Don’t Have A Radio

You’re right, I didn’t. But I did have schematics, and I did know where the radio in the POD dropped its stuff. So, I did what anyone would do and just directly soldered a serial port to the POD where the signals from the radio would have gone. (this would come in useful some years later when designing the “PhoBoard” used for the current Phocon system -- did exactly the same thing)

  

This rig would make it hard to run from base to top-center, but all I really need to know is that the timing was alright.

# Time To Show Off

At this point I had enough to hack together what was to become the ‘cycl’ driver [link to old photon commit] for FreeBSD 2.x. I was sure it did the thing.

  

I hacked together some really simple software that would run a Photon game in the most basic sense, so it was time for a live test...

  

My buddy HR was doing some tech work at the newly launched MarcTron (canonical term for any laser tag facility owned by Marc Mueller), and I was like “hey, I think I’ve got this thing”, so I loaded onto a laptop and one late night we gave it a go at the MarcTron facility and it worked! And didn’t exhibit the sync-loss problems that the incumbent’s software exhibited. (our “test case” as I remember was a 20+ minute game)

  

And then MarcTron closed, and there was a huge fallout with the incumbent’s peeps, and everyone thought it was over.

# XP LaserSport

Oh wait, this was MarcTron. MarcTron is like herpes -- you think it’s gone, but it’s always there. He had an angle on re-opening, but needed something to run the game. I had the basics of what could work, so I hobbled something together in rather short order.

  

The target platform at the time was FreeBSD 2.x which I had written the original kernel driver on. As I consider everything that went around it as “glue with some UX”, the decade-appropriate choice seemed to be to use Tcl/TK. It allowed for a pretty easy interface to bind to raw ‘C’ functions, but also some pretty decent capabilities for both UI and the scoring displays.

  

The original XP lasersport platform consisted of:

-   Game Computer - Ran the database and game software. Was loaded with a “multi-head” VGA video card that was connected to a couple VGA->Composite converters for the current game and post-game displays.
    
-   ET - NFS mounted /usr/photon from the game computer, and did the ET stuff.
    

  

The game computer also hosted a web interface to “pre-enter” games. The front counter could stage the game, and when you show up at the ET you just scan your passport and go.

## The Random Years

Not too long after XP LaserSport opened, I kind of checked out of the scene. Real job, life, and stuff. (Check out my videos doing fire performance). One of the MarcTron followers (codename Random) made a bunch of improvements to the software around handicapping and stuff. They might have made sense of the time.

# PhoCon

“Just when I thought I was out, they pull me back in…”

  

Me: So, all the software and ET, and everything still works right?

They: Yep.

  

[couple weeks before event]

They: Yeah, so we can’t find the ET.

  

…(hack together an ET on a laptop)...

  

[arrive at event]

Me: Well, this Game Computer’s hard drive is making a rather interesting clicking sound.

  

As you may remember, the first PhoCon went off rather well. I was shitting my pants.

## PhoCon The Second

Me: Ok, time to upgrade this shit. HD displays. Non-clicky hard drives.

  

I had built two laptops -- one for the ET (which is the ET we’re still using) and the second to run the game. I was hoping that it could handle interfacing with the central radio, but the backup was connecting over the network to some a shim running on the old game computer to do the deed. As was quickly learned: the modern hardware couldn’t keep sync, and the hacky network link was used.

## PhoCon: The Third (overdoing it)

The PhoBoard that interfaced with the central radio was launched, as it was clear the hardware at hand wasn’t going to hack it.

  

Jeff (Hit N Run) and I had decided that we needed to bring back Photon’s lighting experience as it really brings the room together.

  

This year I dropped in a mac mini running a VM for the game software, and the lighting controls running natively in MacOS.

## PhoCon The Fourth

“We need more bass.”

  

I think for this round I had eliminated all of the C bindings in the game software, and added a bunch of tested and fixed a bunch of bugs. And finally got the ET/Barcode Scanner interaction in its happy place. Also, did all of that from Australia, mate!

## PhoCon The Fifth

That was this year; we didn’t actually make (many) changes. Things just worked!
