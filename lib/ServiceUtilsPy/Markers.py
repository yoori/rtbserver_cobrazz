import os
import time


class Marker:
    def __init__(self, markers, name, is_changed, mtime):
        self.name = name
        self.path = os.path.join(markers.context.markers_dir, name)
        self.is_changed = is_changed
        self.mtime = mtime


class Markers:
    def __init__(self, context):
        self.service = context.service
        self.context = context
        self.__markers = {}
        if self.context.markers_dir is not None:
            for _, _, files in os.walk(self.context.markers_dir, True):
                for file in files:
                    self.__markers[file] = Marker(
                        self,
                        file,
                        False,
                        os.path.getmtime(os.path.join(self.context.markers_dir,
                                                      file)))
                break

    def is_added(self, name):
        return self.__markers.get(name, None) is not None

    def add(self, name, mtime=None):
        try:
            marker = self.__markers[name]
        except KeyError:
            self.__markers[name] = Marker(self, name, True, time.time()
                                          if mtime is None else mtime)
            self.service.print_(0, f"Marker added {name}")
            return True
        if mtime is None or marker.mtime == mtime:
            self.service.print_(0, f"Marker already added {name}")
            return False
        marker.mtime = mtime
        marker.is_changed = True
        self.service.print_(0, f"Marker updated {name}")
        return True

    def get_mtime(self, name):
        return self.__markers[name].mtime

    def check_mtime_interval(self, name, interval):
        mtime = self.get_mtime(name)
        assert mtime is not None
        if mtime + interval <= time.time():
            return True
        self.service.print_(0, f"Marker still waiting {name}")
        return False

    def on_exit(self, is_error):
        if not is_error:
            for marker in self.__markers.values():
                if marker.is_changed:
                    with open(marker.path, "wt"):
                        pass
                    if marker.mtime is not None:
                        os.utime(marker.path, (os.path.getatime(marker.path),
                                               marker.mtime))
