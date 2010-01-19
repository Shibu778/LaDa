
#! /uhome/mdavezac/usr/bin/python
# 
#PBS -l nodes=2:ppn=2,walltime=48:00:0
#PBS -q Std
#PBS -m n 
#PBS -e err
#PBS -o out
import os


def is_valid(flavorbase, x):
  from lada import enumeration, crystal

  types = [ i for i in enumeration.IntegerIterator(flavorbase,x) ]
  return 2*len([i for i in types if i == 0]) == len(types)
  

def enum( n, lattice ):
  from math import pow
  import time
  import numpy as np
  from lada import enumeration, math, crystal

  supercells = enumeration.find_all_cells(lattice, n)
  smiths = enumeration.create_smith_groups(lattice, supercells)
  nflavors = enumeration.count_flavors(lattice)
  nsites = len([0 for i in lattice.sites if len(i.type) > 1])
  transforms = enumeration.create_transforms(lattice)
  for smith in smiths:
    card = smith.smith[0]*smith.smith[1]*smith.smith[2]*nsites
    label_exchange=enumeration.LabelExchange( card, nflavors )
    flavorbase = enumeration.create_flavorbase(card, nflavors)
    translations = enumeration.Translation(smith.smith, nsites)
    database = enumeration.Database(card, nflavors)
    maxterm = 0
    for x in xrange(1, int(pow(nflavors, card))-1):
      if not database[x]: continue
      if not is_valid(flavorbase, x):
        database[x] = False
        continue
      maxterm = x
      for labelperm in label_exchange:
        t = labelperm(x, flavorbase)
        if t > x: database[t] = False
      for translation in translations:
        t = translation(x, flavorbase)
        if t > x: database[t] = False
        elif t == x: 
          database[t] = False
          continue
        for labelperm in label_exchange:
          u = labelperm(t, flavorbase)
          if u > x: database[u] = False

    # checks supercell dependent transforms.
    for nsupercell, supercell in enumerate(smith.supercells):
      mine = []
      # creates list of transformation which leave the supercell invariant.
      cell = np.dot(lattice.cell, supercell.hermite)
      specialized = []
      for transform in transforms:
        if not transform.invariant(cell): continue
        transform.init(supercell.transform, smith.smith)
        if not transform.is_trivial:
          specialized.append( transform )

      specialized_database = enumeration.Database(database)
      for x in xrange(1, maxterm+1):
        if not database[x]: continue
        maxterm = x
        
        for transform in specialized:
          t = transform(x, flavorbase)
          if t == x: continue
          specialized_database[t] = False

          for labelperm in label_exchange:
            u = labelperm(t, flavorbase)
            if u == x: continue
            specialized_database[u] = False
            
          for translation in translations:
            u = translation(t, flavorbase)
            if u == x: continue
            specialized_database[u] = False
            for labelperm in label_exchange:
              v = labelperm(u, flavorbase)
              if v == x: continue
              specialized_database[v] = False
        if specialized_database[x]: yield x, smith, supercell, flavorbase

def create_database(lattice, n0=1, n1=4):
  from lada import enumeration, crystal
  from math import pow

  # prints lattice
  lattice.set_as_crystal_lattice()
  print "lattice:\n  cell: ",
  for i in range(3):
    for j in range(3):
      print lattice.cell[i,j],
  print 
  print "  scale: ", lattice.scale
  for site in lattice.sites:
    print "  site: ",
    for i in range(3): print site.pos[i],
    for i in site.type: print i, 
    print 
  print "endlattice\n\n"

  nconf = 1
  nconf = 0
  result = []
  perrow = 21
  iter = 0
  for n in range(n0, n1):
    if iter != 0: 
      print 
      iter = 0
    print "\nn: ", n
    npern = 0
    oldsupercell = None
    first = True
    for x, smith, supercell, flavorbase in enum(n, lattice):
      if oldsupercell == None or oldsupercell != supercell:
        if first:
          print "  flavorbase: ", len(flavorbase), flavorbase[1]
          first = False
        npern = 0
        if iter != 0: 
          print 
          iter = 1
        print "  hermite: ",
        for i in range(3):
          for j in range(3):
            print supercell.hermite[i,j],
        print
        print "  transform: ",
        for i in range(3):
          for j in range(3):
            print supercell.transform[i,j],
        print
        print "  smith: ",
        for i in range(3): print smith.smith[i],
        print
        oldsupercell = supercell
        iter = 0

      if iter == 0:
        print "   ",
      elif iter % perrow == 0:
        iter = 0
        print
        print "   ",
      iter += 1
      print x,

