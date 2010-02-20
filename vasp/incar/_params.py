""" Standard parameter types for use as attributes in Incar """
class Standard(object):
  """
      A standard parameter in the form of key/value pair. 
      
      In practice, the key should be an INCAR tag, and the value something
      which makes sense for that key.
      C{print self.incar_string(vasp, mpicomm)} outputs C{print
      str(self.key), "=", str(self.value)}.
  """
  def __init__(self, key, value, validity=None):
    super(Standard, self).__init__()

    self.key = key
    """ Any string """
    self._value = value
    """ Any value """
    self.validity = validity
    """ Functor returning true if the value is valid. """

  # getter setter functions for value.
  def _getvalue(self): return self._value
  def _setvalue(self, value): 
    if self.validity != None:
      if not self.validity(value):
        raise ValueError, "Value %s = %s is invalid.\n%s\n"  % (self.key, value, self.__doc__)
    self._value = value

  value = property(_getvalue, _setvalue)
  """ Actual property 
  
      It takes a meaningful value when set (eg a string), and
      returns its  "VASP" equivalent when gotten.
  """

  def incar_string(self, vasp, mpicomm):
    """ Prints the key/value as key = value """
    return "%s = %s" % (self.key, str(self.value))

  def __getstate__(self):
    from marshal import dumps
    d = self.__dict__.copy()
    del d["validity"]
    if self.validity == None: return d, None
    return d, dumps(self.validity.func_code)
  def __setstate__(self, arg):
    from marshal import loads
    self.__dict__.update(arg[0])
    self.validity = None
    if arg[1] != None: 
      self.validity = lambda x: x
      self.validity.func_code  = loads(arg[1])


class NoPrintStandard(Standard):
  """ Does not print if value is the default given on initialization. """
  def __init__(self, *args, **kwargs):
    super(NoPrintStandard, self).__init__(*args, **kwargs)
    self.default = self.value
  def incar_string(self, vasp, mpicomm):
    if self.default == self.value: 
      return "# %s = VASP default." % (self.key)
    return super(NoPrintStandard, self).incar_string(vasp, mpicomm)
  def __setstate__(self, arg): 
    super(NoPrintStandard, self).__setstate__(arg)
    self.default = self.value

class Iniwave(Standard):
  """ Iniwave variable 

      self.value can be either "random" or "jellium".
  """
  def __init__(self, value = "random"):
    validity = lambda x: x=="random" or x=="jellium"
    super(Iniwave, self).__init__("INIWAVE", value, validity)

  def _getvalue(self): 
    if self._value == "random": return 1
    elif self._value == "jellium": return 0
    else: raise RuntimeError, "Internal bug."

  value = property(_getvalue, Standard._setvalue)
  """ Iniwave property
  
      It takes a meaningful value when set (C{self.value = random})
        - random: wavefunctions are initalized with random number. Safest. 
        - jellium: wavefunctions are initalized a jelliums. Not so safe.
      returns its  "VASP" equivalent when gotten (C{print self.value} outputs 0 or 1).
  """


class Algo(Standard): 
  """ Electronic minimization. 
      Can be \"very fast\", \"fast\", or \"normal\" (default). 
  """ 
  def __init__(self):
    super(Algo, self).__init__("ALGO", "normal") 
  def _getvalue(self):
    if self._value not in ["Very_Fast", "Fast", "Normal"]:
      self.value = self._value
    return self._value
  def _setvalue(self, value):
    lower = value.lower().rstrip().lstrip()
    lower = lower.replace("_", " ")
    lower = lower.replace("-", " ")
    if lower == "very fast": self._value = "Very_Fast"
    elif lower == "fast": self._value = "Fast"
    elif lower == "normal": self._value = "Normal"
    else: raise ValueError, "%s = %s is invalid.\n%s\n" % (self.key, value, self.__doc__)
  value = property(_getvalue, _setvalue)

class Precision(Standard):
  """ Sets accuracy of calculation. 
      Can be \"accurate\" (default), \"low\", \"medium\", \"high\".
  """
  def __init__(self): 
    super(Precision, self).__init__("PREC", "accurate") 
  def _getvalue(self):
    if self._value not in ["accurate", "lower", "medium", "high"]:
      self.value = self._value
    return self._value
  def _setvalue(self, value):
    self._value = value.lower()
    if self._value not in ["accurate", "lower", "medium", "high"]:
      raise ValueError, "%s = %s is invalid.\n%s\n" % (self.key, value, self.__doc__)
  value = property(_getvalue, _setvalue)

