def processalloc(job):
  """ returns a random number between 1 and 4 included. """
  from random import randint
  return randint(1, 4)

def test_failures(d):
  """ Tests whether scheduling jobs works on known failure cases. """
  from lada.jobfolder.jobfolder import JobFolder
  from lada.process.pool import PoolProcess
  from lada import default_comm
  from functional import Functional
  root = JobFolder()
  for n in xrange(8):
    job = root / "a{0}".format(n)
    job.functional = Functional("whatever", [n])
    job.params['sleep'] = 1

  comm = default_comm.copy()
  comm['n'] = 4

  def processalloc_test1(job):  return d[job.name[1:-1]]

  program = PoolProcess(root, processalloc=processalloc_test1, outdir="whatever")
  program._comm = comm
  for i in xrange(10000):
    jobs = program._getjobs()
    assert sum(program._alloc[u] for u in jobs) <= program._comm['n'],\
           (jobs, [program._alloc[u] for u in jobs])

def test_getjobs(nprocs=8, njobs=20):
  """ Test scheduling. """
  from lada.jobfolder.jobfolder import JobFolder
  from lada.process.pool import PoolProcess
  from lada import default_comm
  from functional import Functional

  root = JobFolder()
  for n in xrange(njobs):
    job = root / "a{0}".format(n)
    job.functional = Functional("whatever", [n])
    job.params['sleep'] = 1

  comm = default_comm.copy()
  comm['n'] = nprocs
  def processalloc(job):
    """ returns a random number between 1 and 4 included. """
    from random import randint
    return randint(1, comm['n'])

  for j in xrange(100):
    program = PoolProcess(root, processalloc=processalloc, outdir="whatever")
    program._comm = comm
    for i in xrange(1000):
      jobs = program._getjobs()
      assert sum(program._alloc[u] for u in jobs) <= program._comm['n'],\
             (jobs, [program._alloc[u] for u in jobs])

def test(executable):
  """ Tests JobFolderProcess. Includes failure modes.  """
  from time import sleep
  from tempfile import mkdtemp
  from os.path import join, exists
  from shutil import rmtree
  from numpy import all, arange, abs, array
  from lada.jobfolder.jobfolder import JobFolder
  from lada.jobfolder.massextract import MassExtract
  from lada.jobfolder import save
  from lada.process.pool import PoolProcess
  from lada.process import Fail
  from lada.misc import Changedir
  from lada.error import internal
  from lada import default_comm
  from functional import Functional

  root = JobFolder()
  for n in xrange(8):
    job = root / "a{0}".format(n)
    job.functional = Functional(executable, [n])
    job.params['sleep'] = 1

  comm = default_comm.copy()
  comm['n'] = 4

  dir = mkdtemp()
  save(root, join(dir, 'dict.dict'), overwrite=True)
  try: 
    program = PoolProcess(root, processalloc=processalloc, outdir=dir)
    assert program.nbjobsleft > 0
    # program not started. should fail.
    try: program.poll()
    except internal: pass
    else: raise Exception()
    try: program.wait()
    except internal: pass
    else: raise Exception()

    # now starting for real.
    program.start(comm)
    while not program.poll(): 
      sleep(1)
      continue
    assert program.nbjobsleft == 0
    extract = MassExtract(join(dir, 'dict.dict'))
    assert all(extract.success.itervalues())
    order = array(extract.order.values()).flatten()
    assert all(arange(8) - order == 0)
    pi = array(extract.pi.values()).flatten()
    assert all(abs(pi - array([0.0, 3.2, 3.162353, 3.150849,
                               3.146801, 3.144926, 3.143907, 3.143293]))\
                < 1e-5 )
    error = array(extract.error.values()).flatten()
    assert all(abs(error - array([3.141593, 0.05840735, 0.02076029, 0.009256556,
                                  0.005207865, 0.00333321, 0.002314774, 0.001700664]))\
                < 1e-5 )
    assert all(n['n'] == comm['n'] for n in extract.comm)
    # restart
    assert program.poll()
    assert len(program.process) == 0
    program.start(comm)
    assert len(program.process) == 0
    assert program.poll()
  finally: 
    try: rmtree(dir)
    except: pass

  try: 
    job = root / str(666)
    job.functional = Functional(executable, [666])
    program = PoolProcess(root, nbpools=2, outdir=dir, processalloc=processalloc)
    assert program.nbjobsleft > 0
    program.start(comm)
    program.wait()
    assert program.nbjobsleft == 0
  except Fail: 
    assert len(program.errors.keys()) == 1
    assert '666' in program.errors
  else: raise Exception
  finally:
    try: rmtree(dir)
    except: pass
  try: 
    job.functional.order = [667]
    program = PoolProcess(root, nbpools=2, outdir=dir, processalloc=processalloc)
    assert program.nbjobsleft > 0
    program.start(comm)
    program.wait()
    assert program.nbjobsleft == 0
  finally:
    try: rmtree(dir)
    except: pass

def test_large():
  """ Test speed of job-scheduling for largers, more numerous jobs. """
  from random import random
  from lada.jobfolder.jobfolder import JobFolder
  from lada.process.pool import PoolProcess
  from lada.process.dummy import DummyProcess, DummyFunctional
  from lada import default_comm
  root = JobFolder()
  for n in xrange(100):
    job = root / "a{0}".format(n)
    job.functional = DummyFunctional(chance=random()*0.5+0.15)

  comm = default_comm.copy()
  comm['n'] = 256

  def processalloc_test1(job): 
    """ returns a random number between 1 and 4 included. """
    from random import randint
    return randint(1, 64)

  program = PoolProcess(root, processalloc=processalloc_test1, outdir="whatever")
  program.start(comm)
  while not program.poll():
    # print 256 - program._comm['n'], len(program.process)
    continue

  def processalloc_test1(job): 
    """ returns a random number between 1 and 4 included. """
    from random import choice
    return choice([31, 37, 43])
  program = PoolProcess(root, processalloc=processalloc_test1, outdir="whatever")
  program.start(comm)
  while not program.poll():
    # print 256 - program._comm['n'], len(program.process)
    continue
    
if __name__ == "__main__":
  from sys import argv, path
  from os.path import abspath
  if len(argv) < 1: raise ValueError("test need to be passed location of pifunc.")
  if len(argv) > 2: path.extend(argv[2:])

  test_large()

  d = {'a1': 1, 'a0': 3, 'a3': 3, 'a2': 3, 'a5': 3, 'a4': 2, 'a7': 2, 'a6': 1}
  test_failures(d)
  test_getjobs(8, 20)
  test_getjobs(16, 60)
  test(abspath(argv[1]))

