"""Game objects and Game Collections."""

import re
import string
import json
import hashlib

from datetime import datetime
from django.db import transaction
from string import Template


def dictfetchall(cursor):
  desc = cursor.description
  return [
      dict(zip([col[0] for col in desc], row))
      for row in cursor.fetchall()
  ]

class Player(object):
  """A Photon Player"""

  def __init__(self, slot, passport, codename, score):
    self._slot = slot
    if passport:
      self._passportid = passport
    else:
      self._passportid = ""
    self._codename = codename
    if score:
      self._score = score
    else:
      self._score = 0

  def score(self):
    return self._score

  def codename(self):
    return self._codename

  def passportid(self):
    return self._passportid

  def __str__(self):
    return "(%s) %s %d" % (self.passportid(), self.codename(), self.score())

class GameEvent(object):
  """An play-by-play event"""

  def __init__(self, timeremaining, event):
    self._timeremaining = timeremaining
    self._event = event

  def timeremaining(self):
    return self._timeremaining

  def event(self):
    return self._event


class Team(object):
  """A team of players"""
  def __init__(self, color, name, score):
    self._players = []
    self._color = color
    self._name = name
    self._score = score

  def score(self):
    return self._score

  def name(self):
    return self._name

  def players(self):
    return self._players

  def addplayer(self, player):
    self._players.append(player)

  def color(self):
    return self._color

  def sortedplayers(self):
    return sorted(self._players, key=lambda score: score.score(), reverse=True)

class GameSearchResult(object):
  def __init__(self, collectionname, gameid):
    self._redteamname = None
    self._greenteamname = None
    self._gamemode = None
    self._gamenumber = None
    self._gamedatetime = None
    self._collectionname = collectionname
    self._gameid = gameid

  def redteamname(self):
    return self._redteamname

  def greenteamname(self):
    return self._greenteamname

  def gamemode(self):
    return self._gamemode

  def gamenumber(self):
    return self._gamenumber

  def gamedatetime(self):
    return self._gamedatetime

  def collectionname(self):
    return self._collectionname

  def gameid(self):
    return self._gameid    

  def set_url(self, url):
    self._url = url

  def url(self):
    return self._url;

  def populate_from_result(self, result):
    self._redteamname = result['red_teamname']
    self._greenteamname = result['green_teamname']
    self._gamemode = result['mode']
    if 'manuever' in result:
      self._gamenumber = result['manuever']
    else:
      self._gamenumber = result['id']
    self._gamedatetime = result['rantime']
    

class GameSearch(object):
  # TODO: these should be defined somewhere else.
  classiccollections = ['baltimore89']
  collections = sorted(['laurel', 'pc2016',
                        'pc2015', 'baltimore92', 'chicago92'])

  def __init__(self, connection, searchterms={ }):
    self._searchterms = searchterms
    self._connection = connection
    self._resultcount = None
    self._resultlist = []
    self._myhash = None
  def _serialize(self):
    return json.dumps(self._searchterms)

  def _deserialize(self, data):
    self._searchterms = json.loads(data)
    return True
    
  def searchterms(self):
    return self._searchterms

  def _hash(self):
    if self._myhash:
      return self._myhash
    else:
      digester = hashlib.sha1()
      digester.update(self._serialize())
      return digester.hexdigest()
  
  def _load_from_hash(self):
    '''load search paramters from database by hash'''
    cursor = self._connection.cursor()
    cursor.execute('SELECT * from searches where hash = %s', [self._hash()])
    if cursor.rowcount == 0:
      raise LookupError
    result = dictfetchall(cursor)
    self._deserialize(result[0]['searchterms'])
  
  def _populate_results(self):
    cursor = self._connection.cursor()
    for collectionname in sorted(self.collections):
      collection = newgamecollection(collectionname, self._connection)
      self._resultlist.extend(collection.searchresultsfor(self._hash()))

  def do_with_hash(self, hash):
    self._myhash = hash
    self._load_from_hash()
    self._populate_results()
    return self._resultlist

  def do(self):
    cursor = self._connection.cursor()
    cursor.execute('SELECT * FROM searches WHERE hash = %s', [self._hash()])
    if cursor.rowcount != 1:
      self._do_real_search()
    self._populate_results()
    return self._resultlist
  
  def nextid(self, collection, gameid):
    cursor = self._connection.cursor()
    cursor.execute(
        'SELECT * FROM search_results WHERE hash = %s '
        'AND ((id > %s and collection = %s) '
        'OR (collection > %s)) '
        'ORDER BY collection ASC, id ASC LIMIT 1', [
          self._hash(), gameid, collection, collection])
    if cursor.rowcount != 1:
      return None
    else:
      result = dictfetchall(cursor)
      return { 'collection': result[0]['collection'],
               'id': result[0]['id'] }
    
  def previousid(self, collection, gameid):
    cursor = self._connection.cursor()
    cursor.execute(
        'SELECT * FROM search_results WHERE hash = %s '
        'AND ((id < %s and collection = %s) '
        'OR (collection < %s)) '
        'ORDER BY collection DESC, id DESC LIMIT 1', [
          self._hash(), gameid, collection, collection])
    if cursor.rowcount != 1:
      return None
    else:
      result = dictfetchall(cursor)
      return { 'collection': result[0]['collection'],
               'id': result[0]['id'] }
        
  # This should do transactions somehows.
  def _do_real_search(self):
    cursor = self._connection.cursor()
    cursor.execute("DELETE FROM search_results WHERE hash = %s", [
        self._hash()])
    cursor.execute("DELETE FROM searches WHERE hash = %s", [
        self._hash()])
    for collectionname in self.collections:
      collection = newgamecollection(collectionname, self._connection)
      searchquery = collection.searchquery(self._searchterms)
      cursor.execute(
          "INSERT INTO search_results (hash, collection, id) " +
          searchquery, [self._hash(),collectionname])
      #transaction.set_dirty()
    cursor.execute(
        "INSERT INTO searches (hash, searchterms, created) VALUES " +
        "(%s, %s, CURRENT_TIMESTAMP)", [self._hash(), self._serialize()])
    #transaction.commit_unless_managed()
    return self._hash()
    

