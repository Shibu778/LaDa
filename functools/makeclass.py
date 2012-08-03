""" Creates functionals (classes) from a method. """
def create_initstring(classname, base, method, excludes):
  """ Creates a string defining the __init__ method. """
  from inspect import getargspec

  # creates line:  def __init__(self, ...):
  # keywords are deduced from arguments with defaults.
  # others will not be added.
  args = getargspec(method)
  result = "def __init__(self"
  if args.defaults is not None: 
    nargs = len(args.args) - len(args.defaults)
    for key, value in zip(args.args[nargs:], args.defaults):
      if key in excludes: continue
      result += ", {0}={1!r}".format(key, value)
  result += ", copy=None, **kwargs):\n"

  # adds standard doc string.
  result +=\
      "  \"\"\" Initializes {0} instance.\n\n"                                           \
      "     This function is created automagically from "                                \
        ":py:func:`{1.func_name} <{1.__module__}.{1.func_name}>`.\n"                     \
      "     Please see that function for parameter definitions other than ``copy``.\n"   \
      "     Other relevant keywords can be found in "                                    \
        ":py:func:`{2.__name__}.__init__ <{2.__module__}.{2.__name__}>`.\n\n"            \
      "     :param {2.__name__} copy:\n"                                                 \
      "         Deep-copies attributes from this instance to the new (derived) object.\n"\
      "         This parameter makes easy to create meta-functional from the most\n"     \
      "         basic wrappers.\n"                                                       \
      "  \"\"\"\n".format(classname, method, base)

  # creates line: from copy import deepcopy
  # used by the copy keyword argument below.
  result += "  from copy import deepcopy\n"
  # creates line: super(BASECLASS, self).__init__(...)
  # arguments are taken from BASECLASS.__init__
  result += "  super(self.__class__, self).__init__("
  initargs = getargspec(base.__init__)
  if initargs.args is not None and len(initargs) > 1:
    # first add args without defaults.
    # fails if not present in method's default arguments.
    ninitargs = len(initargs.args) - len(initargs.defaults)
    for i, key in enumerate(initargs.args[1:ninitargs]):
      if key in excludes: 
        raise Exception('Cannot ignore {1} when synthesizing {0}.'.format(classname, key))
      if key not in args.args[nargs:]:
        raise Exception('Could not synthesize {0}. Missing default argument.'.format(classname))
      result += ", {0}".format(key)
  if initargs.defaults is not None and args.defaults is not None: 
    # then add keyword arguments, ignoring thosse that are not in method
    for i, (key, value) in enumerate(zip(initargs.args[nargs:], initargs.defaults)):
      if key in args.args[ninitargs:]: result += ", {0} = {0}".format(key)
  # add a keyword dict if present in initargs
  if initargs.keywords is not None or initargs.defaults is not None: result += ', **kwargs'
  result += ')\n\n'
  # deals with issues on how to print first argument.
  result = result.replace('(, ', '(')

  # create lines: self.attr = value
  # where attr is something in method which is not in baseclass.__init__
  if args.defaults is not None: 
    for key, value in zip(args.args[nargs:], args.defaults):
      if key in excludes or key in initargs.args: continue
      result += "  self.{0} = {0}\n".format(key)

  # create lines which deep-copies base-class attributes to new derived attributes,
  # eg, using copy. Does not include previously set parameters and anything in
  # excludes.
  avoid = set(initargs.args[:ninitargs]) | set(args.args[nargs:]) | set(excludes)
  result += "  if copy is not None:\n"                                  \
                "    avoid = {0!r}\n"                                   \
                "    for key, value in copy.__dict__.iteritems():\n"    \
                "      if key not in avoid and key not in kwargs:\n"    \
                "         setattr(self, key, deepcopy(value))\n"        \
                .format(avoid)
  return result

