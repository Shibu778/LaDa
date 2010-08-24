""" Point-defect helper functions. """
__docformat__ = "restructuredtext en"


def inequivalent_sites(lattice, type):
  """ Yields sites occupied by type which are inequivalent.
  
      When creating a vacancy on, say, "O", or a substitution of "Al" by "Mg",
      there may be more than one site which qualifies. We want to iterate over
      those sites which are inequivalent only, so that only the minimum number
      of operations are performed. 
      :note: lattice sites can be defined as occupiable by more than one atomic type:
             C{lattice.site.type[i] = ["Al", "Mg"]}. These sites will be
             counted if C{type in lattice.site.type}, where type is the input
             parameter.

      :Parameters:
          lattice : `lada.crystal.Lattice`
            Lattice for which to find equivalent sites.
          type : str 
            Atomic specie for which to find inequivalent sites. 

      :return: indices of inequivalent sites.
  """
  from numpy.linalg import inv, norm
  from lada.crystal import fold_vector

  # all sites with occupation "type". 
  sites = [site for site in lattice.sites if type in site.type]
  site_indices = [i for i,site in enumerate(lattice.sites) if type in site.type]

  # inverse cell.
  invcell = inv(lattice.cell)
  # loop over all site with type occupation.
  i = 0
  while i < len(sites):
    # iterates over symmetry operations.
    for op in lattice.space_group:
      pos = op(site.pos)
      # finds index of transformed position, using translation quivalents.
      for t, other in enumerate(sites):
        if norm(fold_vector(pos, lattice.cell, invcell)) < 1e-12:
          print t
          break
      # removes equivalent site and index from lists if necessary
      if t != i and t < len(sites): 
        sites.pop(t)
        site_indices.pop(t)
    i += 1
  return site_indices

def vacancy(structure, lattice, type):
  """ Yields all inequivalent vacancies. 
  
      Loops over all symmetrically inequivalent vacancies of given type.

      :Parameters:
        structure : `lada.crystal.Structure`
          structure on which to operate
        lattice : `lada.crystal.Lattice` 
          back-bone lattice of the structure.
        type : str
          type of atoms for which to create vacancy.

      :return: 
        a 3-tuple consisting of:
        - the structure with a vacancy.
        - the vacancy atom from the original structure. It is given an
          additional attribute, C{index}, referring to list of atoms of the
          original structure.
        - A suggested name for the vacancy: site_i, where i is the site index
          of the vacancy (in the lattice).
  """
  from copy import deepcopy
  
  structure = deepcopy(structure)
  inequivs = inequivalent_sites(lattice, type)
  for i in inequivs:

    # finds first qualifying atom
    for which, atom in enumerate(structure.atoms):
      if atom.site == i: break

    assert which < len(structure.atoms), RuntimeError("Site index not found.")

    # name of this vacancy
    name = "vacancy_{0}".format(type)
    if len(inequivs) > 1: name += "/site_{0}".format(i)
    # creates vacancy and keeps atom for record
    atom = deepcopy(structure.atoms.pop(which))
    atom.index = which
    # structure 
    result = deepcopy(structure)
    result.name = name
    # returns structure with vacancy.
    yield result, atom
    # removes vacancy
    structure.atoms.insert(which, atom)

def substitution(structure, lattice, type, subs):
  """ Yields all inequivalent vacancies. 
  
      Loops over all equivalent vacancies.

      :Parameters:
        structure : `lada.crystal.Structure`
          structure on which to operate
        lattice : `lada.crystal.Lattice`
          back-bone lattice of the structure.
        type : str
          type of atoms for which to create substitution.
        subs : str
          substitution type

      :return: a 3-tuple consisting of:
        - the structure with a substitution.
        - the substituted atom in the structure above. The atom is given an
          additional attribute, C{index}, referring to list of atoms in the
          structure.
        - A suggested name for the substitution: site_i, where i is the site
          index of the substitution.
  """
  from copy import deepcopy

  # Case for vacancies.
  if subs == None: 
    for args in vacancy(structure, lattice, type):
      yield args
    return

  
  result = deepcopy(structure)
  inequivs = inequivalent_sites(lattice, type)
  for i in inequivs:

    # finds first qualifying atom
    for which, atom in enumerate(structure.atoms):
      if atom.site == i: break

    assert which < len(structure.atoms), RuntimeError("Site index not found.")

    # name of this substitution
    name = "{0}_on_{1}".format(subs, type) 
    if len(inequivs) > 1: name += "/site_{0}".format(i)
    # creates substitution
    orig = result.atoms[which].type
    result.atoms[which].type = subs
    # substituted atom.
    substituted = deepcopy(result.atoms[which])
    substituted.index = which
    result.name = name
    # returns structure with vacancy.
    yield result, substituted
    # removes substitution
    result.atoms[which].type = orig

