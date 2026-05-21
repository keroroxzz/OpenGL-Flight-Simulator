import subprocess
import threading
import os

def run_gdb():
    os.chdir('flight_sim')
    process = subprocess.Popen(['gdb', '-batch', '-ex', 'run', '-ex', 'bt', '-ex', 'quit', './flight_sim'], 
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    def kill_proc():
        process.kill()
    timer = threading.Timer(10.0, kill_proc)
    timer.start()
    out, _ = process.communicate()
    timer.cancel()
    print(out.decode())

run_gdb()