def create_iter(iter, excludes, module, classname):
  """ Creates the iterator method. """
  from inspect import getargspec
  # make stateless.
  result = "from lada.functools import stateless, assign_attributes\n"\
           "@assign_attributes(ignore=['overwrite'])\n@stateless\n"
  # creates line:  def iter(self, ...):
  # keywords are deduced from arguments with defaults.
  # others will not be added.
  args = getargspec(iter)
  result += "def iter(self"
  if args.args is not None and len(args.args) > 1:
    # first add arguments without default (except for first == self).
    nargs = len(args.args) - len(args.defaults)
    for key in args.args[1:nargs]: result += ", {0}".format(key)
  if args.args is not None and len(args.args) > 1:
    # then add arguments with default
    nargs = len(args.args) - len(args.defaults)
    for key, value in zip(args.args[nargs:], args.defaults):
      if key in excludes: result += ", {0}={1!r}".format(key, value)
  # then add kwargs.,
  result += ", **kwargs):\n"

  # adds standard doc string.
  doc = iter.__doc__ 
  if doc is not None and '\n' in doc:
    first_line = doc[:doc.find('\n')].rstrip().lstrip()
    result +=\
        "  \"\"\"{0}\n\n"                                                  \
        "     This function is created automagically from "                \
          ":py:func:`{1.func_name} <{1.__module__}.{1.func_name}>`.\n"     \
        "     Please see that function for the description of its parameters.\n"\
        "  \"\"\"\n"\
        .format(first_line, iter)
  # import iterations method
  result += "  from lada.functools import SuperCall\n"                     \
            "  from {0.__module__} import {0.func_name}\n"                 \
            "  from {1} import {2}\n\n".format(iter, module, classname)
  # add iteration line:
  result += "  for o in {0.func_name}(SuperCall({1}, self)"                \
            .format(iter, classname)
  if args.args is not None and len(args.args) > 1:
    # first add arguments without default (except for first == self).
    nargs = len(args.args) - len(args.defaults)
    for key in args.args[1:nargs]: result += ", {0}".format(key)
  if args.args is not None and len(args.args) > 1:
    # then add arguments with default
    nargs = len(args.args) - len(args.defaults)
    for key in args.args[nargs:]:
      if key in excludes: result += ", {0}={0}".format(key)
      else: result += ", {0}=self.{0}".format(key)
  # adds arguments to overloaded function. 
  if args.keywords is not None: result += ", **kwargs"
  result += "): yield o\n"
 
  return result

def create_call_from_iter(iter, excludes):
  """ Creates a call method relying on existence of iter method. """
  from inspect import getargspec
  # creates line:  def call(self, ...):
  # keywords are deduced from arguments with defaults.
  # others will not be added.
  args = getargspec(iter)
  callargs = ['self']
  if args.args is not None and len(args.args) > 1:
    # first add arguments without default (except for first == self).
    nargs = len(args.args) - len(args.defaults)
    for key in args.args[1:nargs]: callargs.append(str(key))
  if args.args is not None and len(args.args) > 1:
    # then add arguments with default
    nargs = len(args.args) - len(args.defaults)
    for key, value in zip(args.args[nargs:], args.defaults):
      if key in excludes: callargs.append("{0}={1!r}".format(key, value))
  
  # then add kwargs,
  if args.args is None or 'comm' not in args.args: callargs.append('comm=None')
  if args.keywords is not None: callargs.append('**' + args.keywords)
  result = "def __call__({0}):\n".format(', '.join(callargs))
 
  # adds standard doc string.
  doc = iter.__doc__ 
  if doc is not None and '\n' in doc:
    first_line = doc[:doc.find('\n')].rstrip().lstrip()
    result +=\
        "  \"\"\"{0}\n\n"                                                  \
        "     This function is created automagically from iter_.\n"        \
        "     Please see that function for the description of its parameters.\n\n"\
        "     :param comm:\n"\
        "        Additional keyword argument defining how call external programs.\n"\
        "     :type comm: Dictionary or :py:class:`~lada.process.mpi.Communicator`\n\n"\
        "     .. _iter: :py:func:`{1.func_name} <{1.__module__}.{1.func_name}>`\n"     \
        "  \"\"\"\n"\
        .format(first_line, iter)
  # add iteration line:
  iterargs = []
  if args.args is not None and len(args.args) > 1:
    # first add arguments without default (except for first == self).
    nargs = len(args.args) - len(args.defaults)
    for key in args.args[1:nargs]: iterargs.append("{0}".format(key))
  if args.args is not None and len(args.args) > 1:
    # then add arguments with default
    nargs = len(args.args) - len(args.defaults)
    for key in args.args[nargs:]:
      if key in excludes: iterargs.append("{0}={0}".format(key))
  # adds arguments to overloaded function. 
  if args.args is None or 'comm' not in args.args:
    iterargs.append('comm=None')
  if args.keywords is not None: iterargs.append("**" + args.keywords)
  result += "  result  = None\n"                                               \
            "  for program in self.iter({0}):\n"                               \
            "    if getattr(program, 'success', False):\n"                     \
            "      result = program\n"                                         \
            "      continue\n"                                                 \
            "    if not hasattr(program, 'start'):\n"                          \
            "      return program\n"                                           \
            "    program.start(comm)\n"                                        \
            "    program.wait()\n"                                             \
            "  return result".format(', '.join(iterargs))
  return result