def charged_states(species, A=None, B=None):
  """ Loops over charged systems. 

      :Types:
        - species: `lada.vasp.specie.Specie`
        - `A` : None or str index to `species`.
        - `B` : None or str index to `species`.

      - if only one of C{A} and C{B} is not None, then the accepted charge
        states are anything between 0 and C{-A.oxidation} included. This
        works both for negative and positive oxidation numbers. The "-" sign
        comes from the rational that an ion C{A} with oxidation
        C{A.oxidation} is removed from the system.
      - if both C{A} and C{B} are not None, than reviewed chared states are between
        C{max(-A.oxidation, B.oxidation-A.oxidation)} and
        C{min(-A.oxidation, B.oxidation-A.oxidation)}. 

      :return: Yields a 2-tuple:
        - Number of electrons to add to the system (not charge). 
        - a suggested name for the charge state calculation.
  """

  if A == None: A, B = B, A
  if A == None: # no oxidation
    yield None, "neutral"
    return
  else: A = species[A]
  if B == None:
    # max/min oxidation state
    max_charge = -A.oxidation if hasattr(A, "oxidation") else 0
    min_charge = 0
    if max_charge < min_charge: max_charge, min_charge = min_charge, max_charge
  else:
    B = species[B]
    # Finds maximum range of oxidation.
    maxA_oxidation = A.oxidation if hasattr(A, "oxidation") else 0
    maxB_oxidation = B.oxidation if hasattr(B, "oxidation") else 0
    max_charge = max(-maxA_oxidation, maxB_oxidation - maxA_oxidation, 0)
    min_charge = min(-maxA_oxidation, maxB_oxidation - maxA_oxidation, 0)

  for charge in range(min_charge, max_charge+1):
    # directory
    if   charge == 0:   oxdir = "charge_neutral"
    elif charge > 0:    oxdir = "charge_" + str(charge) 
    elif charge < 0:    oxdir = "charge_" + str(charge) 
    yield -charge, oxdir


def band_filling_correction(defect, cbm):
  """ Returns band-filling corrrection. 

      :Parameters: 
        defect : return of `lada.vasp.Vasp.__call__`
          An output extraction object as returned by the vasp functional when
          computing the defect of interest.
        cbm : float 
          The cbm of the host with potential alignment.
  """
  from numpy import sum
  indices = defect.eigenvalues > cbm
  return sum( (defect.multiplicity * defect.eigenvalues * defect.occupations)[indices] - cbm )
  

def potential_alignment(defect, host, maxdiff=0.5):
  """ Returns potential alignment correction. 

      :Parameter:
        defect : return of `lada.vasp.Vasp.__call__`
          An output extraction object as returned by the vasp functional when
          computing the defect of interest.
        host : return of `lada.vasp.Vasp.__call__`
          An output extraction object as returned by the vasp functional when
          computing the host matrix.
        maxdiff : float
          Maximum difference between the electrostatic potential of an atom and
          the equivalent host electrostatic potential beyond which that atom is
          considered pertubed by the defect.

      :return: The potential alignment in eV (without charge factor).
  """
  from operator import itemgetter
  from numpy import average, array, abs
  from crystal import specie_list

  # first get average atomic potential per atomic specie in host.
  host_electropot = {}
  host_species = specie_list(host.structure)
  for s in host_species:
    indices = array([atom.type == s for atom in structure.atoms])
    host_electropot[s] = average(host.electropot[indices]) 
  
  # creates an initial list of unperturbed atoms. 
  types = array([atom.type for atom in defect.structure.atoms])
  unperturbed = array([(type in host_species) for type in types])

  # now finds unpertubed atoms according to maxdiff
  while any(unperturbed): # Will loop until no atom is left. That shouldn't happen!

    # computes average atomic potentials of unperturbed atoms.
    defect_electropot = {}
    for s in host_species:
      defect_electropot[s] = average( defect.electropot[unperturbed & (types == s)] )

    # finds atomic potential farthest from the average potentials computed above.
    discrepancies =   defect.electropot[unperturbed] \
                    - array([host_electropot[type] for type in types[unperturbed]])
    index, diff = max(enumerate(abs(discrepancies)), key=itemgetter(2))

    # if discrepancy too high, mark atom as perturbed.
    if diff > maxdiff: unperturbed[unperturbed][index] = False
    # otherwise, we are done for this loop!
    else: break

  # now computes potential alignment from unpertubed atoms.
  diff_from_host =    defect.electropot[unpertubed]  \
                    - array([host_electropot[type] for type in types[unperturbed]])
  return average(diff_from_host)
                    