class Game(object):
  """A Game.
     The member attributes of this are directly populated by a
     GameCollection when loading the database.
  """

  # pylint: disable=too-many-instance-attributes

  _daysofweek = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']
  def __init__(self, collection, gameid):
    self._team = {0: None, 1: None}
    self._gamenumber = None
    self._gamedatetime = None
    self._gamemode = None
    self._gamelength = None
    self._indexid = None
    self._collection = collection
    self._indexid = int(gameid)
    self._events = []
    self._slot = {}

  def redteam(self):
    """Returns a Team"""
    return self._team[0]

  def greenteam(self):
    """Returns a Team"""
    return self._team[1]

  def gamenumber(self):
    """Returns the Game Number (maneuver)"""
    return self._gamenumber

  def gamedatetime(self):
    """Returns a datetime of when the game was ran"""
    return self._gamedatetime

  def gamedayofweek(self):
    """Returns a day of the week (string) when the game was ran"""
    return self._daysofweek[self.gamedatetime().weekday()]

  def gamemode(self):
    """Returns the game mode string (e.g. League, Public)"""
    return self._gamemode

  def gamelength(self):
    """Returns the length of the game in seconds."""
    return self._gamelength

  def indexid(self):
    """Returns the database ID of the game. (not necessarily the gamenumber)"""
    return self._indexid

  def collection(self):
    """Returns the collection this game was from."""
    return self._collection

  def events(self):
    """Returns a list of game events, if supported by the database."""
    return self._events

  def __str__(self):
    o = "Maneuver: %d, Mode: %s, DateTime %s<br>" % (
        self.gamenumber(), self.gamemode(), self.gamedatetime())
    o += "Index ID: %d, collection %s<br>" % (
        self._indexid, self._collection.name())
    o += "Red Team: %s, Green Team %s<br>" % (
        self.redteam().name(), self.greenteam().Name())
    o += "Red Team Score: %d, Green Team Score %d<br>" % (
        self.redteam().score(), self.greenteam().score())
    for t in [self.redteam(), self.greenteam()]:
      for p in t.players():
        o += "%s %s<br>" % (t.color(), p)
    return o


class GameCollection(object):
  """Abstract Game Collection"""
  def __init__(self, name):
    self._name = name

  def name(self):
    return self._name

  def _dictfetchall(self, cursor):
    desc = cursor.description
    return [
        dict(zip([col[0] for col in desc], row))
        for row in cursor.fetchall()
    ]

def newgamecollection(collection, connection=None):
  """Builder for a game collection."""
  if collection == 'pc2015':
    return TCLGameCollection(connection, collection, "pc2015_", 3)
  elif collection == 'pc2016':
    return TCLGameCollection(connection, collection, "pc2016_", 3)
  elif collection == 'baltimore89':
    return ClassicGameCollection(connection, collection, "bmore89_", 10)
  elif collection == 'baltimore92':
    return B90GameCollection(connection, collection, "bmore90_", 26)
  elif collection == 'chicago92':
    return B90GameCollection(connection, collection, "chi90_", 26)
  elif collection == 'laurel':
    return TCLGameCollection(connection, collection, "laurel_", 2)
  else:
    raise LookupError("Invalid game collection id.")

