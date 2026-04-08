from hipfile._hipfile import get_op_error_string


class HipFileException(Exception):
    def __init__(self, hipfile_err, hip_err):
        self._hipfile_err = hipfile_err
        self._hip_err = hip_err

    @property
    def hipfile_err(self):
        return self._hipfile_err

    @property
    def hip_err(self):
        return self._hip_err

    def __str__(self):
        err_msg = f"{self._hipfile_err} - {get_op_error_string(self._hipfile_err)}"
        if self._hipfile_err == 5011:
            err_msg += f" {self._hip_err}"
        return err_msg