def third_order_charge_correction(cell, n = 200):
  """ Returns energy of third order charge correction. 
  
      :Parameters: 
        cell : 3x3 numpy array
          The supercell of the defect in Angstroem(?).
        n 
          precision. Higher better.
      :return: energy of a single negative charge in supercell.
  """
  from math import pi
  from numpy import array, dot, det
  from ..physics import Rydberg

  def fold(vector):
    """ Returns smallest distance. """
    result = None
    for i in range(-1, 2):
      for j in range(-1, 2):
        for k in range(-1, 2):
          v = arrray([vector[0] + float(i), vector[1] + float(j), vector[2] + float(k)])
          v = dot(cell, v)
          m = dot(v,v)
          if result == None or result > m: result = m
    return result

  # chden = ones( (n, n, n), dtype="float64")
  result = 0e0
  for ix in range(n):
    for iy in range(n):
      for iz in range(n):
        vec = array([float(ix)/float(n)-0.5, float(iy)/float(n)-0.5, float(iz)/float(n)-0.5])
        result += fold(vec)
        
  return -result / float(n**3) * Rydberg("eV") * pi * 4e0 / 3e0 / det(cell)


def first_order_charge_correction(cell, charge=1e0, cutoff=None):
  """ First order charge correction of +1 charge in given supercell.
  
      :Parameters:
        - `cell`: Supercell of the point-defect.
        - `charge`: Charge of the point-defect.
        - `cutoff`: Ewald cutoff parameter.
  """
  from numpy.linalg import norm
  from ..crystal import Structure
  try: from ..pcm import Clj 
  except ImportError as e:
    print "Could not import Point-Charge Model package (pcm). \n"\
          "Cannot compute first order charge correction.\n"\
          "Please compile LaDa with pcm enabled.\n"
    raise

  clj = Clj()
  clj.charge["A"] = charge
  if cutoff == None:
    clj.ewald = 10 * max( [norm(cell[:,i]) for i in range(3)] )
  else: clj.ewald_cutoff = cutoff

  structure = Structure(cell)
  structure.add_atom = ((0e0,0,0), "A")

  return clj.ewald(structure)

def magnetic_neighborhood(structure, defect, species):
   """ Finds magnetic neighberhood of a defect.
   
       If the defect is a substitution with a magnetic atom, then the
       neighberhood is the defect alone. Otherwise, the neighberhood extends to
       magnetic first neighbors. An atomic specie is deemed magnetic if marked
       as such in `species`.

       :Parameters: 
         structure : `lada.crystal.Structure`
           The structure with the point-defect already incorporated.
         defect : `lada.crystal.Atom`
           The point defect, to which and *index* attribute is given denoting
           the index of the atom in the original supercell structure (without
           point-defect).
         species : dict of `lada.vasp.species.Specie`
           A dictionary defining the atomic species.
       :return: indices of the neighboring atoms in the point-defect `structure`.
   """
   from numpy import array
   from numpy.linalg import norm
   from . import Neighbors

   # checks if substitution with a magnetic defect.
   if defect.index < len(structure.atoms):
     atom = structure.atoms[defect.index]
     if species[atom.type].magnetic and norm(defect.pos - atom.pos) < 1e-12:
       return [defect.index]
   # now finds first neighbors. 12 is the highest coordination number, so
   # this should include the first shell.
   neighbors = [n for n in Neighbors(structure, 12, defect.pos)]
   # only take the first shell and keep indices (to atom in structure) only.
   neighbors = [n.index for n in neighbors if n.distance < neighbors[0].distance + 1e-1]
   # only keep the magnetic neighborhood.
   return [n for n in neighbors if species[structure.atoms[n].type].magnetic]

def equiv_bins(n, N):
  """ Generator over ways to fill N equivalent bins with n equivalent balls. """
  from itertools import chain
  from numpy import array
  assert N > 0
  if N == 1: yield [n]; return
  if n == 0: yield [0 for x in range(N)]
  for u in xrange(n, 0, -1):
    for f in  equiv_bins(n-u, N-1):
      result = array([x for x in chain([u], f)])
      if all(result[0:-1]-result[1:] >= 0): yield result

