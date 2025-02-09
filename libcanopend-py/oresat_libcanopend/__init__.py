try:
    from ._version import version as __version__
except ImportError:
    __version__ = "0"

from .entry import Entry, DataType, EntryBitField
from .node_client import NodeClient