def create_call(call, excludes):
  """ Creates the call method. """
  from inspect import getargspec
  # make stateless.
  result = "from lada.functools import stateless, assign_attributes\n"\
           "@assign_attributes(ignore=['overwrite'])\n@stateless\n"
  # creates line:  def iter(self, ...):
  # keywords are deduced from arguments with defaults.
  # others will not be added.
  args = getargspec(call)
  result += "def __call__(self"
  if args.args is not None and len(args.args) > 1:
    # first add arguments without default (except for first == self).
    nargs = len(args.args) - len(args.defaults)
    for key in args.args[1:nargs]: result += ", {0}".format(key)
  if args.args is not None and len(args.args) > 1:
    # then add arguments with default
    nargs = len(args.args) - len(args.defaults)
    for key, value in zip(args.args[nargs:], args.defaults):
      if key in excludes: result += ", {0}={1!r}".format(key, value)
  # then add kwargs.,
  result += ", **kwargs):\n"

  # adds standard doc string.
  doc = call.__doc__ 
  if doc is not None and '\n' in doc:
    first_line = doc[:doc.find('\n')].rstrip().lstrip()
    result +=\
        "  \"\"\"{0}\n\n"                                                       \
        "     This function is created automagically from iter_"                \
        "     Please see that function for the description of its parameters.\n\n"\
        "     .. _iter: :py:func:`{1.func_name} <{1.__module__}.{1.func_name}>`.\n" \
        "  \"\"\"\n"\
        .format(first_line, call)
    # import iterations method
  result += "  from lada.functools import SuperCall\n".format(call)
  result += "  from {0.__module__} import {0.func_name}\n".format(call)
  # add iteration line:
  result += "  return {0.func_name}(SuperCall(self.__class__, self)".format(call)
  if args.args is not None and len(args.args) > 1:
    # first add arguments without default (except for first == self).
    nargs = len(args.args) - len(args.defaults)
    for key in args.args[1:nargs]: result += ", {0}".format(key)
  if args.args is not None and len(args.args) > 1:
    # then add arguments with default
    nargs = len(args.args) - len(args.defaults)
    for key in args.args[nargs:]:
      if key in excludes: result += ", {0}={0}".format(key)
      else: result += ", {0}=self.{0}".format(key)
  result = result.replace('(, ', '(')
  # adds arguments to overloaded function. 
  if args.keywords is not None: result += ", **kwargs"
  result += ")\n"
 
  return result