class Ediff(Standard):
  """ Sets the convergence criteria (per atom) for electronic minimization.

      This tolerance is divided by the number of atoms in the system. This
      makes tolerance consistent from one system to the next.
  """
  def __init__(self, value = 1e-4):
    super(Ediff, self).__init__("EDIFF", value, validity = lambda x: x > 0e0)
  def incar_string(self, vasp, mpicomm):
    return "%s = %f " % (self.key, self.value * float(len(vasp._system.atoms)))


class Encut(object):
  """ Defines energy cutoff for calculation. 
      The base cutoff is obtained from the species of the owner specified in
      __init__. The actual cutoff is that times the safety (also specified in __init__).
  """
  class Safety(object):
    def __init__(self, value): self.value = value
    def __set__(self, owner, value):
      assert value > 0, "Ecut safety must be larger than 0.\n"
      self._value = value
    def __get__(self, owner, parenttype=None): return self._value

  safety = Safety(1.25)
  """ A safety upon the largest ENMAX """

  def __init__(self, safety = 1.25):
    super(Encut, self).__init__()
    self.safety = safety

  def _getvalue(self):
    raise ValueError, "Value of encut cannot be obtained directly. use enmax method instead.\n"
  def _setvalue(self, value):
    raise ValueError, "Value of encut cannot be set manually. Change safety instead.\n"
  x = property(_getvalue, _setvalue)
  def incar_string(self, vasp, mpicomm):
    return "ENCUT = %f " % (self.enmax(vasp.species) * self.safety)

  @staticmethod
  def enmax(species):
    """ Retrieves max ENMAX from list of species. """
    import os.path
    import subprocess
    import re
    from math import ceil

    result = 0
    for s in species: 
      if not os.path.exists( os.path.join(s.path, "POTCAR") ):
        raise AssertionError, "Could not find potcar in " + s.path
      with open(os.path.join(s.path, "POTCAR"), "r") as potcar: 
        r = re.compile("ENMAX\s+=\s+(\S+);\s+ENMIN")
        p = r.search(potcar.read())
        if p == None: raise AssertionError, "Could not retrieve ENMAX from " + s.path
        if result < float( p.group(1) ): result = float( p.group(1) )

    return ceil(result)

class Smearing(object):
  """ Value of the smearing used in the calculation. 
      It can be specified as a string: "type x", where type is any of fermi,
      gaussian, mp, tetra, metal or insulator, and x is the energy scale in eV.
          - fermi use a Fermi-Dirac broadening.
          - gaussian uses a gaussian smearing.
          - mp is for Methfessel-Paxton, it should be specified as "mp N x",
            where N is the order the mp method.
          - tetra tetrahedron method without Bloechl correction.
          - bloechl means tetrahedron method with Bloechl correction.
          - "metal x" is equivalent to "mp 1 x"
          - insulator is equivalent to "tetra bloechl".
          - if x is omitted a default value of 0.2eV is used.
  """
  def __init__(self, string = "insulator 0.2"):
    super(Smearing, self).__init__()
    self.value = string

  def _getvalue(self): return (self.type, self.smearing)
  def _setvalue(self, value):
    first = None
    second = None 
    third = None

    if isinstance(value, str):
      data = value.split()
      first = data[0].lower()
      if len(data) > 1: second = data[1]
      if len(data) > 2: third  = data[2]
      if len(data) > 3: raise "Unknown input %s. smearing: %s\n" % (value, self.__doc__)
    elif hasattr(value, __len__):
      first = str(value[0]).lower()
      if len(value) > 1: second = value[1]
      if len(value) > 2: third  = value[2]
      if len(value) > 3: raise "Unknown input %s. smearing: %s\n" % (value, self.__doc__)
    elif isinstance(value, float):
      self.value = float(value)
      return
    elif isinstance(value, int): raise "int is not an accepted type for smearing."

    if first == "fermi" or first == "-1":    
      self.type == -1
      if second != None: self.smearing = float(second)
    elif first == "gaussian" or first == "0":
      self.type == 0
      if second != None: self.smearing = float(second)
    elif first == "metal":
      self.type = 1
      if second != None: self.smearing = float(second)
    elif first == "mp" or first == "metal":
      if third != None:
        self.type = int(second)
        self.smearing = float(third)
      elif second != None:
        self.type = int(second)
      else: self.type = 1
      assert self.type >= 1, "Mehtfessel-Paxton order must be at least 1."
    elif first == "bloechl" or first == "-5" or first == "insulator":
      self.type = -5
      if second != None: self.smearing = float(second)
    elif first == "tetra" or first == "-4":
      self.type = -4
      if second != None: self.smearing = float(second)
    else: 
      try: self._value = int(first)
      except: raise ValueError, "Unknown smearing value %s.\nsmearing: %s\n" % (value, self.__doc__)
      assert self._value >= 1, "Unknown smearing value %s.\nsmearing: %s\n" % (value, self.__doc__)
  value = property(_getvalue, _setvalue)
  """
      Type of smearing used.
      If "gotten", this decorator returns a tuple containing the type and the energy scale.
      It can be set using a similar tuple, or a string input with keywords. 
  """
  def incar_string(self, vasp, mpicomm):
    return "ISMEAR  = %s\nSIGMA   = %f\n" % self.value

  
