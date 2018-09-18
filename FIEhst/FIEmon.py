#******************************************************
 # Dioghenes
 # Polytechnic of Turin / LIRMM
 # 2018
 # FIE - Fault Injection Environment v04.6
#******************************************************

from serial 	import *
from string 	import *
from sys 		import *
from os 		import path
from time 		import *
from random 	import *

"""	
	********************************************************************
		FIEmon Setup section: set the following variables to have  
		a behavior that is the desired one; they must meet the same 
		used by the board.
	********************************************************************
"""
	# *** FIEmon setup ***  FIE mon behavior setup
_VERSION = "04.6"			# Version of the program
_STARTITCHAR = "+"			# Character to be received from the board to send back injection parameters
_STOPITCHAR = "."			# Character used by the PC to understand that the current run is terminated
_SEPCHAR = "\n"				# Used to separate a data from another one
_FILENAME = "L1a2time_golden"		# Use this variable to choice the base of the filename

	# *** CONSTANTS ***     Do not change these setups
cFIE_anlmode		=	5	# ANL mode - just to be used when a timer is required to do some analysis
cFIE_radmode		=	4	# RAD mode - injection (random time injection)
cFIE_depmode		=	3	# DEP mode - injection (deep injection with a very high number of instants)
cFIE_injmode		=	2	# INJ mode - injection (FIE completely enabled with no tracing)
cFIE_sijmode		=	1	# SIJ mode - single injection(FIE completely enabled with no reset, single injection)
cFIE_trcmode		=	0	# TRC mode - trace mode(FIE disabled and tracing enabled)
cFIE_MODE_NAMES		=	["TRACE","SINGLE INJECTION","DISCRETE INJECTION","DEEP INJECTION","RANDOM INJECTION","ANALYSIS"]
cFIE_N_LOCI 		=   [10,19,5,22] # Number of injection loci in the various lists
cFIE_NBIT_LOCI		= 	[
						[32,32,32,32,32,32,32,32,32,32],
						[32,32,32,32,32,32,32,32,8,32,32,32,32,32,32,32,32,32,32],
						[32,32,32,32,32],
						[32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,8,8,32,8]
						]

	# *** FIE mode ***
cFIE_ENABLED = cFIE_injmode			# Choose the mode

	# *** FIE list ***
cFIE_LIST 				=	1		# Select the list to use during injection (in RAD set to a val >1 to repeat many inj in rand time)

	# *** MODE setup ***			# Mode specific FIE setup
if (cFIE_ENABLED==cFIE_sijmode):
	cFIE_INJ_CYCLE 		=	0		# Dummy, leave at 0
	cFIE_INJ_BITN	 	=	0		# Position of bits to inject starting from LSB
	cFIE_INJ_LOCUS		=	18		# Locus to inject in
	cFIE_INJ_TIME_T	 	=	1023	# Period for injection timer
	cFIE_INJ_TIME_D	 	=	3412	# Prescaler for injection timer
elif (cFIE_ENABLED==cFIE_injmode):
	cFIE_INJ_CYCLE 		=	1		# Max number of injections to do on the same locus
	cFIE_INJ_BITN		=	8		# Position of bits to inject starting from LSB; -1 to inject in MSB only; 1 to inject in LSB only
	cFIE_INJ_TIME_T	 	=	10		# Max cFIE_INJ_TIME_T len
	cFIE_INJ_PERIOD_VECT =	[5221,5891,6143,6576,7399,7910,8019,8609,9132,9803]
	cFIE_INJ_TIME_D	 	=	5		# Max cFIE_INJ_TIME_D len
	cFIE_INJ_PRESC_VECT =	[5000,6000,7000,8000,9000]
elif (cFIE_ENABLED==cFIE_depmode):
	cFIE_INJ_CYCLE 		=	1		# Max number of injections to do on the same locus
	cFIE_INJ_BITN	 	=	1		# Position of bits to inject starting from LSB
	cFIE_INJ_TIME_T	 	=	10		# Max val for timer
	cFIE_INJ_TIME_D	 	=	10		# Max val for prescaler
	cFIE_INJ_BASE_T		=	7000	# Base val for timer DEP
	cFIE_INJ_BASE_D		=	7000	# Base val for prescaler DEP
