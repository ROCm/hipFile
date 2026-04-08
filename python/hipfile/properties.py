from hipfile._hipfile import (
    # Driver properties
    driver_get_properties as _driver_get_properties,
    # Library version
    get_version as _get_version,
)
from hipfile.error import HipFileException


def driver_get_properties():
    _props, err = _driver_get_properties()
    if err[0] != 0:
        raise HipFileException(err[0], err[1])
    return _props


def get_version():
    version_tuple, err = _get_version()
    if err[0] != 0:
        raise HipFileException(err[0], err[1])
    return version_tuple
