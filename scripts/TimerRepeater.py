import threading

class TimerRepeater:
    """
    A simple timer implementation that repeats itself.
    """

    def __init__(self, name, interval, target):
        """
        Creates a timer.

        Parameters:
            name (str): Name of the thread.
            interval (float): Interval in seconds between execution of target.
            target (callable): Function that is called every 'interval' seconds.
        """
        self._name = name
        self._interval = interval
        self._target = target
        self._thread = None
        self._event = threading.Event()

    def _run(self):
        """
        Runs the thread that emulates the timer.
        """
        while not self._event.wait(self._interval):
            self._target()

    def start(self):
        """
        Starts the timer.
        """
        if self._thread is None:
            self._event.clear()
            self._thread = threading.Thread(target=self._run, name=self._name)
            self._thread.daemon = True  # Allows thread to be killed when main program exits
            self._thread.start()

    def stop(self):
        """
        Stops the timer.
        """
        if self._thread is not None:
            self._event.set()
            self._thread.join()
            self._thread = None

    def is_running(self):
        """
        Checks if the timer is currently running.

        Returns:
            bool: True if the timer is running, False otherwise.
        """
        return self._thread is not None and self._thread.is_alive()
