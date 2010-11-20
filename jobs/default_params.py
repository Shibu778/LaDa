""" Holds some default parameters. """
__docformat__ = "restructuredtext en"

readonly = False
""" Whether items can be modified in parallel using attribute syntax. """
naked_end = True
""" Whether last item is returned as is or wrapped in ForwardingDict. """
only_existing = True
""" Whether attributes can be added or only modified. """
unix_re  = True
""" If True, then all regex matching is done using unix-command-line patterns. """
