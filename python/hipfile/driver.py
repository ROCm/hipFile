from hipfile._hipfile import (
    driver_open,
    driver_close,
    use_count as _use_count
)

class Driver:

    @staticmethod
    def use_count():
        return _use_count()

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def close(self):
        driver_close()

    def open(self):
        driver_open()