class MockGameCollection(GameCollection):
  """Mock game collection."""
  def __init__(self, name):
    super(MockGameCollection, self).__init__(name)

  def getgamebyid(self, gameid):
    o = Game(self, gameid)
    o._gamemode = "Public"
    o._gamelength = 360
    o._gamedatetime = datetime(1986, 05, 05, 12, 00)
    o._gamenumber = 999
    return o

class SQLGameCollection(GameCollection):
  """Game collection from SQL"""

  def __init__(self, connection, name, prefix, store):
    self._prefix = ""
    self._members_table = "members"
    self._games_table = "g_data"
    self._players_table = "g_players"
    self._events_table = "g_events"
    self._prefix = prefix
    self._members_table = self._prefix + self._members_table
    self._games_table = self._prefix + self._games_table
    self._players_table = self._prefix + self._players_table
    self._events_table = self._prefix + self._events_table
    self._store = store
    self._connection = connection
    super(SQLGameCollection, self).__init__(name)

  def getgamebyid(self, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "SELECT * from " + self._games_table +
        " WHERE id = %s and storeid = %s and state = 'COMPLETE'", [
            int(indexid), self._store])
    if cursor.rowcount != 1:
      raise LookupError("No or multiple rows returned?")
    result = self._dictfetchall(cursor)
    o = Game(self, indexid)
    o._team[0] = Team("red", result[0]['red_teamname'],
                      result[0]['red_teamscore'])
    o._team[1] = Team("green", result[0]['green_teamname'],
                      result[0]['green_teamscore'])
    o._gamenumber = result[0]['id']
    o._gamedatetime = result[0]['rantime']
    o._gamemode = result[0]['mode']
    o._gamelength = result[0]['length']

    self._loadplayerstogame(o, indexid)
    return o

  def previousgameid(self, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "select id from " + self._games_table +
        " WHERE id < %s and storeid = %s " +
        " ORDER BY id DESC LIMIT 1", [int(indexid), self._store])
    if cursor.rowcount != 1:
      return None
    result = self._dictfetchall(cursor)
    return result[0]['id']

  def nextgameid(self, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "select id from " + self._games_table +
        " WHERE id > %s and storeid = %s " +
        " ORDER BY id ASC LIMIT 1", [int(indexid), self._store])
    if cursor.rowcount != 1:
      return None
    result = self._dictfetchall(cursor)
    return result[0]['id']

  def nextday(self, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "select id from " + self._games_table +
        " WHERE id > %s AND storeid = %s " +
        " AND rantime::date > (select rantime::date from " +
        self._games_table + " WHERE id = %s) ORDER BY id,rantime ASC LIMIT 1",
        [int(indexid), self._store, int(indexid)])
    if cursor.rowcount != 1:
      return None
    result = self._dictfetchall(cursor)
    return result[0]['id']

  def previousday(self, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "select id from " + self._games_table +
        " WHERE id < %s AND storeid = %s " +
        " AND rantime::date < (select rantime::date from " +
        self._games_table + " where id = %s and storeid = %s) " +
        "ORDER BY rantime DESC LIMIT 1",
        [int(indexid), self._store, int(indexid), self._store])
    if cursor.rowcount != 1:
      return None
    result = self._dictfetchall(cursor)
    return result[0]['id']

  def _loadplayerstogame(self, game, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "SELECT * from " + self._players_table +
        " WHERE id = %s and storeid = %s", [
            int(indexid), self._store])
    for player in self._dictfetchall(cursor):
      p = Player(player['slot'], player['idnum'], player['name'],
                 player['score'])
      game._team[player['team']].addplayer(p)
      slotid = player['slot']
      if player['team'] == 1:
        slotid += 20
      game._slot[slotid] = p

  def searchquery(self, parameters):
    query_template = Template(
        "SELECT DISTINCT %s as hash, %s as collection, d.id as id " +
        "FROM "
        "$gametable d, $playerstable p WHERE d.id = p.id AND "
        "p.searchcodename LIKE '%%$codename%%' ORDER BY d.id")
    if 'codename' not in parameters:
      raise LookupError
    myparams = {'gametable': self._games_table,
                'playerstable': self._players_table,
                'codename': parameters['codename']}
    return query_template.substitute(myparams)

  def searchresultsfor(self, hash):
    out = []
    query_template = Template(
        "SELECT DISTINCT d.id as id, d.mode as mode, " +
        "d.storeid as storeid, d.red_teamname as red_teamname, " +
        "d.green_teamname as green_teamname, d.rantime as rantime " +
        "FROM $gametable d, search_results r WHERE "
        "hash = %s AND collection = %s AND d.id = r.id AND " +
        "d.storeid = %s ORDER BY d.id ASC")
    query = query_template.substitute({'gametable': self._games_table})
    cursor = self._connection.cursor()
    cursor.execute(query, [hash, self.name(), self._store])
    for row in dictfetchall(cursor):
      gs = GameSearchResult(self.name(), row['id'])
      gs.populate_from_result(row)
      out.append(gs)
    return out
      
