""" Miscellaneous """
import _opt 
from contextlib import contextmanager
__load_vasp_in_global_namespace__ = _opt.__load_vasp_in_global_namespace__
__load_pescan_in_global_namespace__ = _opt.__load_pescan_in_global_namespace__
cReals = _opt.cReals
_RedirectFortran = _opt._RedirectFortran
streams = _opt._RedirectFortran.fortran

class _RedirectPy:
  """ Redirects C/C++ input, output, error. """
  def __init__(self, unit, filename, append):
    self.unit = unit
    self.filename = filename
    self.append = append
  def __enter__(self):
    import sys
    if self.unit == streams.input:    self.old = sys.stdin
    elif self.unit == streams.error:  self.old = sys.stderr
    elif self.unit == streams.output: self.old = sys.stdout
    else: raise RuntimeError("Unknown redirection unit.")
    self.file = open(self.filename if len(self.filename)\
                else "/dev/null", "a" if self.append else "w")
    if self.unit == streams.input:    sys.stdin  = self.file
    elif self.unit == streams.error:  sys.stderr = self.file
    elif self.unit == streams.output: sys.stdout = self.file
    else: raise RuntimeError("Unknown redirection unit.")
    return self
  def __exit__(self, *wargs):
    import sys 
    if self.unit == streams.input:    sys.stdin  = self.old 
    elif self.unit == streams.error:  sys.stderr = self.old 
    elif self.unit == streams.output: sys.stdout = self.old 
    else: raise RuntimeError("Unknown redirection unit.")
    self.file.close()
      
class _RedirectAll:
  """ Redirects C/C++ input, output, error. """
  def __init__(self, unit, filename, append):
    self.unit = unit
    self.filename = filename
    if self.filename == None: self.filename = ""
    if self.filename == "/dev/null": self.filename = ""
    self.append = append
  def __enter__(self):
    import os
    import sys
    if self.unit == streams.input:    self.old = os.dup(sys.__stdin__.fileno())
    elif self.unit == streams.output: self.old = os.dup(sys.__stdout__.fileno())
    elif self.unit == streams.error:  self.old = os.dup(sys.__stderr__.fileno())
    else: raise RuntimeError("Unknown redirection unit.")
    if len(self.filename) == 0:
      self.filefd = os.open(os.devnull, os.O_RDWR | os.O_APPEND)
    else:
      self.filefd = os.open(self.filename, (os.O_RDWR | os.O_APPEND) if self.append\
                                           else (os.O_CREAT | os.O_WRONLY | os.O_EXCL))
    if self.append: self.file = os.fdopen(self.filefd, 'a')
    else: self.file = os.fdopen(self.filefd, 'w')
    if self.unit == streams.input:    os.dup2(self.file.fileno(), sys.__stdin__.fileno())
    elif self.unit == streams.output: os.dup2(self.file.fileno(), sys.__stdout__.fileno())
    elif self.unit == streams.error:  os.dup2(self.file.fileno(), sys.__stderr__.fileno())
    else: raise RuntimeError("Unknown redirection unit.")
    return self
  def __exit__(self, *wargs):
    import os
    import sys
    try: self.file.close()
    except: print "Am here"
    if self.unit == streams.input:    os.dup2(self.old, sys.__stdin__.fileno()) 
    elif self.unit == streams.output: os.dup2(self.old, sys.__stdout__.fileno())
    elif self.unit == streams.error:  os.dup2(self.old, sys.__stderr__.fileno())
    else: raise RuntimeError("Unknown redirection unit.")

def redirect_all(output=None, error=None, input=None, append = False):
  """ A context manager to redirect inputs, outputs, and errors. 
  
      @param output: Filename to which to redirect output. 
      @param error: Filename to which to redirect err. 
      @param input: Filename to which to redirect input. 
      @param append: If true, will append to files. All or nothing.
  """
  from contextlib import nested
  result = []
  for value, unit in [ (output, streams.output), (error, streams.error), (input, streams.input) ]:
    if value == None: continue
    result.append( _RedirectAll(unit=unit, filename=value, append=append) )
  return nested(*result)


