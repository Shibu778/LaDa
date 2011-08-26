""" Class which keeps track of known candidates. """
__docformat__ = "restructuredtext en"
__all__ = ["History"]

class History(object):
   """ Class which keeps track of known candidates. 
   
       The history is kept via a history file. 
   """
   HISTORYCAR = "HISTORYCAR"
   """ Default history filename. """


   def __init__(self, filename = None, directory=None, limit=-1):
     """ Initializes a history object. """
     from ..opt import RelativeDirectory

     self.filename = filename if filename != None else self.HISTORYCAR
     """ file where history is saved. """
     self._directory = RelativeDirectory(directory)
     """ Directory where history file is saved. """
     self.limit = limit
     """ MAximum size of history. """

   @property
   def directory(self):
     """ Directory where history file is saved. """
     return self._directory.path
   @directory.setter 
   def directory(self, value): self._directory.path = value

   @property
   def filepath(self):
     """ Path to history file. """
     from os.path import join
     return join(self.directory, self.filename)

   def remove_stale(self, comm=None):
     """ Remove possibly stale lock and/or corrupt history file. """
     from os import remove
     from pickle import load
     from ..opt import LockFile
     from ..mpi import Communicator
     lock = LockFile(self.filepath)
     lock.remove_stale(comm)
     comm = Communicator(comm)
     if comm.is_root:
       lock.lock()
       try: 
         with open(self.filepath, "r") as file: result = load(file)
       except:
         try: remove(self.filepath)
         except: pass
     comm.barrier()

   def _update(self, hist, other):
     """ Returns other with updated attributes from history. """
     other.genes = hist.genes
     if hasattr(other, "__dict__"): other.__dict__.update(hist.__dict__)
     else:
       for slot in other.__slots__: 
         if hasattr(hist, slot) and hasattr(other, slot): 
           setattr(other, slot, getattr(hist, slot))

   def __call__(self, indiv, add=True):
     """ Returns true if individual is in history. """
     from pickle import load, dump
     from os.path import exists
     from ..opt import acquire_lock
     with acquire_lock(self.filepath) as lock:

       if not exists(self.filepath):
         with open(self.filepath, "w") as file: dump([indiv], file)
         return False
       
       with open(self.filepath, "r") as file: history = load(file)
       if indiv in history: return True
       if not add: return False
       history.append(indiv)
       while self.limit > 0 and self.limit < len(history): history.pop(0)
         
       with open(self.filepath, "w") as file: dump(history, file)
       return False

   def get(self, indiv):
     """ Returns individual if it exists in history. """
     from pickle import load
     from os.path import exists
     from copy import deepcopy
     from ..opt import acquire_lock
     if not exists(self.filepath): return 
     with acquire_lock(self.filepath) as lock:
       with open(self.filepath, "r") as file: history = load(file)
       if indiv in history:
         result = deepcopy(indiv)
         self._update(history[history.index(indiv)], result)
         return result

   def replace(self, indiv):
     """ Replaces an already existing indiviual. 


         Throws ValueError if the individual does not exist. 
     """ 
     from pickle import dump, load
     from os.path import exists
     from ..opt import acquire_lock
     if not exists(self.filepath): return 
     with acquire_lock(self.filepath) as lock:
       with open(self.filepath, "r") as file: history = load(file)
       try: history[history.index(indiv)] = indiv
       except ValueError: raise ValueError("Individual not in history.")
       with open(self.filepath, "w") as file: dump(history, file)
       
   def __contains__(self, indiv):
     """ True if the individual is in history. """
     from ..opt import acquire_lock
     from pickle import load
     from os.path import exists
     if not exists(self.filepath): return False
     with acquire_lock(self.filepath) as lock:
       with open(self.filepath, "r") as file: history = load(file)
       return indiv in history

   def pop(self, indiv):
     """ Removes individual if it exists in history. """
     from pickle import load, dump
     from os.path import exists
     from copy import deepcopy
     from ..opt import acquire_lock
     if not exists(self.filepath): return 
     with acquire_lock(self.filepath) as lock:
       with open(self.filepath, "r") as file: history = load(file)
       if indiv in history: 
         result = deepcopy(indiv)
         self._update(history.pop(history.index(indiv)), result)
         with open(self.filepath, "w") as file: dump(history, file)
         return result

   @property 
   def candidates(self):
     """ List of all known candidates. """
     from pickle import load
     from os.path import exists
     from ..opt import acquire_lock
     if not exists(self.filepath): return []
     with acquire_lock(self.filepath) as lock:
       with open(self.filepath, "r") as file: return load(file)

   @property
   def filesize(self):
     """ Returns size of history file in human readable format. """
     size = float(self._filesize)
     for suffix in ["b", "Kb", "Mb", "Gb", "Tb"]:
       if size < 1024.0: break
       size /= 1024.0
     return str(size) + " " + suffix

   @property
   def _filesize(self):
     """ Returns size of history file unformated. """
     from os.path import exists, getsize
     from ..opt import acquire_lock
     if not exists(self.filepath): return 0
     with acquire_lock(self.filepath) as lock: return getsize(self.filepath)
     return str(size) + " " + suffix

   def __repr__(self):
     """ Python representation of this object. """
     return "{0.__class__.__name__}({1}, {2}, {0.limit})"\
            .format(self, repr(self.filename), repr(self.directory))
