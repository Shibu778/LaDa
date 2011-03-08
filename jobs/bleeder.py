""" Functor to *bleed* a job dictionary amongst pools of processes.
    
    Bleeding a job dictionary means that pools of processes are actively
    picking jobs from the dictionary, marking them as executed, and running
    them. Any one job should only be visited once.
"""

__docformat__ = "restructuredtext en"

class Bleeder(object): 
  """ Bleeds a jobdictionary from file. 
  
  
      The goal is to iterate through all jobs across multiple pools of
      processes, such that jobs are visited only once. It allows the best
      possible load balance across pools of processes. During iterations,
      modifications to the jobs are kept (when used in a for loop. Do not
      create lists).
  """
  def __init__(self, jobdict, pools, comm, directory = '.'): 
    """ Creates a bleeding job-dictionary. """
    from tempfile import NamedTemporaryFile
    from pickle import dump
    from ..opt import RelativeDirectory
    super(Bleeder, self).__init__()
   
    self._comm = comm
    """ *World* communicator. """

    self.pools = pools
    """ Number of processor pools to work with. """

    self._filename = None
    """ Name of the temp file where the job-dictionary is stored. """
    directory = RelativeDirectory(directory).path
    if self.is_root: 
      with NamedTemporaryFile(dir=directory, delete=False, prefix='ga_evaldict') as file:
        dump(jobdict, file)
        self._filename = file.name
    self._filename = self.broadcast(self._filename)

  @property 
  def pools(self):
    """ Number of processor pools to work with. """
    return self._pools
  @pools.setter
  def pools(self, value):
    if value == None: self._pools = 1
    elif not self.is_mpi: self._pools = 1
    elif value > self.comm.size: self._pools = self.comm.size
    else: self._pools = value

    self._local_comm = self.comm
    if self.is_mpi and self._pools > 1: 
      self._local_comm = self.comm.split(self.comm.rank % self._pools) 
  @pools.deleter
  def pools(self): self.pools = None

  @property
  def is_mpi(self): 
    """ True if this is an mpi session. """
    return False if self.comm == None else self.comm.size > 1
  @property 
  def is_root(self):
    """ True if this process is the world root. """
    return self.comm.rank == 0 if self.is_mpi else True
  @property
  def comm(self):
    """ *World* communicator. """
    return self._comm
  
  def broadcast(self, value):
    """ Broadcasts from comm, if needed. """
    if self.is_mpi:
      from boost.mpi import broadcast
      return broadcast(self.comm, value, 0)
    return value
  def barrier(self):
    """ MPI barrier if needed. """
    if self.is_mpi: self.comm.barrier()
  @property
  def is_local_root(self):
    """ True if this process is the local root. """
    return self.local_comm.rank == 0 if self.is_mpi else True
  @property
  def is_local_mpi(self):
    """ True if local comm has more than one rank. """
    return False if self.local_comm == None else self.local_comm.size > 1
  @property
  def local_comm(self): 
    """ Local communicator. """
    return self._local_comm
  def local_broadcast(self, value):
    """ Broadcasts from local comm, if needed. """
    if self.is_local_mpi:
      from boost.mpi import broadcast
      return broadcast(self.local_comm, value, 0)
    return value
  def local_barrier(self):
    """ Barrier over local processes. """
    if self.is_local_mpi: self.local_mpi.barrier()

  def __iter__(self): 
    """ Iterates over all jobs until completion. 
    
        Yielded jobs can be modified. These modifications will be saved.
    """
    from os.path import exists
    from pickle import load as pickle_load, dump
    from ..opt import LockFile
    # infinite loop. breaks when no new jobs can be found.
    while True:
      # only local root reads stuff. 
      job = None
      if self.is_local_root: 
        # acquire a lock first.
        with LockFile(self._filename) as lock:
          # checks for file existence. Done if no file.
          if exists(self._filename): 
            # Loads pickle.
            with open(self._filename, 'r') as file: jobdict = pickle_load(file)
            # Finds first untagged job.
            for job in jobdict.itervalues():
              if not job.is_tagged: break
            # Check we found an untagged job. Otherwise, we are done.
            if not job.is_tagged: 
              job.tag()
              with open(self._filename, 'w') as file: dump(jobdict, file)
            else: job = None # no job was found.
      # for all nodes, broadcasts job.
      job = self.local_broadcast(job)
      # check for bailout.
      if job == None: break

      # yield job.
      yield job

      # saves job and whatever modifications.
      if self.is_local_root: 
        # acquire a lock first.
        with LockFile(self._filename) as lock:
          # Loads pickle.
          with open(self._filename, 'r') as file: jobdict = pickle_load(file)
          # modifies job.
          jobdict[job.name] = job
          # save jobs.
          with open(self._filename, 'w') as file: dump(jobdict, file)
    
  def cleanup(self): 
    """ Cleans up disk. Return job-dictionary. """
    from os.path import exists
    from os import remove
    from pickle import load as pickle_load
    from ..opt import LockFile
    self.barrier()
    jobdict = None
    if self.is_root:
      # acquire a lock first.
      with LockFile(self._filename) as lock:
        # checks for file existence. Done if no file.
        if exists(self._filename): 
          # Loads pickle.
          with open(self._filename, 'r') as file: jobdict = pickle_load(file)
          remove(self._filename)
    return self.broadcast(jobdict)

  def itercompute(self, *args, **kwargs):
    """ Iterates over computable jobs. 
    
        Communicator is handled by this routine.
        Yields the object returned by each job, as well as the job itself.
        Yielded jobs can be modified. These modifications will be saved.
    """
    kwargs['comm'] = self.local_comm
    for job in self: yield job.compute(*args, **kwargs), job