def read_database(filename, withperms=True):
  import re
  import numpy as np
  from lada import crystal, enumeration

  lattice = crystal.Lattice()
  with open(filename, "r") as file:
    
    # reads lattice
    for line in file:
      data = line.split()
      if len(data) == 0: continue
      if data[0] == "cell:":
        data.pop(0)
        for i in range(3):
          for j in range(3): 
            lattice.cell[i,j] = float(data.pop(0))
      elif data[0] == "scale:":
        lattice.scale = float(line.split()[1])
      elif data[0] == "site:":
        data = line.split()
        data.pop(0)
        site = crystal.Site()
        for i in range(3): site.pos[i] = float(data.pop(0))
        while len(data): site.type.append( data.pop(0) )
        lattice.sites.append(site)
      elif data[0] == "endlattice": break

    lattice.set_as_crystal_lattice()
    transforms = enumeration.create_transforms(lattice)
    specialized = []
    nsites = len([0 for i in lattice.sites if len(i.type) > 1])

    hermite = None
    flavorbase = None
    transform = np.zeros((3,3), dtype="float64")
    structure = crystal.sStructure()
    structure.scale = lattice.scale
    for line in file:
      data = line.split()
      if len(data) == 0: continue
      if data[0] == "n:": continue
      if data[0] == "hermite:":
        data.pop(0)
        if hermite == None: hermite = np.zeros((3,3), dtype="float64")
        for i in range(3):
          for j in range(3): 
            hermite[i,j] = float(data.pop(0))
        specialized = []
      elif data[0] == "transform:": 
        data.pop(0)
        for i in range(3):
          for j in range(3): 
            transform[i,j] = float(data.pop(0))
      elif data[0] == "smith:":
        smith = np.array( [int(data[1]), int(data[2]), int(data[3])] )
        translations = enumeration.Translation(smith, nsites)
        cell = np.dot(lattice.cell, hermite)
        structure.cell = cell
        structure.atoms.clear()
        crystal.fill_structure(structure)
        for transformation in transforms:
          if not transformation.invariant(cell): continue
          transformation.init(transform, smith)
          if not transformation.is_trivial:
            specialized.append( transformation )
      elif data[0] == "flavorbase:":
        card, nflavors = int(data[1]),  int(data[2])
        flavorbase = enumeration.create_flavorbase(card, nflavors)
        label_exchange=enumeration.LabelExchange(card, nflavors)
      elif len(data) > 0:
        assert flavorbase != None
        assert hermite != None
        for x in data:
          # adds label permutations that are not equivalent by affine transforms.
          x = int(x)
          others = set([x])
          if withperms:
            for labelperm in label_exchange:
              u = labelperm(x, flavorbase)
              if u in others: continue
  
              dont = False
              for transform in specialized:
                t = transform(u, flavorbase)
                if t in others:
                  dont = True
                  break
  
                for translation in translations:
                  v = translation(t, flavorbase)
                  if v in others:
                    dont = True
                    break
              if not dont: others.add(u)

          for u in others:
            enumeration.as_structure(structure, u, flavorbase)
            yield structure

          

if __name__ == "__main__":
  import inverse_lattice

  lattice = inverse_lattice.lattice()
  lattice.sites[0].type = lattice.sites[2].type
  lattice.sites[1].type = lattice.sites[2].type
  create_database(lattice, 1, 5) 

# N = 0
# for structure in read_database("database", withperms=True): N  += 1
# print N


