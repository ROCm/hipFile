from hipfile._hipfile import *  # noqa: F401,F403
from hipfile._hipfile import (
    # Constants
    VERSION_MAJOR,
    VERSION_MINOR,
    VERSION_PATCH,
    BASE_ERR,
    # Error helpers
    is_hipfile_err,
    hipfile_errstr,
    is_hip_drv_err,
    hip_drv_err,
    get_op_error_string,
    # Version
    get_version,
    # File handles
    handle_register,
    handle_deregister,
    # Buffer registration
    buf_register,
    buf_deregister,
    # Synchronous I/O
    read,
    write,
    # Driver properties
    driver_get_properties,
)
from hipfile.driver import (
    Driver
)

__version__ = f"{VERSION_MAJOR}.{VERSION_MINOR}.{VERSION_PATCH}"