def inequiv_bins(n, N):
  """ Generator over ways to fill N inequivalent bins with n equivalent balls. """
  from itertools import permutations
  for u in equiv_bins(n, N):
    u = [v for v in u]
    history = []
    for perm in permutations(u, len(u)):
      seen = False
      for other in history:
        same = not any( p != o for p, o in zip(perm, other) )
        if same: seen = True; break
      if not seen: history.append(perm); yield [x for x in perm]

def electron_bins(n, atomic_types):
  """ Loops over electron bins. """
  from itertools import product 
  from numpy import zeros, array
  # creates a dictionary where every type is associated with a list of indices into atomic_types.
  Ns = {}
  for type in set(atomic_types):
    Ns[type] = [i for i,u in enumerate(atomic_types) if u == type]
  # Distributes electrons over inequivalent atomic types.
  for over_types in inequiv_bins(n, len(Ns.keys())):
    # now distributes electrons over each type independently.
    iterables = [ equiv_bins(v, len(Ns[type])) for v, type in zip(over_types, Ns.keys()) ] 
    for nelecs in product(*iterables):
      # creates a vector where indices run as in atomic_types argument.
      result = zeros((len(atomic_types),), dtype="float64")
      for v, (type, value) in zip(nelecs, Ns.items()): result[value] = array(v)
      yield result

def magmom(indices, moments, nbatoms):
  """ Yields a magmom string from knowledge of which moments are non-zero. """
  from operator import itemgetter
  s = [0 for i in range(nbatoms)]
  for i, m in zip(indices, moments): s[i] = m
  compact = [[1, s[0]]]
  for m in s[1:]:
    if abs(compact[-1][1] - m) < 1e-12: compact[-1][0] += 1
    else: compact.append( [1, m] )
    
  string = ""
  for n, m in compact:
    if n > 1: string +=" {0}*{1}".format(n, m)
    elif n == 1: string += " {0}".format(m)
    assert n != 0
  return string

def electron_counting(structure, defect, species, extrae):
  """ Enumerate though number of electron in point-defect magnetic neighberhood. 

      Generator over the number of electrons of each atom in the magnetic
      neighberhood of a point defect with `extrae` electrons. If there are no
      magnetic neighborhood, then `magmom` is set
      to None and the total magnetic moment to 0 (e.g. lets VASP figure it out).
      Performs a sanity check on integers to make sure things are correct.
      :Parameters:
        structure : `lada.crystal.Structure`
          Structure with point-defect already inserted.
        defect : `lada.crystal.Atom`
          Atom making up the point-defect.
          In addition, it should have an *index* attribute denoting the defect 
        species : dict of `lada.vasp.species.Specie`
          Dictionary containing details of the atomic species.
        extrae :
          Number of extra electrons to add/remove.
      :return: yields (indices, electrons) where indices is a list of indices
        to the atom in the neighberhood, and electrons is a corresponding list of
        elctrons.
  """
  from numpy import array
  from ..physics import Z
  indices = magnetic_neighborhood(structure, defect, species)

  # no magnetic neighborhood.
  if len(indices) == 0: 
    yield None, None
    return

  # has magnetic neighberhood from here on.
  atoms = [structure.atoms[i] for i in indices]
  types = [a.type for a in atoms]
  nelecs = array([species[type].valence - species[type].oxidation for type in types])

  # loop over different electron distributions.
  for tote in electron_bins(abs(extrae), types):
    # total number of electrons on each atom.
    if extrae < 0:   tote = nelecs - tote
    elif extrae > 0: tote += nelecs

    # sanity check. There may be more electrons than orbitals at this point.
    sane = True
    for n, type in zip(tote, types):
      if n < 0: sane = False; break;
      z = Z(type)
      if (z >= 21 and z <= 30) or (z >= 39 and z <= 48) or (z >= 57 and z <= 80):  
        if n > 10: sane = False;  break
      elif n > 8: sane = False; break

    if not sane: continue

    yield indices, tote
 

