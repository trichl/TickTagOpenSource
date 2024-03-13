import serial
import time
import pandas as pd
from datetime import datetime
import os
from tqdm import tqdm
from inputimeout import inputimeout, TimeoutOccurred
import sys


PORT = 'COM18'
BAUDRATE = 9600
TIMEOUT = 5

def str_to_dict(datastr): # Looks like 'id : value, id : value ...'
    datastr = datastr.replace(' ', '')
    datastr = datastr.replace('\r', '')
    datastr = datastr.replace('\n', '')
    return dict(subString.split(":") for subString in datastr.split(","))


def get_digits(interest_string):
    return ''.join(filter(lambda i: i.isdigit(), interest_string))


def progressbar_init(total_bar, description, units):
        progressbar = tqdm(total=total_bar, desc=description, unit=units, colour='green', file=sys.stdout, leave=False)
        return progressbar


def exit_program():
    print("\n\nQ key pressed : Exiting the program now !\n\n")
    sys.exit()

class Serial_Reader:

    buffer = []
    serial_active = 1
    line_count = 0
    progressbar = None
    serial_connect = None

    def __init__(self, port, baudrate, timeout):
        self.serial_connect = serial.Serial(port, baudrate, timeout=timeout)
        self.progressbar = progressbar_init(total_bar=1000, description="Reading Serial Data", units="lines")
        return

    def progressbar_update(self):
        self.progressbar.update(1)
        return
    
    def progressbar_change_max(self, total_bar):
        self.progressbar.total = total_bar
        return
    
    def read_line(self):
        line = self.serial_connect.readline().decode('utf-8').strip('\n')
        return line
    
    def listen(self, Tag_Data):
        with self.serial_connect as ser:
            while self.serial_active:
                line = self.read_line()
                if line:
                    self.progressbar.update(1)
                    self.buffer.append(line)
                    #print(line)

                if self.line_count == 1:
                    try:
                        Tag_Data.datadict = str_to_dict(line)      
                        self.line_count += 1
                    except ValueError:
                        print(20*'-')
                        print('\n TAG HAS NO DATA IN MEMORY \n')
                        sys.exit()

                elif self.line_count == 2:
                    Tag_Data.datadict = str_to_dict(line) | Tag_Data.datadict
                    #print(Tag_Data.datadict)
                    Tag_Data.fixes = int(get_digits(Tag_Data.datadict['Fixes']))
                    #print(f'{Tag_Data.fixes} fixes and type {type(Tag_Data.fixes)}')

                    self.progressbar_change_max(Tag_Data.fixes)
                    self.line_count = 0
                    
                elif "*START" in line:
                    self.line_count = 1
 
                elif 'ID:' in line:
                    taginfo = line.split(',')
                    Tag_Data.software_version, Tag_Data.id, Tag_Data.voltage = taginfo[0], taginfo[1][8:], taginfo[2]

                elif '9 Exit' in line:
                    self.serial_active = 0

        return Tag_Data
    
    def reset_memory(self):
        try:
            reset_memory = inputimeout(prompt="Would you like to reset the memory ? y/N\n", timeout=40)
        except TimeoutOccurred:
            reset_memory = "N"
        if reset_memory.upper() == 'Y':
            print('Resetting memory ... ')
            with self.serial_connect as ser:
                while ser.is_open == 0:
                    time.sleep(0.5)

                ser.write(b'\n1\r\n')
                line = ser.readline().decode('utf-8').strip('\n')
                print(line)
        else:
            print("Memory has not been reset, a wrong character was entered or it took too long.\n")

    def end_of_communication(self):
        self.progressbar.close()
        print(f"{20*'-'}\n")
        print(f"Communication with TickTag has been closed.\n")
        print(f"{20*'-'}")

    