class Isym(Standard):
  """ Type of symmetry used in the calculation.
      Can be "off" or a float corresponding to the tolerance used to determine
      symmetry operation.  By default, it is 1e-5.
  """
  def __init__(self, value = 1e-5):
    super(Isym, self).__init__("SYMPREC", value, None)
    self._value = value

  def _getvalue(self):
    if self._value <= 0e0: return "off"
    return self._value
  def _setvalue(self, value):
    strval = str(value)
    if strval == "off": self._value = -1e0
    else: self._value = float(value) 
  value = property(_getvalue, _setvalue)
  def incar_string(self, vasp, mpicomm):
    if self._value <= 0e0: return "ISYM = 0\n" 
    return super(Isym,self).incar_string(vasp, mpicomm)

  def __eq__(self, other):
    if isinstance(other, Isym):
      return self._value == self._value
    return self == Isym(other)
  
class FFTGrid(object):
  """ Computes fft grid using VASP. Or if grid is given, computes using that grid. """
  def __init__(self, grid = None): 
    super(FFTGrid, self).__init__()
    self._value = grid

  def _getvalue(self):
    if self._value == None:
      raise "Value is computed from VASP. Call self.__call__(vasp) instead.\n"
    return self._value

  def _setvalue(self, value):
    self._value = value
    if self._value != None: 
      assert len(self._value) == 3, "FFT grid should be none or a 3-tuple.\n"
      assert self._value[0] > 0, "FFT grid components should be positive.\n"
      assert self._value[1] > 0, "FFT grid components should be positive.\n"
      assert self._value[2] > 0, "FFT grid components should be positive.\n"
  value = property(_getvalue, _setvalue)

  def incar_string(self, vasp, mpicomm):
    return "NGX = %i\nNGY = %i\nNGZ = %i" % self(vasp, mpicomm)

  def __call__(self, vasp, mpicomm):
    from copy import deepcopy
    from os import getcwd
    from .. import kpoints
    from ...opt.tempdir import Tempdir
    from ..extract import Extract

    if self._value != None: return self._value

    vasp = deepcopy(vasp)
    with Tempdir(workdir=vasp._tempdir, keep=True, comm=mpicomm) as vasp._tempdir:
      vasp.kpoints = kpoints.Gamma()
      vasp.relaxation.value = None
      # may need to do some checking here... will run into infinit recursion
      # if attribute is not fftgrid. Can we access vasp.__dict__ and remove
      # reference to self?
      del vasp.fftgrid # simply remove attribute to get VASP default
      # makes sure we do nothing during this run
      vasp.nelmdl = Standard("NELMDL",  0)
      vasp.nelm   = Standard("NELM",  0)
      vasp.nelmin = Standard("NELMIN",  0)

      # Now runs vasp. OUTCAR should be in temp indir
      vasp._prerun(mpicomm)
      vasp._run(mpicomm)
      # no need for postrun

      # finally extracts from OUTCAR.
      return Extract(directory = vasp._tempdir, mpicomm = mpicomm).fft



      