def low_spin_states(structure, defect, species, extrae, do_integer=True, do_average=True):
  """ Enumerate though low-spin-states in point-defect. 

      Generator over low-spin magnetic states of a defect with
      `extrae` electrons. The extra electrons are distributed both as integers
      and as an average. All these states are ferromagnetic. In the special
      case of a substitution with a magnetic atom, the moment is expected to go
      on the substitution alone. If there are no magnetic neighborhood, then `magmom` is set
      to None and the total magnetic moment to 0 (e.g. lets VASP figure it out).
      :Parameters:
        structure : `lada.crystal.Structure`
          Structure with point-defect already inserted.
        defect : `lada.crystal.Atom`
          Atom making up the point-defect.
          In addition, it should have an *index* attribute denoting the defect 
        species : dict of `lada.vasp.species.Specie`
          Dictionary containing details of the atomic species.
        extrae :
          Number of extra electrons to add/remove.
      :return: yields (indices, moments) where the former index the relevant
               atoms in `structure` and latter are their respective moments.
  """

  from numpy import array, abs, all, any

  history = []
  def check_history(*args):
    for i, t in history:
      if all(abs(i-args[0]) < 1e-12) and all(abs(t-args[1]) < 1e-12):
        return False
    history.append(args)
    return True


  if do_integer: 
    for indices, tote in electron_counting(structure, defect, species, extrae):
      if tote == None: continue # non-magnetic case
      indices, moments = array(indices), array(tote) % 2
      if all(abs(moments) < 1e-12): continue # non - magnetic case
      if check_history(indices, moments): yield indices, moments
  if do_average: 
    for indices, tote in electron_counting(structure, defect, species, 0):
      if tote == None: continue # non-magnetic case
      if len(indices) < 2: continue
      indices, moments = array(indices), array(tote) % 2 + extrae / float(len(tote))
      if all(abs(moments) < 1e-12): continue # non - magnetic case
      if check_history(indices, moments): yield indices, moments


def high_spin_states(structure, defect, species, extrae, do_integer=True, do_average=True):
  """ Enumerate though high-spin-states in point-defect. 

      Generator over high-spin magnetic states of a defect with
      `extrae` electrons. The extra electrons are distributed both as integers
      and as an average. All these states are ferromagnetic. In the special
      case of a substitution with a magnetic atom, the moment is expected to go
      n the substitution alone. If there are no magnetic neighborhood, then
      `magmom` is set to None and the total magnetic moment to 0 (e.g. lets
      VASP figure it out).
      :Parameters:
        structure : `lada.crystal.Structure`
          Structure with point-defect already inserted.
        defect : `lada.crystal.Atom`
          Atom making up the point-defect.
          In addition, it should have an *index* attribute denoting the defect 
        species : dict of `lada.vasp.species.Specie`
          Dictionary containing details of the atomic species.
        extrae :
          Number of extra electrons to add/remove.
      :return: yields (indices, moments) where the former index the relevant
               atoms in `structure` and latter are their respective moments.
  """
  from numpy import array, abs, all, any

  def is_d(t): 
    """ Determines whether an atomic specie is transition metal. """
    from ..physics import Z
    z = Z(t)
    return (z >= 21 and z <= 30) or (z >= 39 and z <= 48) or (z >= 57 and z <= 80) 

  def determine_moments(arg, ts): 
    """ Computes spin state from number of electrons. """
    f = lambda n, t: (n if n < 6 else 10-n) if is_d(t) else (n if n < 5 else 8-n)
    return [f(n,t) for n, t in zip(arg, ts)]

  history = []
  def check_history(*args):
    for i, t in history:
      if all(abs(i-args[0]) < 1e-12) and all(abs(t-args[1]) < 1e-12):
        return False
    history.append(args)
    return True
  
  if do_integer: 
    for indices, tote in electron_counting(structure, defect, species, extrae):
      if tote == None: continue # non-magnetic case
      
      types = [structure.atoms[i].type for i in indices]
      indices, moments = array(indices), array(determine_moments(tote, types))
      if all(moments == 0): continue # non - magnetic case
      if check_history(indices, moments):  yield indices, moments

  if do_average: 
    for indices, tote in electron_counting(structure, defect, species, 0):
      if tote == None: continue # non-magnetic case
      if len(indices) < 2: continue

      types = [structure.atoms[i].type for i in indices]
      indices = array(indices)
      moments = array(determine_moments(tote, types)) + float(extrae) / float(len(types))
      if all(abs(moments) < 1e-12): continue # non - magnetic case
      if check_history(indices, moments):  yield indices, moments

def magname(moments, prefix=None, suffix=None):
  """ Construct name for magnetic moments. """
  if len(moments) == 0: return "paramagnetic"
  string = str(moments[0])
  for m in moments[1:]: string += "_" + str(m)
  if prefix != None: string = prefix + "_" + string
  if suffix != None: string += "_" + suffix
  return string