elif (cFIE_ENABLED==cFIE_radmode):
	cFIE_INJ_CYCLE 		=	500		# Max number of injections to do on the same locus
	cFIE_INJ_BITN	 	=	-1		# Position of bits to inject starting from LSB
	cFIE_INJ_MOD_T	 	=	20000	# Max variability for timer to be summed to cFIE_INJ_BASE_T
	cFIE_INJ_MOD_D	 	=	10000	# Max variability for prescaler to be summed to cFIE_INJ_BASE_D
	cFIE_INJ_BASE_T		=	5000	# Base val for timer DEP
	cFIE_INJ_BASE_D		=	5000	# Base val for prescaler DEP
else:
	print "  ***  Bad configuration; nothing to do.  ***"
	exit()
	
"""	End of setup section ********************************************"""


class FIEmon:
	
	"""
		****************************************************************
		__init__()
			Set default parameters for the serial connection and define 
			valid values.
		****************************************************************
	"""
	def __init__(self):
		self._serialConn = None			# Serial object
		self._port   = "/dev/ttyACM0"	# Port to connect to like COMx or /dev/ttyACMx
		self._baud   = 9600				# Baudrate of connection
		self._tmou   = None				# Timeout for read operations in seconds
		self._bsize  = EIGHTBITS		# Bytesize 
		self._parity = PARITY_NONE		# Parity 
		self._stpbit = STOPBITS_ONE		# Stopbits 
		self._xonoff = False			# Enable/Disable XON/XOFF
		
		self.validParams = ['-o','-r','-h', '-t', '-b', '-p', '-s', '-x']
		self.validBaud 	 = [50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200]
		self.validParity = [PARITY_NONE, PARITY_EVEN, PARITY_ODD, PARITY_MARK, PARITY_SPACE]
		self.validStopbit= [STOPBITS_ONE, STOPBITS_ONE_POINT_FIVE, STOPBITS_TWO]
		self.validBySize = [FIVEBITS, SIXBITS, SEVENBITS, EIGHTBITS]
		
		self.main()		
		
		
	"""
		****************************************************************
		main()
			Parse command line arguments, start the serial connection
			and run the main algorithm.
		****************************************************************
	"""
	def main(self):
		print "\n  *** FIEmon v"+_VERSION+" ***\n"
		ch = self.parseArg()
		if(ch == 0):
			if(self.serialStart()==0):
				self.runFIEmon(_STARTITCHAR,_STOPITCHAR,cFIE_ENABLED)
				self.serialStop()
				print "  INFO > Serial logging finished"
			print "\n  *** Exit ***\n"
		elif(ch == -1):
			print "\n  *** Exit with errors ***\n"

		
	"""
		****************************************************************
		parseArg()
			Parse the command line parameters: if no parameter is given
			just use the internal defaults.
		****************************************************************
	"""
	def parseArg(self):
		try:
			intArgv = argv[1:]
			for param in intArgv:
				if '-h' in param:
					self.printHelp()
					return 1
				else:
					if '-o' in param:
						self._port = str(self.findParamArgs('-o',param))
					elif '-r' in param:
						self._baud = int(self.findParamArgs('-r',param))
						if self._baud not in self.validBaud:
							print "  ERR > Invalid baudrate"
							raise Exception
					elif '-t' in param:
						try:
							self._tmou = int(self.findParamArgs('-t',param))
						except:
							self._tmou = None
					elif '-b' in param:
						self._bsize = int(self.findParamArgs('-b',param))
						if self._bsize not in self.validBySize:
							print "  ERR > Invalid bitsize"
							raise Exception
					elif '-p' in param:
						self._parity = str(self.findParamArgs('-p',param))
						if self._parity not in self.validParity:
							print "  ERR > Invalid parity settings"
							raise Exception
					elif '-s' in param:
						self._stpbit = float(self.findParamArgs('-s',param))
						if self._stpbit not in self.validStopbit:
							print "  ERR > Invalid stop bits number"
							raise Exception
					elif '-x' in param:
						self._xonoff = bool(self.findParamArgs('-x',param))
					else:
						print "  ERR > Invalid option"
						raise Exception
			return 0
			
		except:
			return -1
		
	
	"""
		****************************************************************
		printHelp()
			Just print an help message.
		****************************************************************
	"""		
	def printHelp(self):
		print "  This script must be used in this way:"
		print "    python2 FIEmon_v04.py -o[port] -r[baud] -t[timeout] -b[bitsize] -p[parity] -s[stopbits] -x[xonxoff]"
		print "    python2 FIEmon_v04.py -h"
		print "  Please, note that there is no space among the various options specifiers '-k'and their argument"
		print "  Example:"
		print "    python2 FIEmon_v04.py -o/dev/ttyACM3 -r115200 -t100 -s1.5"
		print "  If you type python2 FIEmon_v04.py the following default values are applied."
		print "    Port:     /dev/ttyACM0"
		print "    Baud:     9600\t[50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200]"
		print "    Timeout:  infinite"
		print "    Bitsize:  8\t\t[5,6,7,8]"
		print "    Parity:   N\t\t[N,E,O,S,M]"
		print "    Stopbits: 1.0\t[1.0,1.5,2.0]"
		print "    Xonxoff:  0\t\t[0,1]\n"
	
	
	"""
		****************************************************************
		serialStart()
			Start the serial communication and wait for an input from
			the user for synchronization.
		****************************************************************
	"""	
	def serialStart(self):
		try:
			self.serialConn = Serial(port=self._port, 		\
									 baudrate=self._baud,	\
									 bytesize=self._bsize, 	\
									 parity=self._parity,	\
									 stopbits=self._stpbit, \
									 timeout=self._tmou,	\
									 xonxoff=self._xonoff)
		except:
			print "  INFO > Cannot open port "+self._port
			return -1
		print "  INFO > Port: "+self._port+" * Baud: "+str(self._baud)+" * Bytesize: "+str(self._bsize)+" * Parity: "+self._parity+" * Stopbits: "+str(self._stpbit)+" * Timeout: "+str(self._tmou)
		print "  INFO > Flushing away the buffer..."
		sleep(.5)
		self.serialConn.flush()
		sleep(.5)
		print "  ACTION > Press Enter to start the log or CTRL-c to exit...",
		try:
			raw_input()
		except KeyboardInterrupt:
			print ""
			return 1
		print "  INFO > Waiting for data..."
		print "*****************************************************"
		return 0


	"""
		****************************************************************
		runFIEmon()
			Main injection algorithm.
		****************************************************************
	"""	
	def runFIEmon(self,startitChar,stopitChar,mode):
		fn = "./logs/"+_FILENAME
		fp = open(fn,"w")
		
		fp.write("DATE:\t"+strftime("%d %b %Y",gmtime())+"\n")
		fp.write("TIME:\t"+strftime("%H:%M:%S",gmtime())+"\n")
		fp.write("MODE:\t"+cFIE_MODE_NAMES[cFIE_ENABLED]+"\n")
		fp.write("LIST:\t"+str(cFIE_LIST)+"\n")
		fp.write("LISTL:\t"+str(cFIE_N_LOCI[cFIE_LIST-1])+"\n")
		fp.write("NBIT:\t"+str(cFIE_INJ_BITN)+"\n");
		fp.write("CYC:\t"+str(cFIE_INJ_CYCLE)+"\n\n");
		
		self.serialConn.flush()		
		try:
			# *** Initialize counters
			if (mode == cFIE_sijmode):
				FIE_INJ_CYCLE = 0
				FIE_INJ_BITN = cFIE_INJ_BITN
				FIE_INJ_LOCUS = cFIE_INJ_LOCUS
				FIE_INJ_TIME_T = cFIE_INJ_TIME_T
				FIE_INJ_TIME_D = cFIE_INJ_TIME_D
				if cFIE_INJ_BITN >= cFIE_NBIT_LOCI[cFIE_LIST][cFIE_INJ_LOCUS]:
					print "  INFO > No injection done: non-existing bit"
					return
			elif (mode == cFIE_injmode):
				FIE_INJ_CYCLE = 0
				if (cFIE_INJ_BITN < 0):
					FIE_INJ_BITN = (cFIE_NBIT_LOCI[cFIE_LIST-1][0])-1
				else:
					FIE_INJ_BITN = 0
				FIE_INJ_LOCUS = 0
				FIE_INJ_TIME_T = 0
				FIE_INJ_TIME_D = 0
			elif (mode == cFIE_depmode):
				FIE_INJ_CYCLE = 0
				if (cFIE_INJ_BITN < 0):
					FIE_INJ_BITN = (cFIE_NBIT_LOCI[cFIE_LIST-1][0])-1
				else:
					FIE_INJ_BITN = 0
				FIE_INJ_LOCUS = 0
				FIE_INJ_TIME_T = 0
				FIE_INJ_TIME_D = 0
			elif (mode == cFIE_radmode):
				FIE_INJ_CYCLE = 0
				if (cFIE_INJ_BITN < 0):
					FIE_INJ_BITN = (cFIE_NBIT_LOCI[cFIE_LIST-1][0])-1
				else:
					FIE_INJ_BITN = 0
				FIE_INJ_LOCUS = 0
				FIE_INJ_TIME_T = 0
				FIE_INJ_TIME_D = 0
			
			# *** Set to 0/void some variables
			c = ""
			buf = ""
			end = 0
			
			# *** Main injection loop
			while (end == 0):
				
				# *** Wait for start from the board
				while(c != startitChar):
					c = str(self.serialConn.read())
					
				self.serialConn.write("INIT".encode())
					
				# *** Send parameters to the board
				if (mode == cFIE_sijmode):
					startit_params = str(FIE_INJ_BITN)+"\t"+str(FIE_INJ_LOCUS)+"\t"+str(FIE_INJ_TIME_T)+"\t"+str(FIE_INJ_TIME_D)
				elif (mode == cFIE_injmode):
					startit_params = str(FIE_INJ_BITN)+"\t"+str(FIE_INJ_LOCUS)+"\t"+str(cFIE_INJ_PERIOD_VECT[FIE_INJ_TIME_T])+"\t"+str(cFIE_INJ_PRESC_VECT[FIE_INJ_TIME_D])
				elif (mode == cFIE_depmode):
					startit_params = str(FIE_INJ_BITN)+"\t"+str(FIE_INJ_LOCUS)+"\t"+str(cFIE_INJ_BASE_T+FIE_INJ_TIME_T)+"\t"+str(cFIE_INJ_BASE_D+FIE_INJ_TIME_D)
				elif (mode == cFIE_radmode):
					seed(FIE_INJ_CYCLE+FIE_INJ_BITN+FIE_INJ_LOCUS+FIE_INJ_TIME_T+FIE_INJ_TIME_D)
					FIE_INJ_TIME_T = cFIE_INJ_BASE_T + randint(0,cFIE_INJ_MOD_T)
					FIE_INJ_TIME_D = cFIE_INJ_BASE_D + randint(0,cFIE_INJ_MOD_D)
					startit_params = str(FIE_INJ_BITN)+"\t"+str(FIE_INJ_LOCUS)+"\t"+str(FIE_INJ_TIME_T)+"\t"+str(FIE_INJ_TIME_D)
				self.serialConn.write(startit_params.encode())
				
				# *** Receive results about injection
				while(c != stopitChar):	
					c = str(self.serialConn.read())
					if (c == '\r'):
						pass
					elif (c == startitChar):
						fp.write("\n\nInterruption from the board");
						raise KeyboardInterrupt
					elif(c == stopitChar):	
						tmp = buf.split("-")[-1]
						tmp = tmp.split("\t")[2]
						if (tmp == "0") and (mode != cFIE_radmode):
							FIE_INJ_CYCLE = cFIE_INJ_CYCLE
							FIE_INJ_BITN = 1000
							FIE_INJ_LOCUS = 1000
							buf = ""
							break
						print str(FIE_INJ_CYCLE)+"\t",
						fp.write(str(FIE_INJ_CYCLE)+"\t")
						print buf,
						dataLog = str(self.execAlgo(buf))
						fp.write(dataLog)							
						buf = ""
					else:
						buf = buf+c	
				
				# *** Stop now if in SIJ mode	
				if (mode == cFIE_sijmode):
					end = 1
					continue
				
				# *** Update parameters for next injection		
				if (FIE_INJ_CYCLE<cFIE_INJ_CYCLE-1):
					FIE_INJ_CYCLE += 1
				elif (cFIE_INJ_BITN>=0):
					if (FIE_INJ_BITN<cFIE_INJ_BITN-1) and (FIE_INJ_BITN<(cFIE_NBIT_LOCI[cFIE_LIST-1][FIE_INJ_LOCUS]-1)):
						FIE_INJ_CYCLE = 0
						FIE_INJ_BITN += 1
					elif (FIE_INJ_LOCUS<cFIE_N_LOCI[cFIE_LIST-1]-1):
						FIE_INJ_CYCLE = 0
						FIE_INJ_BITN = 0
						FIE_INJ_LOCUS += 1
					elif (mode != cFIE_radmode) and (FIE_INJ_TIME_T<cFIE_INJ_TIME_T-1):
						FIE_INJ_CYCLE = 0
						FIE_INJ_BITN = 0
						FIE_INJ_LOCUS = 0
						FIE_INJ_TIME_T += 1
					elif (mode != cFIE_radmode) and (FIE_INJ_TIME_D<cFIE_INJ_TIME_D-1):
						FIE_INJ_CYCLE = 0
						FIE_INJ_BITN = 0
						FIE_INJ_LOCUS = 0
						FIE_INJ_TIME_T = 0
						FIE_INJ_TIME_D += 1	
					else:
						end = 1
				elif (cFIE_INJ_BITN<0):
					if (FIE_INJ_BITN>0) and (FIE_INJ_BITN>(cFIE_NBIT_LOCI[cFIE_LIST-1][FIE_INJ_LOCUS]+cFIE_INJ_BITN)):
						FIE_INJ_CYCLE = 0
						FIE_INJ_BITN -= 1
					elif (FIE_INJ_LOCUS<cFIE_N_LOCI[cFIE_LIST-1]-1):
						FIE_INJ_CYCLE = 0
						FIE_INJ_LOCUS += 1
						FIE_INJ_BITN = cFIE_NBIT_LOCI[cFIE_LIST-1][FIE_INJ_LOCUS]-1
					elif (mode != cFIE_radmode) and (FIE_INJ_TIME_T<cFIE_INJ_TIME_T-1):
						FIE_INJ_CYCLE = 0
						FIE_INJ_LOCUS = 0
						FIE_INJ_BITN = cFIE_NBIT_LOCI[cFIE_LIST-1][FIE_INJ_LOCUS]-1
						FIE_INJ_TIME_T += 1
					elif (mode != cFIE_radmode) and (FIE_INJ_TIME_D<cFIE_INJ_TIME_D-1):
						FIE_INJ_CYCLE = 0
						FIE_INJ_LOCUS = 0
						FIE_INJ_BITN = cFIE_NBIT_LOCI[cFIE_LIST-1][FIE_INJ_LOCUS]-1
						FIE_INJ_TIME_T = 0
						FIE_INJ_TIME_D += 1	
					else:
						end = 1
			
			# *** Close the file at the end of the loop	
			fp.close()		
		
		except KeyboardInterrupt:
			print "\nINFO > Logging interrupted by the user"
			fp.close()	
		except Exception,e:
			print "INFO > Exception occurred. Please check the report:"
			print str(e)
			fp.close()	
					
			
	"""
		****************************************************************
		execAlgo()
			Implement here the data processing algorithm. This function
			returns the value that has to be written in the logging file
			so take care it adds a \n at the end of each processed data.
			Implement this function as a simple
				return buf
			if you don't want any data processing.
		****************************************************************
	"""		
	def execAlgo(self,buf):
		return buf


	"""
		****************************************************************
		serialStop()
			Stop the serial connection flushing the buffer.
		****************************************************************
	"""		
	def serialStop(self):
		self.serialConn.flush()
		sleep(.5)
		self.serialConn.close()
		print "*****************************************************"
		
		
	"""
		****************************************************************
		findParamArgs()
			Support function to be used to parse correctly each argument
			given by command line.
		****************************************************************
	"""		
	def findParamArgs(self,param,argument):
		ret = argument.replace(param,"")
		ret = ret.replace(" ","")
		return ret
		

""" 
	RUN
		Create an instance of the class 
"""
FIEmon()
