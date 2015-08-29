"""Photon GameViewer Application"""

import traceback

from django.db import connection
from django.http import HttpResponse
from django.http import HttpResponseRedirect
from django.template.loader import get_template
from django.template import Context
from gameview.game import newgamecollection

# TODO:
# * search by codename.
# ** probably need to create something minus whitespace to search for
# ** misspellings.
# * search by teamname.
# * calendar.
# * authentication.
# * claim passports.
# * bookmark games.


def home(request):
  return HttpResponseRedirect('/static/gvindex.html')

def game_url(collection, gameid):
  return "/game/%s/%d" % (collection, gameid)

def format_seconds(s):
  return "%dm%02s" % (int(s / 60), s % 60)

def format_play_by_play(e):
  pbp = ""
  t = get_template('templates/pbp_second.html')
  body = get_template('templates/pbp.html')
  last_time = -1
  compiled_events = []
  count = 0
  for event in e:
    if event.timeremaining() != last_time:
      if len(compiled_events) > 0:
        if count % 2 == 1:
          odd = "odd"
        else:
          odd = "even"
        count += 1
        pbp += t.render(Context({'time': format_seconds(last_time),
                                 'events': compiled_events,
                                 'odd': odd}))
        compiled_events = []
      last_time = event.timeremaining()
    compiled_events.append(event.event())
  if len(compiled_events) > 0:
    if count % 2 == 1:
      odd = "odd"
    else:
      odd = "even"
    pbp += t.render(Context({'time': format_seconds(last_time),
                             'events': compiled_events,
                             'odd' : odd}))
    return body.render(Context({'pbp_seconds': pbp}))
  return ""

def format_metadata(g):
  metadata_template = get_template('templates/metadata.html')
  return metadata_template.render(Context({'g': g}))

def format_game_scores(g):
  # padd a team out to "10" slots by default,
  # otherwise, out to the largest team.
  pad_target = 10
  if len(g.redteam().players()) > pad_target:
    pad_target = len(g.redteam().players())
  if len(g.greenteam().players()) > pad_target:
    pad_target = len(g.greenteam().players())
  green_pad = range(len(g.greenteam().players()), pad_target)
  red_pad = range(len(g.redteam().players()), pad_target)


  scores_template = get_template('templates/scores.html')
  return scores_template.render(Context({'g': g,
                                         'green_pad': green_pad,
                                         'red_pad': red_pad}))

def format_navigation(g):
  # Get navigation points.
  next_id = g.collection().nextgameid(g.indexid())
  next_day = g.collection().nextday(g.indexid())
  prev_id = g.collection().previousgameid(g.indexid())
  prev_day = g.collection().previousday(g.indexid())
  if next_id:
    next_url = game_url(g.collection().name(), next_id)
  else:
    next_id = "THE END"
    next_url = "/static/theend.html"
  if prev_id:
    prev_url = game_url(g.collection().name(), prev_id)
  else:
    prev_id = "THE END"
    prev_url = "/static/theend.html"

  if next_day:
    next_day_url = game_url(g.collection().name(), next_day)
  else:
    next_day = "THE END"
    next_day_url = "/static/theend.html"
  if prev_day:
    prev_day_url = game_url(g.collection().name(), prev_day)
  else:
    prev_day = "THE END"
    prev_day_url = "/static/theend.html"


  navigation_template = get_template('templates/navigation.html')

  return navigation_template.render(
      Context({ 
          'next_url': next_url,
          'prev_url': prev_url,
          'prev_id': prev_id,
          'next_id': next_id,
          'prev_day_url': prev_day_url,
          'next_day_url': next_day_url,
          'next_day': next_day,
          'prev_day': prev_day}))

def render_page(content):
  page_template = get_template('templates/page.html')
  return page_template.render(Context({'content': content}))

def error_page(error):
  error_template = get_template('templates/error.html')
  return render_page(error_template.render(Context({'error': error})))

def game(request, collection, gameid):
  try:
    game_collection = newgamecollection(collection, connection)
  except LookupError:
    return HttpResponse(
        error_page("Cannot load collection: %s" % (traceback.format_exc())))

  if gameid == 'start':
    gameid = game_collection.nextgameid(0)
  elif gameid == 'end':
    gameid = game_collection.previousgameid(999999)

  try:
    g = game_collection.getgamebyid(gameid)
  except LookupError as e:
    return HttpResponse(
        error_page("Cannot load game: %s" % (traceback.format_exc())))
  
  scores = format_game_scores(g)
  metadata = format_metadata(g)
  navigation = format_navigation(g)
  # Render play by play if its available.
  if len(g.events()) > 0:
    pbp = format_play_by_play(g.events())
  else:
    pbp = ""
    
  game_template = get_template('templates/game.html')

  content = game_template.render(
      Context({ 
          'g': g,
          'navigation' : navigation,
          'pbp': pbp,
          'scores': scores,
          'metadata': metadata,
      }))
    
  return HttpResponse(render_page(content))