class TCLGameCollection(SQLGameCollection):
  def getgamebyid(self, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "SELECT * from " + self._games_table +
        " WHERE id = %s and storeid = %s and state = 'COMPLETE'", [
            int(indexid), self._store])
    if cursor.rowcount != 1:
      raise LookupError("No or multiple rows returned?")
    result = self._dictfetchall(cursor)
    o = Game(self, indexid)
    o._team[0] = Team("red", result[0]['red_teamname'],
                      result[0]['red_teamscore'])
    o._team[1] = Team("green", result[0]['green_teamname'],
                      result[0]['green_teamscore'])
    o._gamenumber = result[0]['id']
    o._gamedatetime = result[0]['rantime']
    o._gamemode = result[0]['mode']
    o._gamelength = result[0]['length']

    self._loadplayerstogame(o, indexid)
    self._loadplaybyplay(o, indexid)
    return o

  def _loadplaybyplay(self, game, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "SELECT * from " + self._events_table +
        " WHERE id = %s and storeid = %s", [
            int(indexid), self._store])
    for event in self._dictfetchall(cursor):
      ev = self._playbyplayreplace(game, event['event'])
      game._events.append(GameEvent(event['timestamp'], ev))

  def searchquery(self, parameters):
    query_template = Template(
        "SELECT DISTINCT %s as hash, %s as collection, d.id as id " +
        "FROM " +
        "$gametable d, $playerstable p WHERE d.id = p.id AND " +
        "d.state = 'COMPLETE' AND " +
        "p.searchcodename LIKE '%%$codename%%' ORDER BY d.id")
    if 'codename' not in parameters:
      raise LookupError
    myparams = {'gametable': self._games_table,
                'playerstable': self._players_table,
                'codename': parameters['codename']}
    return query_template.substitute(myparams)


  def _playbyplayreplace(self, game, event):
    o = []
    m = re.compile(r'%(\d+)%')
    for i in string.split(event, ' '):
      r = m.match(i)
      if r:
        try:
          o.append(game._slot[int(r.group(1))].codename())
        except:
          o.append(i)
      else:
        o.append(i)
    return " ".join(o)

  def previousgameid(self, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "select id from " + self._games_table +
        " WHERE id < %s and storeid = %s AND state = 'COMPLETE'" +
        " ORDER BY id DESC LIMIT 1", [int(indexid), self._store])
    if cursor.rowcount != 1:
      return None
    result = self._dictfetchall(cursor)
    return result[0]['id']

  def nextgameid(self, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "select id from " + self._games_table +
        " WHERE id > %s and storeid = %s AND state = 'COMPLETE'" +
        " ORDER BY id ASC LIMIT 1", [int(indexid), self._store])
    if cursor.rowcount != 1:
      return None
    result = self._dictfetchall(cursor)
    return result[0]['id']

class OldSchoolPlayer(Player):

  def __init__(self, slot, passport, score):
    super(OldSchoolPlayer, self).__init__(slot, passport, "", score)    

  def codename(self):
    return self.passportid()

class ClassicGameCollection(SQLGameCollection):

  def getgamebyid(self, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "SELECT * from " + self._games_table +
        " WHERE id = %s and storeid = %s", [
            int(indexid), self._store])
    if cursor.rowcount != 1:
      raise LookupError("No or multiple rows returned?")
    result = self._dictfetchall(cursor)
    o = Game(self, indexid)
    o._team[0] = Team("red", result[0]['red_teamname'],
                      result[0]['red_teamscore'])
    o._team[1] = Team("green", result[0]['green_teamname'],
                      result[0]['green_teamscore'])
    o._gamenumber = result[0]['manuever']
    o._gamedatetime = result[0]['rantime']
    o._gamemode = result[0]['mode']
    o._gamelength = 360

    self._loadplayerstogame(o, indexid)
    return o

  def _loadplayerstogame(self, game, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "SELECT * from " + self._players_table +
        " WHERE id = %s and storeid = %s", [
            int(indexid), self._store])
    for player in self._dictfetchall(cursor):
      game._team[player['team']].addplayer(
          OldSchoolPlayer(player['slot'], player['idnum'], player['score']))


class B90GameCollection(ClassicGameCollection):
  def _loadplayerstogame(self, game, indexid):
    cursor = self._connection.cursor()
    cursor.execute(
        "SELECT * from " + self._players_table +
        " WHERE id = %s and storeid = %s", [
            int(indexid), self._store])
    for player in self._dictfetchall(cursor):
      game._team[player['team']].addplayer(
          Player(player['slot'], player['idnum'],
                 player['name'], player['score']))