class Restart(object):
  """
      Directory where to restart, or None.
      
      If None, then starts from scratch.
      If this directory contains WAVECAR or CHGCAR, then restarts from
      wavefunctions or charge. If this directory does not exist, issue an
      error.
  """
  def __init__(self, directory = None):  
    super(Restart, self).__init__()
    self._value = directory

  def __get__(self, owner, ownertype=None): return self
  def __set__(self, owner, value): self._setvalue(self, value)
  def _getvalue(self): return self._value
  def _setvalue(self, object): self._value = object
  value = property(_getvalue, _setvalue)

  def incar_string(self, vasp, mpicomm):
    istart = "0   # start from scratch"
    icharg = "2   # superpositions of atomic densities"
    if self.value == None or len(self.value) == 0:
      istart = "0   # start from scratch"
    elif not os.path.exists( self.value ):
      raise RuntimeError, "Could not find restart directory " + self.value + "\n";
    else:
      ewave = os.path.exists( os.path.join( self.value, 'WAVECAR' ) )
      echarge = os.path.exists( os.path.join( self.value, 'CHGCAR' ) )
      if ewave:
        istart = "1  # restart"
        icharg = "0   # from wavefunctions " + os.path.join( self.value, 'WAVECAR' )
      elif echarge:
        istart = "1  # restart"
        icharg = "1   # from charge " + os.path.join( self.value, 'CHGCAR' )
      else: 
        istart = "0   # start from scratch"
        icharg = "2   # superpositions of atomic densities"

    return  "ISTART = %s\nICHARG = %s" % (istart, icharg)

class Relaxation(object):
  """ Sets ISIF in incar depending on type relaxation required. 

      @param value: Specifies the type of relaxation. Defaults to None. 
        - if set to None or empty string, then no relaxation.
        - if ionic is in string, then includes ionic relaxation.
        - if cellshape is in string, then includes cell relaxation.
        - if volume is in string, then includes volume relaxation.
        Makes sure that the parameters make sense together. 
        Can also be set using an integer between 0 and 7. See VASP manual. 
      @param nsw: the number of ionic steps to perform. Defaults to 40.
      @param ibrion: the type of ionic relaxation algorithm. Defaults to 2.
      @param potim: ionic-time step during relaxation. Defaults to 0.5.
  """
  def __init__(self, value = None, nsw=40, ibrion=2, potim=0.5):
    super(Relaxation, self).__init__()
    self._value = value
    self.nsw = 40
    self.ibrion = 2
    self.potim = 0.5

  @property
  def key(self): return "ISIF"

  def _getvalue(self): return self._value
  def _setvalue(self, object):
    import re

    if object == None:
      self._value = 0
      return 

    try:
      self._value = int(object) 
      assert self._value >= 0
      return 
    except ValueError: pass

    if object.lower() == "static": 
      self._value = 0
      return 

    ionic =  re.search( "ion(ic|s)?", object.lower() ) != None
    cellshape = re.search( "cell(\s+|-|_)?(?:shape)?", object.lower() ) != None
    volume = re.search( "volume", object.lower() ) != None
    if (not ionic) and (not cellshape) and (not volume): self._value = 0
    elif ionic and (not cellshape) and (not volume):     self._value = 2
    elif ionic and cellshape and volume:                 self._value = 3
    elif ionic and cellshape and (not volume):           self._value = 4
    elif(not ionic) and  cellshape and (not volume):     self._value = 5
    elif(not ionic) and  cellshape and volume:           self._value = 6
    elif(not ionic) and (not cellshape) and volume:      self._value = 7
    elif ionic and (not cellshape) and volume:
      raise RuntimeError, "VASP does not allow relaxation of atomic position"\
                          "and volume at constant cell-shape.\n"
  value = property(_getvalue, _setvalue)

  def incar_string(self, vasp, mpicomm):
    isif = self.value
    result = "NSW    = %3i   # number of ionic steps.\n"\
             "IBRION =   %1i # ionic-relaxation minimization method.\n"\
             "POTIM  = %7.4f # ionic-relaxation step \n" % (self.nsw, self.ibrion, self.potim)
    if isif == 0: return "ISIF = 0     # static calculation " 
    elif isif == 1:       
      return result + "ISIF   = 2     # relaxing atomic positions. Only tr(strain) is correct."
    elif isif == 2:           
      return result + "ISIF   = 2     # relaxing atomic positions."
    elif isif == 3:           
      return result + "ISIF   = 3     # relaxing all structural degrees of freedom."
    elif isif == 4:           
      return result + "ISIF   = 4     # relaxing atom. pos. and cell-shape at constant V."
    elif isif == 5:           
      return result + "ISIF   = 5     # relaxing cell-shape at constant atom.-pos. and V."
    elif isif == 6:           
      return result + "ISIF   = 6     # relaxing V. and cell-shape at constant atom.-pos."
    elif isif == 7:           
      return result + "ISIF   = 7     # relaxing V. only."
    raise RuntimeError, "Internal bug."
