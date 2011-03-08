""" Subpackage containing extraction methods for VASP common to both GW and DFT. """
__docformat__  = 'restructuredtext en'
__all__ = ['Extract']
from ...opt.decorators import make_cached, broadcast_result

class Extract(object):
  """ Implementation class for extracting data from VASP output """

  def __init__(self):
    """ Initializes the extraction class. """
    object.__init__(self)
    
  @property
  def exports(self):
    """ List of files to export. """
    from os.path import join, exists
    return [ join(self.directory, u) for u in [self.OUTCAR, self.FUNCCAR] \
             if exists(join(self.directory, u)) ]

  @property 
  @make_cached
  @broadcast_result(attr=True, which=0)
  def algo(self):
    """ Returns the kind of algorithms. """
    result = self._find_first_OUTCAR(r"""^\s*ALGO\s*=\s*(\S+)\s*""")
    if result == None: return 'Normal'
    return result.group(1).lower()

  @property
  def is_dft(self):
    """ True if this is a DFT calculation, as opposed to GW. """
    return self.algo not in ['gw', 'gw0', 'chi', 'scgw', 'scgw0'] 
  @property
  def is_gw(self):
    """ True if this is a GW calculation, as opposed to DFT. """
    return self.algo in ['gw', 'gw0', 'chi', 'scgw', 'scgw0'] 
    

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def functional(self):
    """ Returns vasp functional used for calculation.

        Requires file FUNCCAR to be present.
    """
    from cPickle import load
    with self.__funccar__() as file: return load(file)

  @property
  def vasp(self):
    """ Deprecated. """
    from warnings import warn
    warn(DeprecationWarning('vasp attribute is deprecated in favor of functional.'), stacklevel=2)
    return self.functional

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def success(self):
    """ Checks that VASP run has completed. 

        At this point, checks for the existence of OUTCAR.
        Then checks that timing stuff is present at end of OUTCAR.
    """
    from os.path import exists, join
    import re

    for path in [self.OUTCAR]:
      if not exists(join(self.directory, path)): return False
      
    regex = r"""General\s+timing\s+and\s+accounting\s+informations\s+for\s+this\s+job"""
    return self._find_last_OUTCAR(regex) != None


  @broadcast_result(attr=True, which=0)
  def _starting_structure_data(self):
    """ Structure at start of calculations. """
    from re import compile
    from numpy import array, zeros, dot

    species_in = self.species

    cell = zeros((3,3), dtype="float64")
    atoms = []

    with self.__outcar__() as file: 
      atom_index, cell_index = None, None
      cell_re = compile(r"""^\s*direct\s+lattice\s+vectors\s+""")
      atom_re = compile(r"""^\s*position\s+of\s+ions\s+in\s+fractional\s+coordinates""")
      for line in file:
        if cell_re.search(line) != None: break
      for i in range(3):
        cell[:,i] = array(file.next().split()[:3], dtype='float64')
      for line in file:
        if atom_re.search(line) != None: break
      for line in file:
        data = line.split()
        if len(data) != 3: break
        atoms.append(dot(cell, array(data, dtype='float64')))

    return cell, atoms

  @property
  @make_cached
  def starting_structure(self):
    """ Structure at start of calculations. """
    from ...crystal import Structure
    from quantities import eV
    cell, atoms = self._starting_structure_data()
    structure = Structure()
    # tries to find adequate name for structure.
    try: name = self.system
    except RuntimeError: name = ''
    structure.name = self.name
    if len(name) == 0 or name == 'POSCAR created by SUPERPOSCAR':
      try: title = self.system
      except RuntimeError: title = ''
      if len(title) != 0: structure.name = title

    structure.energy = 0e0
    structure.cell = cell
    structure.scale = 1e0
    assert len(self.species) == len(self.ions_per_specie),\
           RuntimeError("Number of species and of ions per specie incoherent.")
    assert len(atoms) == sum(self.ions_per_specie),\
           RuntimeError('Number of atoms per specie does not sum to number of atoms.')
    for specie, n in zip(self.species,self.ions_per_specie):
      for i in range(n): structure.add_atom = atoms.pop(0), specie

    if (self.isif == 0 or self.nsw == 0 or self.ibrion == -1) and self.is_dft:
      structure.energy = float(self.total_energy.rescale(eV))
    return structure

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def _structure_data(self):
    """ Greps cell and positions from OUTCAR. """
    from re import compile
    from numpy import array, zeros

    species_in = self.species

    cell = zeros((3,3), dtype="float64")
    atoms = []

    with self.__outcar__() as file: lines = file.readlines()

    atom_index, cell_index = None, None
    atom_re = compile(r"""^\s*POSITION\s+""")
    cell_re = compile(r"""^\s*direct\s+lattice\s+vectors\s+""")
    for index, line in enumerate(lines[::-1]):
      if atom_re.search(line) != None: atom_index = index - 1
      if cell_re.search(line) != None: cell_index = index; break
    assert atom_index != None and cell_index != None,\
           RuntimeError("Could not find structure description in OUTCAR.")
    for i in range(3):
      cell[:,i] = [float(u) for u in lines[-cell_index+i].split()[:3]]
    while atom_index > 0 and len(lines[-atom_index].split()) == 6:
      atoms.append( array([float(u) for u in lines[-atom_index].split()[:3]], dtype="float64") )
      atom_index -= 1

    return cell, atoms

  @property
  @make_cached
  def structure(self):
    """ Greps structure and total energy from OUTCAR. """
    from re import compile
    from numpy import array, zeros
    from quantities import eV
    from ...crystal import Structure

    if self.isif == 0 or self.nsw == 0 or self.ibrion == -1:
      return self.starting_structure


    species_in = self.species
    try: cell, atoms = self._structure_data
    except: return self.contcar_structure

    structure = Structure()
    # tries to find adequate name for structure.
    try: name = self.system
    except RuntimeError: name = ''
    structure.name = self.name
    if len(name) == 0 or name == 'POSCAR created by SUPERPOSCAR':
      try: title = self.system
      except RuntimeError: title = ''
      if len(title) != 0: structure.name = title
    
    structure.energy = float(self.total_energy.rescale(eV)) if self.is_dft else 0e0
    structure.cell = array(cell, dtype="float64")
    structure.scale = 1e0
    assert len(self.species) == len(self.ions_per_specie),\
           RuntimeError("Number of species and of ions per specie incoherent.")
    for specie, n in zip(self.species,self.ions_per_specie):
      for i in range(n):
        structure.add_atom = array(atoms.pop(0), dtype="float64"), specie

    return structure

  @property
  @make_cached
  def contcar_structure(self):
    """ Greps structure from CONTCAR. """
    from os.path import exists, join
    from ...crystal import read_poscar
    from quantities import eV

    species_in = self.species

    result = read_poscar(species_in, self.__contcar__(), comm=self.comm)
    structure.energy = float(self.total_energy.rescale(eV)) if self.is_dft else 0e0
    return result

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def ions_per_specie(self):
    """ Greps species from OUTCAR. """
    result = self._find_first_OUTCAR(r"""\s*ions\s+per\s+type\s*=.*$""")
    if result == None: return None
    return [int(u) for u in result.group(0).split()[4:]]

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def species(self):
    """ Greps species from OUTCAR. """
    return tuple([ u.group(1) for u in self._search_OUTCAR(r"""VRHFIN\s*=\s*(\S+)\s*:""") ])

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def isif(self):
    """ Greps ISIF from OUTCAR. """
    result = self._find_first_OUTCAR(r"""\s*ISIF\s*=\s*(-?\d+)\s+""")
    if result == None: return None
    return int(result.group(1))
  
  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def nsw(self):
    """ Greps NSW from OUTCAR. """
    result = self._find_first_OUTCAR(r"""\s*NSW\s*=\s*(-?\d+)\s+""")
    if result == None: return None
    return int(result.group(1))

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def ibrion(self):
    """ Greps IBRION from OUTCAR. """
    result = self._find_first_OUTCAR(r"""\s*IBRION\s*=\s*(-?\d+)\s+""")
    if result == None: return None
    return int(result.group(1))

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def kpoints(self):
    """ Greps k-points from OUTCAR.
    
        Numpy array where each row is a k-vector in cartesian units. 
    """
    from os.path import exists, join
    from re import compile, search 
    from numpy import array

    result = []
    with self.__outcar__() as file:
      found = compile(r"""Found\s+(\d+)\s+irreducible\s+k-points""")
      for line in file:
        if found.search(line) != None: break
      found = compile(r"""Following\s+cartesian\s+coordinates:""")
      for line in file:
        if found.search(line) != None: break
      file.next()
      for line in file:
        data = line.split()
        if len(data) != 4: break;
        result.append( data[:3] )
    return array(result, dtype="float64") 

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def multiplicity(self):
    """ Greps multiplicity of each k-point from OUTCAR. """
    from os.path import exists, join
    from re import compile, search 
    from numpy import array

    result = []
    with self.__outcar__() as file:
      found = compile(r"""Found\s+(\d+)\s+irreducible\s+k-points""")
      for line in file:
        if found.search(line) != None: break
      found = compile(r"""Following\s+cartesian\s+coordinates:""")
      for line in file:
        if found.search(line) != None: break
      file.next()
      for line in file:
        data = line.split()
        if len(data) != 4: break;
        result.append( float(data[3]) )
    return array(result, dtype="float64")

  @property 
  @make_cached
  @broadcast_result(attr=True, which=0)
  def ispin(self):
    """ Greps ISPIN from OUTCAR. """
    result = self._find_first_OUTCAR(r"""^\s*ISPIN\s*=\s*(1|2)\s+""")
    assert result != None, RuntimeError("Could not extract ISPIN from OUTCAR.")
    return int(result.group(1))

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def name(self):
    """ Greps POSCAR title from OUTCAR. """
    result = self._find_first_OUTCAR(r"""^\s*POSCAR\s*=.*$""")
    assert result != None, RuntimeError("Could not extract POSCAR title from OUTCAR.")
    result = result.group(0)
    result = result[result.index('=')+1:]
    return result.rstrip().lstrip()

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def system(self):
    """ Greps system title from OUTCAR. """
    result = self._find_first_OUTCAR(r"""^\s*SYSTEM\s*=.*$""")
    assert result != None, RuntimeError("Could not extract SYSTEM title from OUTCAR.")
    result = result.group(0)
    result = result[result.index('=')+1:]
    return result.rstrip().lstrip()

  @broadcast_result(attr=True, which=0)
  def _unpolarized_values(self, which):
    """ Returns spin-unpolarized eigenvalues and occupations. """
    from re import compile, search, finditer
    import re
    from os.path import exists, join
    from numpy import array

    with self.__outcar__() as file: lines = file.readlines()
    # Finds last first kpoint.
    spin_comp1_re = compile(r"\s*k-point\s+1\s*:\s*(\S+)\s+(\S+)\s+(\S+)\s*")
    found = None
    for i, line in enumerate(lines[::-1]):
      found = spin_comp1_re.match(line)
      if found != None: break
    assert found != None, RuntimeError("Could not extract eigenvalues/occupation from OUTCAR.")

    # now greps actual results.
    if self.is_dft:
      kp_re = r"\s*k-point\s+(?:\d+)\s*:\s*(?:\S+)\s*(?:\S+)\s*(?:\S+)\n"\
              r"\s*band\s+No\.\s+band\s+energies\s+occupation\s*\n"\
              r"(\s*(?:\d+)\s+(?:\S+)\s+(?:\S+)\s*\n)+"
      skip, cols = 2, 3
    else: 
      kp_re = r"\s*k-point\s+(?:\d+)\s*:\s*(?:\S+)\s*(?:\S+)\s*(?:\S+)\n"\
              r"\s*band\s+No\.\s+.*\n\n"\
              r"(\s*(?:\d+)\s+(?:\S+)\s+(?:\S+)\s+(?:\S+)\s+(?:\S+)"\
              r"\s+(?:\S+)\s+(?:\S+)\s+(?:\S+)\s*\n)+"
      skip, cols = 3, 8
    results = []
    for kp in finditer(kp_re, "".join(lines[-i-1:]), re.M):
      dummy = [u.split() for u in kp.group(0).split('\n')[skip:]]
      results.append([float(u[which]) for u in dummy if len(u) == cols])
    return results

  @broadcast_result(attr=True, which=0)
  def _spin_polarized_values(self, which):
    """ Returns spin-polarized eigenvalues and occupations. """
    from re import compile, search, finditer
    import re
    from os.path import exists, join
    from numpy import array

    with self.__outcar__() as file: lines = file.readlines()
    # Finds last spin components.
    spin_comp1_re = compile(r"""\s*spin\s+component\s+(1|2)\s*$""")
    spins = [None,None]
    for i, line in enumerate(lines[::-1]):
      found = spin_comp1_re.match(line)
      if found == None: continue
      if found.group(1) == '1': 
        assert spins[1] != None, \
               RuntimeError("Could not find two spin components in OUTCAR.")
        spins[0] = i
        break
      else:  spins[1] = i
    assert spins[0] != None and spins[1] != None,\
           RuntimeError("Could not extract eigenvalues/occupation from OUTCAR.")

    # now greps actual results.
    if self.is_dft:
      kp_re = r"\s*k-point\s+(?:\d+)\s*:\s*(?:\S+)\s*(?:\S+)\s*(?:\S+)\n"\
              r"\s*band\s+No\.\s+band\s+energies\s+occupation\s*\n"\
              r"(\s*(?:\d+)\s+(?:\S+)\s+(?:\S+)\s*\n)+"
      skip, cols = 2, 3
    else: 
      kp_re = r"\s*k-point\s+(?:\d+)\s*:\s*(?:\S+)\s*(?:\S+)\s*(?:\S+)\n"\
              r"\s*band\s+No\.\s+.*\n\n"\
              r"(\s*(?:\d+)\s+(?:\S+)\s+(?:\S+)\s+(?:\S+)\s+(?:\S+)"\
              r"\s+(?:\S+)\s+(?:\S+)\s+(?:\S+)\s*\n)+"
      skip, cols = 3, 8
    results = [ [], [] ]
    for kp in finditer(kp_re, "".join(lines[-spins[0]:-spins[1]]), re.M):
      dummy = [u.split() for u in kp.group(0).split('\n')[skip:]]
      results[0].append([float(u[which]) for u in dummy if len(u) == cols])
    for kp in finditer(kp_re, "".join(lines[-spins[1]:]), re.M):
      dummy = [u.split() for u in kp.group(0).split('\n')[skip:]]
      results[1].append([u[which] for u in dummy if len(u) == cols])
    return results

  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def ionic_charges(self):
    """ Greps ionic_charges from OUTCAR."""
    from numpy import array
    regex = """^\s*ZVAL\s*=\s*(.*)$"""
    result = self._find_last_OUTCAR(regex) 
    assert result != None, RuntimeError("Could not find ionic_charges in OUTCAR")
    return array([float(u) for u in result.group(1).split()])

  @property
  @make_cached
  def valence(self):
    """ Greps total energy from OUTCAR."""
    from numpy import array
    ionic = self.ionic_charges
    species = self.species
    atoms = [u.type for u in self.structure.atoms]
    result = 0
    for c, s in zip(ionic, species): result += c * atoms.count(s)
    return result
  
  @property
  @make_cached
  @broadcast_result(attr=True, which=0)
  def nelect(self):
    """ Greps nelect from OUTCAR."""
    regex = """^\s*NELECT\s*=\s*(\S+)\s+total\s+number\s+of\s+electrons\s*$"""
    result = self._find_last_OUTCAR(regex) 
    assert result != None, RuntimeError("Could not find energy in OUTCAR")
    return float(result.group(1)) 

  @property
  @make_cached
  def charge(self):
    """ Greps total charge in the system from OUTCAR."""
    return self.valence-self.nelect