""" Checks for sucess of vasp calculation """
class Success(object):
  """ Checks for success of vasp calculation """
  def __init__(self): object.__init__(self)

  def __get__(self, owner, type=None):
    from os.path import exists, join
    from ...launch import Launch
    import re

    for path in [Launch.OUTCAR, Launch.CONTCAR]:
      if self.indir != "": path = join(self.indir, path)
      if not exists(path): return False
      
    path = Launch.OUTCAR 
    if len(self.indir): path = join(self.indir, path)

    with open(path, "r") as file:
      regex = re.compile(r"""General\s+timing\s+and\s+accounting\s+informations\s+for\s+this\s+job""")
      for line in file:
        if regex.search(line) != None: return True
    return False

  def __set__(self, owner, value):
    raise TypeError, "Success cannot be set. It is a result only.\n"