def makeclass( classname, base, iter=None, call=None,
               doc=None, excludes=None, module=None ):
  """ Creates a class from a function. 
  
      Makes it easy to create a class which works just like the input method.
      This means we don't have to write the boiler plate methods of a class,
      such as `__init__`. Instead, one can focus on writing a function which
      takes a functional and does something special with it, and then at the
      last minute create an actual derived class from the method and the
      functional. It is used for instance in :py:class:`vasp.Relax
      <lada.vasp.Relax>`. The parameters from the method which have defaults
      become attributes of instances of this class. Instances can be called as
      one would call the base functional, except of course the job of the
      method is done.
      
      :param str classname:
         Name of the resulting class.
      :param type base:
         Base class, e.g. for a method using VASP, this would be
         :py:class:`Vasp <lada.vasp.Vasp>`.
      :param function iter:
         The iteration version of the method being wrapped into a class, e.g.
         would override :py:meth:`Vasp.iter <lada.vasp.Vasp.iter>`.  Ignored if
         None.
      :param function call:
         The __call__ version of the method being wrapped into a class, e.g.
         would override :py:meth:`Vasp.__call__ <lada.vasp.Vasp.__call__>`.
         Ignored if None.
      :param str doc:
         Docstring of the class. Ignored if None.
      :param list excludes:
         List of strings indicating arguments (with defaults) of the methods
         which should *not* be turned into an attribute. If None, defaults to
         ``['structure', 'outdir', 'comm']``.
      :param bool withkword: 
         Whether to include ``**kwargs`` when calling the __init__ method of
         the *base* class. Only effective if the method accepts variable
         keyword arguments in the first place.
      :param str module:
         Name of the module within which this class will reside.

      :return: A new class derived from ``base`` but implementing the methods
        given on input. Furthermore it contains an `Extract` class-attribute
        coming from either ``iter``, ``call``, ``base``, in that order.
  """
  basemethod = iter if iter is not None else call
  if basemethod is None:
    raise ValueError('One of iter or call should not be None.')
  if excludes is None: excludes = ['structure', 'outdir', 'comm']

  # dictionary which will hold all synthesized functions.
  funcs = {}

  # creates __init__
  exec create_initstring(classname, base, basemethod, excludes) in funcs
  if iter is not None:
    exec create_iter(iter, excludes, module, classname) in funcs
  if call is not None: exec create_call(call, excludes) in funcs
  elif iter is not None:
    exec create_call_from_iter(iter, excludes) in funcs

  d = {'__init__': funcs['__init__']}
  if call is not None or iter is not None: d['__call__'] = funcs['__call__']
  if iter is not None: d['iter'] = funcs['iter']
  if doc is not None and len(doc.rstrip().lstrip()) > 0:
    d['__doc__'] = doc + "\n\nThis class was automagically generated by "\
                         ":py:func:`lada.functools.makeclass`."
  if hasattr(iter, 'Extract'): d['Extract'] = iter.Extract
  elif hasattr(call, 'Extract'): d['Extract'] = call.Extract
  elif hasattr(base, 'Extract'): d['Extract'] = base.Extract
  if module is not None: d['__module__'] = module
  return type(classname, (base,), d)

def makefunc(name, iter, module=None):
  """ Creates function from iterable. """
  from inspect import getargspec
  # creates header line of function calls.
  # keywords are deduced from arguments with defaults.
  # others will not be added.
  args = getargspec(iter)
  funcstring = "def {0}(".format(name)
  callargs = []
  if args.args is not None and len(args.args) > 0:
    # first add arguments without default (except for first == self).
    nargs = len(args.args) - len(args.defaults)
    for key in args.args[:nargs]: callargs.append(str(key))
  if args.args is not None and len(args.args) > 0:
    # then add arguments with default
    nargs = len(args.args) - len(args.defaults)
    for key, value in zip(args.args[nargs:], args.defaults):
      callargs.append("{0}={1!r}".format(key, value))
  # adds comm keyword, but only to function def.
  if args.keywords is not None:
    callargs.append('**{0}'.format(args.keywords))
  funcstring = "def {0}({1}):\n".format(name, ', '.join(callargs))

  # adds standard doc string.
  doc = iter.__doc__ 
  if doc is not None and '\n' in doc:
    first_line = doc[:doc.find('\n')].rstrip().lstrip()
    funcstring +=\
        "  \"\"\"{0}\n\n"                                                  \
        "     This function is created automagically from "                \
          ":py:func:`{1.func_name} <{1.__module__}.{1.func_name}>`.\n"     \
        "     Please see that function for the description of its parameters.\n\n"\
        "     :param comm:\n"\
        "        Additional keyword argument defining how call external programs.\n"\
        "     :type comm: Dictionary or :py:class:`~lada.process.mpi.Communicator`\n"\
        "  \"\"\"\n"\
        .format(first_line, iter)
  # create function body...
  funcstring += "  from {0.__module__} import {0.func_name}\n"\
                "  for program in {0.func_name}(".format(iter)
  # ... including detailed call to iterator function.
  iterargs = []
  if args.args is not None and len(args.args) > 0:
    for key in args.args: iterargs.append("{0}".format(key))
  if args.args is None or 'comm' not in args.args: 
    iterargs.append('comm=None')
  if args.keywords is not None: iterargs.append('**' + args.keywords)
  funcstring += "{0}):\n"\
                "    if getattr(program, 'success', False):\n"\
                "      result = program\n"\
                "      continue\n"\
                "    program.start(comm)\n"\
                "    program.wait()\n"\
                "  return result".format(', '.join(iterargs))
  funcs = {}
  exec funcstring in funcs
  if module is not None:  funcs[name].__module__ = module
  return funcs[name]

