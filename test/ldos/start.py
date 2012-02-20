def fftmesh(cell, cutoff=20e0):
  """ FFT real-space mesh used by escan. """
  from numpy.linalg import norm
  result = int(norm(cell[:,0]) * cutoff + 5e-1), \
           int(norm(cell[:,1]) * cutoff + 5e-1), \
           int(norm(cell[:,2]) * cutoff + 5e-1)
  # This line makes sure mesh is even. Not sure why...
  result = result[0] + result[0] % 2, \
           result[1] + result[1] % 2, \
           result[2] + result[2] % 2
  return result

def create_start(path, nall = 3, nrand = 5, nmax=100, density=10e0, input='input.py'):
  """ Creates dictionary with input structures for Si/Ge. 
  
      :Parameters:
        path : str
          Path to output dictionary.
        nall : int
          All structure with ``nall`` (excluded) unit-cells are included in the final dictionary.
        nrand : int
          Structures between ``nall`` (included) and ``nrand`` (excluded) unit-cells are
          also considered for inclusion in the final dictionary. However, only
          ``nmax`` are randomly chosen in the end.
        nmax : int
          Structures between ``nall`` (included) and ``nrand`` (excluded) unit-cells are
          also considered for inclusion in the final dictionary. However, only
          ``nmax`` are randomly chosen in the end.
        density : float
          Kpoint density for escan calculations,
        input : str
          Path to input file containing escan functional.
       
      Creates a job-dictionary with a number of structures sampled from an
      exhaustive list of structures to evaluate using escan.
  """
  from random import shuffle
  from itertools import chain
  from IPython.ipapi import get as get_ipy
  from numpy.linalg import norm, inv
  from lada.enumeration import Enum
  from lada.crystal.binary import zinc_blende
  from lada.jobs import JobDict
  from lada.escan import read_input, exec_input, ReducedKDensity
  from lada.crystal.gruber import Reduction

  input = read_input(input)
  kescan = exec_input(repr(input.escan).replace('Escan', 'KEscan')).functional

  enum = Enum(zinc_blende())
  enum.sites[0].type = 'Si', 'Ge'
  enum.sites[1].type = 'Si', 'Ge'
  enum.scale = 5.45
  enum.find_space_group()
  density = density * max([1e0/norm(u) for u in inv(enum.cell * enum.scale).T])

  strs = [u for  n in range(nall, nrand) for u in enum.xn(n)]
  shuffle(strs)
  strs = [enum.as_structure(*u) for u in strs[:nmax]]
  alls = [structure for n in range(nall) for structure in enum.structures(n)]

  jobs = JobDict()
  for i, structure in enumerate(chain(alls, strs)):
    structure.name = str(i)
    nSi = len([a.type for a in structure.atoms if a.type == 'Si'])
    structure.scale = float(nSi) / float(n) * enum.scale + float(n - nSi) / float(n) * 5.69

    jobdict = jobs / structure.name
    jobdict.jobparams['structure'] = structure.copy()
    jobdict.structure.cell = Reduction()(jobdict.structure.cell)
    jobdict.functional = kescan.copy()
    jobdict.functional.kpoints = ReducedKDensity(density, (0.5, 0.5, 0.5))
    jobdict.functional.reference = None
    jobdict.functional.fft_mesh = fftmesh(structure.cell)
    jobdict.functional.nbstates = int(len(structure.atoms) * 4 * 1.5+0.5)

  ip = get_ipy()
  ip.user_ns["current_jobdict"] = jobdict.root
  ip.magic("savejobs " + path)
  return
