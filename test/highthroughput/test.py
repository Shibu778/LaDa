#!/usr/bin/env python
""" High-Thoughput of A2BO4 structures. """

input = _input()
""" All input parameteres. """
mppalloc = input.mppalloc
""" Process allocation scheme. """

def _waves(is_first = True): 
  from re import compile

  from lada import jobs
  from lada.opt import read_input
  from lada.opt.changedir import Changedir
  from lada.crystal import A2BX4, fill_structure
  from lada.vasp import Vasp, Specie, specie, files, Extract
  from lada.vasp.methods import RelaxCellShape
  from lada.vasp import files

  import magnetic

  # list of all lattices in A2BX4.
  lattices = [ A2BX4.S1(), A2BX4.S1I(), A2BX4.S2(), A2BX4.S3(), A2BX4.S3I(),
               A2BX4.b1(), A2BX4.b10(), A2BX4.b10I(), A2BX4.b11(), A2BX4.b12(),
               A2BX4.b15(), A2BX4.b16(), A2BX4.b18(), A2BX4.b19(), A2BX4.b1I(),
               A2BX4.b2(), A2BX4.b20(), A2BX4.b21(), A2BX4.b2I(), A2BX4.b33(),
               A2BX4.b36(), A2BX4.b37(), A2BX4.b38(), A2BX4.b4(), A2BX4.b4I(), 
               A2BX4.b5(), A2BX4.b5I(), A2BX4.b8(), A2BX4.b9(), A2BX4.b9I(), 
               A2BX4.d1(), A2BX4.d1I(), A2BX4.d3(), A2BX4.d3I(), A2BX4.d9() ]

  # regex
  specie_regex = compile("([A-Z][a-z]?)2([A-Z][a-z]?)([A-Z][a-z]?)4")

  # Job dictionary.
  jobdict = jobs.JobDict()

  # loop over materials.
  for material in input.materials:

    # creates dictionary to replace A2BX4 with meaningfull species.
    match = specie_regex.match(material)
    assert match != None, RuntimeError("Incorrect material " + material + ".")
    # checks species are known to vasp functional
    for i in range(1, 4):
      assert match.group(i) in input.vasp.species,\
             RuntimeError("%s not in specie dictionary." % (match.group(i)))
    # actually creates dictionary.
    species_dict = {"A": match.group(1), "B": match.group(2), "X": match.group(3)}

    # loop over lattices. 
    for lattice in lattices:

      # creates a structure.
      structure = fill_structure(lattice.cell, lattice)
      structure.name = material + ": " + lattice.name
      # changes atomic types.
      for atom in structure.atoms:
        atom.type  = species_dict[atom.type]
      # set volume
      structure.scale = input.volume(structure)

      # job dictionary for this lattice.
      lat_jobdict = jobdict / material / lattice.name / "non-magnetic"

      # sets up job parameters.
      if is_first: 
        lat_jobdict.vasp = input.relaxer
        lat_jobdict.args = [structure]
        lat_jobdict.jobparams["ispin"] = 1
        lat_jobdict.jobparams["repat"] = files.input
        continue # first wave.

      # goes through magnetic stuff
      if not magnetic.is_magnetic_system(structure, input.vasp.species): continue

      # first ferro
      job = lat_jobdict / "ferro"
      job.vasp = input.relaxer
      job.args = [structure]
      job.jobparams["ispin"] =  2
      job.jobparams["magmom"] = magnetic.ferro(structure, input.vasp.species)
      job.jobparams["repat"] = files.input
      job.restart = Extract, "../non-magnetic"
      
      # then, antiferro with spin direction depending on cation type.
      magmom = magnetic.sublatt_antiferro(structure, input.vasp.species) 
      if magmom != None:
        job = lat_jobdict / "anti-ferro-0"
        job.vasp = input.relaxer
        job.args = [structure]
        job.jobparams["ispin"] = 2
        job.jobparams["magmom"] = magmom
        job.jobparams["repat"] = files.input
        job.restart = Extract, "../non-magnetic"

      # Then random anti-ferro.
      for i in range(input.nbantiferro):
        job = lat_jobdict / ("anti-ferro-%i" % (i+1))
        job.vasp = input.relaxer
        job.args = [structure]
        job.jobparams["ispin"] = 2
        job.jobparams["magmom"] = magnetic.random(structure, input.vasp.species)
        job.jobparams["repat"] = files.input
        job.restart = Extract, "../non-magnetic"
  return jobdict
                                           
def _input():
  from lada.opt import read_input
  from lada.vasp import Vasp, specie, files
  from lada.vasp.methods import RelaxCellShape

  # names we need to create input.
  input_dict = { "Vasp": Vasp, "U": specie.U, "nlep": specie.nlep, "RelaxCellShape": RelaxCellShape }
  return read_input("input.py", input_dict)
 

first_wave = _waves(True)
""" First waves of jobs. """
second_wave = _waves(False)
""" Second waves of jobs. """