def redirect(fout=None, ferr=None, fin=None, pyout=None, pyerr=None, pyin=None, append = False):
  """ A context manager to redirect inputs, outputs, and errors. 
  
      @param fout: Filename to which to redirect fortran output. 
      @param ferr: Filename to which to redirect fortran err. 
      @param fin: Filename to which to redirect fortran input. 
      @param pyout: Filename to which to redirect C/C++ output. 
      @param pyerr: Filename to which to redirect C/C++ err. 
      @param pyin: Filename to which to redirect C/C++ input. 
      @param append: If true, will append to files. All or nothing.
  """
  from contextlib import nested
  result = []
  for value, unit in [ (fout, streams.output), (ferr, streams.error), (fin, streams.input) ]:
    if value == None: continue
    result.append( _RedirectFortran(unit=unit, filename=value, append=append) )
  for value, unit in [ (pyout, streams.output), (pyerr, streams.error), (pyin, streams.input) ]:
    if value == None: continue
    result.append( _RedirectPy(unit=unit, filename=value, append=append) )
  return nested(*result)



def read_input(filename, global_dict=None, local_dict = None, paths=None, comm = None):
  """ Executes input script and returns local dictionary (as class instance). """
  # stuff to import into script.
  from os import environ
  from os.path import abspath, expanduser, join
  from math import pi 
  from numpy import array, matrix, dot, sqrt, abs, ceil
  from numpy.linalg import norm, det
  from lada.crystal import Lattice, Site, Atom, Structure, fill_structure, FreezeCell, FreezeAtom
  from lada import physics
  from boost.mpi import world
  
  # Add some names to execution environment.
  if global_dict == None: global_dict = {}
  global_dict.update( { "environ": environ, "pi": pi, "array": array, "matrix": matrix, "dot": dot,\
                        "norm": norm, "sqrt": sqrt, "ceil": ceil, "abs": abs, "Lattice": Lattice, \
                        "Structure": Structure, "Atom": Atom, "Site": Site, "physics": physics,\
                        "fill_structure": fill_structure, "world": world, "FreezeCell": FreezeCell, \
                        "FreezeAtom": FreezeAtom, "join": join, "abspath": abspath, \
                        "expanduser": expanduser})
  if local_dict == None: local_dict = {}
  # Executes input script.
  execfile(filename, global_dict, local_dict)

  # Makes sure expected paths are absolute.
  if paths != None:
    for path in paths:
      if path not in local_dict: continue
      local_dict[path] = abspath(expanduser(local_dict[path]))
    
  # Fake class which will be updated with the local dictionary.
  class Input:
    def __init__(self): self._inputfilepath = filename
    def __getattr__(self, name):
      raise AttributeError( "All out of cheese!\nRequired input parameter \"%s\" not found in %s." \
                            % (name, self._inputfilepath) )
  result = Input()
  result.__dict__.update(local_dict)
  return result


@contextmanager
def open_exclusive(*args, **kwargs):
  """ Opens and locks a file.

      This context uses fcntl to acquire a lock on file before reading or
      writing. Parameters, exceptions, and return are the same as the built-in
      open.

      Coding: As usual cray's make life difficult. flock does not work on
      compute nodes when using aprun. lockf does but requires the file to be
      opened with "w" only! Hence we lock a second file rather than the on of
      interest. We can't delete either, in case another process is competing
      with this one. Inconvenient to say the least.
  """
  from fcntl import lockf, LOCK_EX, LOCK_UN
  try:
    lock_filename = args[0] + ".lada_lockfile"
    lock_file = open(lock_filename, "w")
    lockf(lock_file, LOCK_EX)
    file = open(*args, **kwargs)
    yield file
  finally:
    file.close()
    lockf(lock_file, LOCK_UN)
    lock_file.close()

  
@contextmanager
def acquire_lock(filename):
  """ Locks access to a file. 

      This context uses fcntl to acquire a lock on file before reading or
      writing. Parameters, exceptions, and return are the same as the built-in
      open.

      As described below, the lock is not acquired on the file C{filename} itself.
      So a read or write on the file itself will work! Always use
      L{acqire_lock} and/or L{open_exclusive} or things won't work as expected.

      Coding: As usual cray's make life difficult. flock does not work on
      compute nodes when using aprun. lockf does but requires the file to be
      opened with "w" only! Hence we lock a second file rather than the on of
      interest. We can't delete either, in case another process is competing
      with this one. Inconvenient to say the least.
  """
  from fcntl import lockf, LOCK_EX, LOCK_UN
  try:
    lock_filename = filename + ".lada_lockfile"
    lock_file = open(lock_filename, "w")
    yield lock_file
  finally:
    lockf(lock_file, LOCK_UN)
    lock_file.close()

  
