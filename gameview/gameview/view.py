"""Photon GameViewer Application"""

import traceback
import re

from django.db import connection
from django.http import HttpResponse
from django.http import HttpResponseRedirect
from django.template.context_processors import csrf
from django.template.loader import get_template
from django.template import Context
from gameview.game import newgamecollection, GameSearch

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
  
def search_game_url(searchid, page, collection, gameid):
  return "/search/%s/%d/%s/%s" % (searchid, page, collection, gameid)

def search_index_url(searchid, page):
  return "/search/%s/%d" % (searchid, page)

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

def format_navigation(connection, g, searchhash=None):
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

  if searchhash:
    search = GameSearch(connection)
    search.do_with_hash(searchhash)
    next_search_id = search.nextid(g.collection().name(), g.indexid())
    prev_search_id = search.previousid(g.collection().name(), g.indexid())
    if next_search_id:
      next_search_url = search_game_url(searchhash, 0,
                                        next_search_id['collection'],
                                        next_search_id['id'])
    else:
      next_search_id = "THE END"
      next_search_url = "/static/theend.html"
    if prev_search_id:
      prev_search_url = search_game_url(searchhash, 0,
                                        prev_search_id['collection'],
                                        prev_search_id['id'])
    else:
      prev_search_id = "THE END"
      prev_search_url = "/static/theend.html"
    search_url = search_index_url(searchhash, 0)
    searchnav_template = get_template('templates/searchnav.html')
    searchnav = searchnav_template.render(
        Context({
            'prev_search_id': prev_search_id,
            'prev_search_url': prev_search_url,
            'next_search_id': next_search_id,
            'next_search_url': next_search_url,
            'search_index_url': search_url}))
  else:
    searchnav = ''

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
          'prev_day': prev_day,
          'searchnav': searchnav }))

def render_page(content):
  page_template = get_template('templates/page.html')
  return page_template.render(Context({'content': content}))

def error_page(error):
  error_template = get_template('templates/error.html')
  return render_page(error_template.render(Context({'error': error})))

def game(request, collection, gameid, searchid=None, searchpage=None):
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
  except LookupError:
    return HttpResponse(
        error_page("Cannot load game: %s" % (traceback.format_exc())))
  scores = format_game_scores(g)
  metadata = format_metadata(g)
  navigation = format_navigation(connection, g, searchid)
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

def search(request, hash=None, page=None, collectionid=None, id=None):
  if request.method == 'POST':
    return search_post(request)
  if id == None and hash == None:
    return search_form(request)
  elif id == None:
    return search_results(request, hash, page)
  else:
    return game(request, collectionid, id, hash, page)

def normalize_search_field(input):
  input = input.lower()
  return re.sub(r'[^a-z1-9]', '', input)

def search_post(request):
  codename = None
  if 'codename' in request.POST:
    codename = normalize_search_field(request.POST['codename'])
  search = GameSearch(connection, {'codename': codename})
  # populate database with search results.
  search.do()
  return HttpResponseRedirect('/search/%s/0' % (search._hash()))

def search_form(request):
  search_template = get_template('templates/searchform.html')
  c = {}
  c.update(csrf(request))
  search_content = search_template.render(Context(c), request)
  return HttpResponse(render_page(search_content), request)

def search_results(request, hash, page=None):
  results_template = get_template('templates/searchresults.html')
  gs = GameSearch(connection)
  results = gs.do_with_hash(hash)
  formcontext = gs.searchterms()
  formcontext.update(csrf(request))
  search_template = get_template('templates/searchform.html')
  for game in results:
    game.set_url(search_game_url(
        hash,
        0,
        game.collectionname(),
        game.gameid()))
  results_content = results_template.render(Context({
    'searchresults': results,
    'search_form': search_template.render(Context(formcontext))
  }))
  return HttpResponse(render_page(results_content), request)