class Tag_Data():
    datadict = {}
    fixes = None
    id = None
    sofware_version = None
    voltage = None
    logdata = None

    def __init__(self):
        pass
        
    def main(self):
        self.write_temp_files()
        self.get_first_timestamp()
        self.make_summary_files()
        self.update_summary_files()
        self.order_files()

    def get_data(self, buffer):
        self.logdata = buffer
        #self.setup_progresbars()

    def setup_progresbars(self):
        self.logbar = progressbar_init(total_bar=len(self.logdata), description='Writing log .txt File ...', units="lines")
        self.csvbar = progressbar_init(total_bar=len(self.logdata), description='Writing csv .txt File ...', units="lines")
        return
        
    def progressbar_update(self, progressbar):
        progressbar.update(1)
        return
    
    def write_temp_files(self):
        with open(f'test_log_temp.txt', 'w') as logfile:  
            for line in self.logdata:
                logfile.write(line)
                #self.progressbar_update(self.logbar)

        with open(f'test_data_temp.csv', 'w') as csvfile:
            csvfile.write('count,timestamp,lat,lon\n')
            for line in self.logdata:
                if ',' in line[0:5]:
                    csvfile.write(line)
                else:
                    pass
                #self.progressbar_update(self.csvbar)

    def get_first_timestamp(self):
        column_names=["count", "timestamp", "lat", "lon"]
        df = pd.read_csv(f'test_data_temp.csv', sep=',', usecols=column_names)
        self.time_start = df['timestamp'].iloc[0]
        self.time_stop = df['timestamp'].iloc[-1]
        self.duration = self.time_stop - self.time_start
        self.date_first_timestamp = datetime.fromtimestamp(self.time_start).strftime('%Y-%m-%d')

        return self.date_first_timestamp
    
    def print_info(self):

        print(f'Data starts from : {time.ctime(self.time_start)}')
        print(f'Data stops at : {time.ctime(self.time_stop)}')
        print(f'Got {self.fixes} fixes in {self.duration//3600}h and {((self.duration - (self.duration//3600)*3600) /60)}min')
        print(f'Tag id is {self.id} and voltage is {self.voltage}')

    def make_summary_files(self):
        try:
            with open(f'summary_{self.date_first_timestamp}.csv', 'r') as summary_file:
                print("\nExistsing date summary file ...")
        except:
            with open(f'summary_{self.date_first_timestamp}.csv', 'w') as summary_file:
                print('\nCreating new summary file')
                summary_file.write('id,fixes,voltage(mV),duration(h),TTFF(s),Avg. TTF(s),UVs,ErrorsOrGF,TOs,Avg. HDOP (x10)\n')
        try:
            with open(f'summary.csv', 'r') as summary_file:
                #print("Existsing global summary file ...")
                pass
        except:
            with open(f'summary.csv', 'w') as summary_file:
                print('\nCreating new global summary file')
                summary_file.write('id,fixes,voltage(mV),duration(h),TTFF(s),Avg. TTF(s),UVs,ErrorsOrGF,TOs,Avg. HDOP (x10)\n')
        
    def update_summary_files(self):
        with open(f'summary_{self.date_first_timestamp}.csv', 'a') as summary_file:
            summary_file.write(f'''{self.id},{self.fixes},{get_digits(self.voltage)},{round(self.duration/3600,3)},{get_digits(self.datadict['TTFF'])},{get_digits(self.datadict['Avg.TTF'])},{self.datadict['UVs']},{self.datadict['ErrorsOrGF']},{self.datadict['TOs']},{self.datadict['Avg.HDOP(x10)']}\n''')
    
        with open(f'summary.csv', 'a') as summary_file:
            summary_file.write(f'''{self.id},{self.fixes},{get_digits(self.voltage)},{round(self.duration/3600,3)},{get_digits(self.datadict['TTFF'])},{get_digits(self.datadict['Avg.TTF'])},{self.datadict['UVs']},{self.datadict['ErrorsOrGF']},{self.datadict['TOs']},{self.datadict['Avg.HDOP(x10)']}\n''')

    def order_files(self):
        filename_log = self.date_first_timestamp+'-'+str(self.id)+'.txt'
        filename_csv = self.date_first_timestamp+'-'+str(self.id)+'.csv'
        try:
            ordering = f"{self.date_first_timestamp}/"
            newpath = os.path.join(os.path.dirname(os.path.abspath(__file__)), ordering)
            if not os.path.exists(newpath):
                os.makedirs(newpath)
            os.rename('test_log_temp.txt', os.path.join(ordering, filename_log))
            os.rename('test_data_temp.csv', os.path.join(ordering, filename_csv))
        except:
            print(f'{os.rename('test_log_temp.txt', os.path.join(ordering, filename_log))}')
            print(f'{os.rename('test_log_temp.txt', os.path.join(ordering, filename_csv))}')
            print("\nThese files are already existing, please delete them if data was currupted or if you want to overwrite content")

if __name__ == '__main__':
    while True:
        print(10 * '-')
        print(f"Press any key to download data from the tag\n")
        print(f"Press q to exit the application\n")
        a = input('Press enter to continue...\n')
        if a.lower == 'q':
            sys.exit()
        sr = Serial_Reader(port=PORT, baudrate=BAUDRATE, timeout=TIMEOUT)
        td = Tag_Data()
        td = sr.listen(td)
        td.get_data(sr.buffer)
        td.main()
        sr.reset_memory()
        sr.end_of_communication()


    

    