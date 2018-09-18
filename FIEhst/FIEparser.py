#******************************************************
 # Polytechnic of Turin / LIRMM
 # 2018
 # FIE - Fault Injection Environment v04.6
#******************************************************

from string 	import *
from sys 		import *
from os 		import *
from time 		import *
from numpy 		import *
from matplotlib import pyplot
from matplotlib.ticker import MaxNLocator

_VERSION = "04.6"			# Version of the program

LISTNAMES = ["FREERTOS GLOBAL VARIABLES","TASK TCB","TASK LIST","MUTEX or SEMAPHORE or QUEUE"]
LISTS = [
	["uxCurrentNumberOfTasks","xTickCount","uxTopReadyPriority","xSchedulerRunning","uxPendedTicks","xYieldPending","xNumOfOverflows","uxTaskNumber","xNextTaskUnblockTime","uxSchedulerSuspended"],
	["pxTopOfStack","uxPriority","pxStack","uxTCBNumber","uxTaskNumber","uxBasePriority","uxMutexesHeld","ulNotifiedValue","ucNotifyState","xStateListItem.xItemValue","xStateListItem.pxNext","xStateListItem.pxPrevious","xStateListItem.pvOwner","xStateListItem.pvContainer","xEventListItem.xItemValue","xEventListItem.pxNext","xEventListItem.pxPrevious","xEventListItem.pvOwner","xEventListItem.pvContainer"],
	["uxNumberOfItems","pxIndex","xListEnd.xItemValue","xListEnd.pxNext","xListEnd.pxPrevious"],
	["pcHead","pcTail","pcWriteTo","u.pcReadFrom","u.uxRecursiveCallCount","xTasksWaitingToSend.uxNumberOfItems","xTasksWaitingToSend.pxIndex","xTasksWaitingToSend.xListEnd.xItemValue","xTasksWaitingToSend.xListEnd.pxNext","xTasksWaitingToSend.xListEnd.pxPrevious","xTasksWaitingToReceive.uxNumberOfItems","xTasksWaitingToReceive.pxIndex","xTasksWaitingToReceive.xListEnd.xItemValue","xTasksWaitingToReceive.xListEnd.pxNext","xTasksWaitingToReceive.xListEnd.pxPrevious","uxMessagesWaiting","uxLength","uxItemSize","cRxLock","cTxLock","uxQueueNumber","ucQueueType"]
	]
NBIT_LISTS			= 	[
						[32,32,32,32,32,32,32,32,32,32],
						[32,32,32,32,32,32,32,32,8,32,32,32,32,32,32,32,32,32,32],
						[32,32,32,32,32],
						[32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,8,8,32,8]
						]
	
class FIEparser:
	
		"""
			****************************************************************
			__init__()
				Set default values to the parameters of the parser
			****************************************************************
		"""
		def __init__(self):
			self.MODE = ""
			self.LIST = ""
			self.LISTL = ""
			self.NBIT = ""
			self.CYC = ""
			self._n_of_experiments = 0		# Total number of experiments
			self._n_of_exp_per_var = 0		# Total times a locus is injected into
			self._n_of_injections = 0		# Number of injections done (after the consistency check)
			self._n_of_misb = 0				# Number of misbehaviors (crashes included)
			self._n_of_crashes = 0			# Number of times the system crashed
			self._n_of_freezes = 0			# Number of times the system freezed
			self._n_of_degradations = 0		# Number of times the system degraded
			self._n_of_inj_ok = 0			# Number of times the injection was ok (no data corruption)
			self._tolerance = 10			# Tolerance
			self._n_of_bm = 3				# Number of benchmark
			self._n_of_vars_per_bm = 4		# Number of recorded variables for benchmark
			self._max_etaf = -1				# Max etaf
			self._min_etaf = (2**32)-1		# Min etaf
			self._vuln_loci = []			# Loci where injection caused crash or not: 4n=locus - 4n+1=# misbehaviors - 4n+2=# crashes - 4n+3=# freezes - 4n+4=# degradations - 4n+5=list of bits causing misbehaviors
			self.faults = []				# List of parameters giving a fault
			self.l_consistence = []
			self.l_crash_r = []
			self.l_freeze_r = []
			self.l_degr_r = []
			self.l_faultsinbits_r = []
			self.l_crc_err = []
			self._filename_inj = ""
			self._filename_golden = ""
			self.fn = ""
			self._fp_inj = None
			self._fp_golden = None
			self.verbose = 0
						
			self.main()		
			
		"""
			****************************************************************
			main()
				Parse command line arguments, start the serial connection
				and run the main algorithm.
			****************************************************************
		"""
		def main(self):
			print "\n  *** FIEparser v"+_VERSION+" ***\n"
			ch = self.parseArg()
			if(ch == 0):
				print "  INFO > Parsing started"
				if(self.runFIEparser() == 0):
					print "  INFO > Parsing completed"
					print "  INFO > Resume started"
					self.runFIEresume()
					print "  INFO > Resume completed"
					self.runFIEplotter()
					print "\n  *** Exit ***\n"
				else:
					print "\n  *** Exit with errors ***\n"	
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
				if len(intArgv)==0:
					print "  ERR > No option specified. Type python2 FIEparser.py -h to get help."
					return -1
				for param in intArgv:
					if '-h' in param:
						self.printHelp()
						return 1
					else:
						if '-f' in param:
							self.fn = str(self.findParamArgs('-f',param))
							folder = str(self.fn.split("_")[1])
							self._filename_inj = "./logs/"+folder+"/"+self.fn+"_inj"
							self._filename_golden = "./logs/"+folder+"/"+self.fn+"_golden"
							if(path.exists(self._filename_inj) == False):
								print "  ERR > Injection file not found."
								return -1
							if(path.exists(self._filename_golden) == False):
								print "  ERR > Golden file not found."
								return -1
						elif '-t' in param:
							self._tolerance = int(self.findParamArgs('-t',param))
						elif '-n' in param:
							self._n_of_bm = int(self.findParamArgs('-n',param))
						elif '-v' in param:
							self.verbose = 1
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
			print "    python2 FIEparser.py -f<filename> -t[tolerance] "
			print "    python2 FIEparser.py -h"
			print "  Please, note that there is no space among the various options specifiers '-k' and their argument\n"
			print "  GLOSSARY:"
			print "    +Definitions valid for whole system"
			print "      mode\t\t\tmode of injection"
			print "      list\t\t\tlist number"
			print "      listl\t\t\tnumber of loci in the list"
			print "      nbit\t\t\tnumber of bits injected from LSB"
			print "      cyc\t\t\tnumber of injection cycles per locus"
			print "      inj per locus\t\tnumber of injections per locus"
			print "    +Definitions valid for each individual locus"
			print "      occurrences\t\tnumber of misbehaviors of a locus"
			print "      consistence\t\t(misbehavior/total injections on a locus)"
			print "      crash ratio\t\t\t(crashes/misbehavior) for a locus"
			print "      freeze ratio\t\t(freezes/misbehavior) for a locus"
			print "      degradation ratio\t\t(degradations/misbehavior) for a locus"
			
		"""
			****************************************************************
			runFIEparser()
				Main parsing algorithm
			****************************************************************
		"""	
		def runFIEparser(self):
			# Read 'inj' and golden 'files'
			self._fp_inj = file(self._filename_inj,"r")
			self._fp_golden = file(self._filename_golden,"r")
			buf_inj = self._fp_inj.readlines()
			buf_golden = self._fp_golden.readlines()
			self._fp_inj.close()
			self._fp_golden.close()
			buflen_inj = len(buf_inj)-1
			# Golden and inj files bust have the same length
			if(buflen_inj != len(buf_golden)-1):
				return -1
			# Clean elements of the list
			i = 0
			while i < buflen_inj:
				if (buf_inj[i]=="" or buf_inj[i]=="\n"):
					del buf_inj[i]
				if (buf_golden[i]=="" or buf_golden[i]=="\n"):
					del buf_golden[i]
				buf_inj[i] = buf_inj[i].replace("\n","")
				buf_golden[i] = buf_golden[i].replace("\n","")
				buf_inj[i] = buf_inj[i].replace("\r","")
				buf_golden[i] = buf_golden[i].replace("\r","")
				if (buf_inj[i]=="" or buf_inj[i]=="\n"):
					del buf_inj[i]
				if (buf_golden[i]=="" or buf_golden[i]=="\n"):
					del buf_golden[i]
				i += 1
			self.MODE = buf_inj[2].split("\t")[1] 
			self.LIST = str(buf_inj[3].split("\t")[1])
			self.LISTL = int(buf_inj[4].split("\t")[1]) 
			self.NBIT = int(buf_inj[5].split("\t")[1])
			self.CYC = int(buf_inj[6].split("\t")[1])
			body_inj = buf_inj[7:]
			body_golden = buf_golden[7:]
			self._n_of_experiments = len(body_inj)
			
			if self.NBIT >= 0:
				for i in range(0,self.NBIT):
					self.l_faultsinbits_r.append(0)
			elif self.NBIT < 0:
				for i in range(0,32):
					self.l_faultsinbits_r.append(0)
					
			# Analyze benchmark behavior
			warning = 0
			# For each experiment line ...
			i = 0
			while i < self._n_of_experiments:
				# Process parameters of the current experiment				
				bm_params_inj = body_inj[i].split("*")[0]
				bm_params_inj = bm_params_inj.split("\t")
				j = 0
				while j < len(bm_params_inj):
					if bm_params_inj[j]=="":
						del bm_params_inj[j]
					j += 1
				# Process benchmark results of both golden and inj files
				bm_res_inj = body_inj[i].split("*")[1]
				bm_res_inj = bm_res_inj.split("-")[0:self._n_of_bm]
				bm_res_golden = body_golden[i].split("*")[1]
				bm_res_golden = bm_res_golden.split("-")[0:self._n_of_bm]
				
				# Process sytem results about injection		
				sys_res_inj = body_inj[i].split("-")[-1]
				sys_res_inj = sys_res_inj.split("\t")
				k = 0
				while k < len(sys_res_inj):
					if sys_res_inj[k]=="":
						del sys_res_inj[k]
					k += 1
				if(sys_res_inj[0] == '1'):			# If injection is ok (right CRC)
					self._n_of_inj_ok += 1
				if(sys_res_inj[1] == '1'):			# If injection is done
					self._n_of_injections += 1
				else:								# If it is not done, go to next line
					i += 1
					continue
						
				# If system didn't crash check for benchmarks behavior
				if sys_res_inj[2] == '1':
					j = 0
					# For each benchmark ...
					while j < self._n_of_bm:
						# Prepare the comparison between inj and golden files
						bminj_i = bm_res_inj[j].split("\t")
						bmgol_i = bm_res_golden[j].split("\t")
						k = 0
						while k < len(bminj_i):
							if bminj_i[k]=="":
								del bminj_i[k]
							if bmgol_i[k]=="":
								del bmgol_i[k]
							k += 1
						# Comparison done here among the variables
						z = 0
						while z < self._n_of_vars_per_bm:
							if(int(bminj_i[z])<(int(bmgol_i[z])-self._tolerance) or int(bminj_i[z])>(int(bmgol_i[z])+self._tolerance)):
								warning += 1
								break
							z += 1
						j += 1
				elif(sys_res_inj[2] == '0'):
					if(int(sys_res_inj[3])>self._max_etaf):
						self._max_etaf = int(sys_res_inj[3])
					if(int(sys_res_inj[3])<self._min_etaf):		
						self._min_etaf = int(sys_res_inj[3])	
						
				# If there is a misbehavior of any kind
				if (warning > 0) or (sys_res_inj[2] == '0'):
					self._n_of_misb += 1
					# Locus appeared for the first time
					if bm_params_inj[2] not in self._vuln_loci:
						self._vuln_loci.append(bm_params_inj[2])	# Add locus number
						self._vuln_loci.append(1)					# Add misbehavior
						if sys_res_inj[2] == '0':			
							self._vuln_loci.append(1)
							self._vuln_loci.append(0)
							self._vuln_loci.append(0)
							self._n_of_crashes +=1
						elif warning == self._n_of_bm:				# Add freeze/degradation
							self._vuln_loci.append(0)
							self._vuln_loci.append(1)
							self._vuln_loci.append(0)
							self._n_of_freezes += 1
						else:
							self._vuln_loci.append(0)
							self._vuln_loci.append(0)
							self._vuln_loci.append(1)
							self._n_of_degradations += 1
						self._vuln_loci.append([bm_params_inj[1]])	# Add faulting bit
					# Locus already appeared
					elif bm_params_inj[2] in self._vuln_loci:
						pos = self._vuln_loci.index(bm_params_inj[2])			# Find locus index
						self._vuln_loci[pos+1] = self._vuln_loci[pos+1]+1		# Add 1 to misbehavior counter
						if sys_res_inj[2] == '0':			
							self._vuln_loci[pos+2] += 1							# Add 1 to crashes counter if necessary
							self._n_of_crashes +=1
						elif warning == self._n_of_bm:
							self._vuln_loci[pos+3] += 1							# Add 1 to freezes counter if necessary 
							self._n_of_freezes += 1
						else: 
							self._vuln_loci[pos+4] += 1							# Add 1 to degradations counter if necessary
							self._n_of_degradations += 1
						if bm_params_inj[1] not in self._vuln_loci[pos+5]:
							self._vuln_loci[pos+5].append(bm_params_inj[1])		# Add bit to faulting bits if not present	
					self.l_faultsinbits_r[int(bm_params_inj[1])] += 1	 
					
					print "  "+str(bm_params_inj)+"  "+str(bm_res_inj)+"  "+str(bm_res_golden)
					self.faults.append(bm_params_inj)
					
				# If no there is no misbehavior	
				else:
					if bm_params_inj[2] not in self._vuln_loci:
						self._vuln_loci.append(bm_params_inj[2])
						self._vuln_loci.append(0)		
						self._vuln_loci.append(0)
						self._vuln_loci.append(0)
						self._vuln_loci.append(0)		
						self._vuln_loci.append([])
				warning = 0
				i += 1
				
			return 0
					
		"""
			****************************************************************
			runFIEresume()
				Create a resume report
			****************************************************************
		"""	
		def runFIEresume(self):
			print "  +++++++++++++++++++++++++++++++++++++++++++++++++++"
			print "    *** MODE:\t\t" + str(self.MODE)
			print "    *** LIST:\t\t" + str(self.LIST)+" - "+LISTNAMES[int(self.LIST)-1]
			print "    *** LISTL:\t\t" + str(self.LISTL)
			print "    *** NBIT:\t\t" + str(self.NBIT)
			print "    *** CYC:\t\t" + str(self.CYC)
			print "    *** INJ PER LOCUS:\t" + str(int(self._n_of_experiments)/int(self.LISTL))
			print "  +++++++++++++++++++++++++++++++++++++++++++++++++++"
			print "    *** EXPERIMENTS:\t" + str(self._n_of_experiments)
			print "    *** INJECT OK:\t" + str(self._n_of_inj_ok)
			print "    *** INJECTIONS:\t" + str(self._n_of_injections)
			print "    *** FAULTS IN BITS: " + str(self.l_faultsinbits_r)
			print "    *** SYSTEM MISB:\t" + str(self._n_of_misb)
			print "    *** SYSTEM CRASH:\t" + str(self._n_of_crashes)
			print "    *** SYSTEM FREEZE:\t" + str(self._n_of_freezes)
			print "    *** SYSTEM DEGRADED:\t" + str(self._n_of_degradations)
			if self._max_etaf != -1:
				print "    *** MAX ETAF:\t" + str(self._max_etaf)
			else:
				print "    *** MIN ETAF:\tNo max ETAF"
			if self._min_etaf != (2**32)-1:
				print "    *** MIN ETAF:\t" + str(self._min_etaf)
			else:
				print "    *** MIN ETAF:\tNo min ETAF"
			print "  +++++++++++++++++++++++++++++++++++++++++++++++++++"
			i = 0
			while i < self.LISTL:
				if self._vuln_loci[6*i+1] == 0:
					self.l_consistence.append(0)
					self.l_crash_r.append(0)
					self.l_freeze_r.append(0)
					self.l_degr_r.append(0)
				else:
					print "    *** VULN " + str(self._vuln_loci[i*6])+" - "+LISTS[int(self.LIST)-1][int(self._vuln_loci[i*6])]
					print "        Occurences:\t" + str(self._vuln_loci[i*6+1])
					print "        Consistence:\t%.2f%s" %(float(self._vuln_loci[i*6+1])*float(self.LISTL)/float(self._n_of_experiments)*100,"%")
					print "        Crash ratio:\t%.2f%s\t %s" %(float(self._vuln_loci[i*6+2])/float(self._vuln_loci[i*6+1])*100,"%\t",str(self._vuln_loci[i*6+2]))
					print "        Freeze ratio:\t%.2f%s\t %s" %(float(self._vuln_loci[i*6+3])/float(self._vuln_loci[i*6+1])*100,"%\t",str(self._vuln_loci[i*6+3]))
					print "        Degrad ratio:\t%.2f%s\t %s" %(float(self._vuln_loci[i*6+4])/float(self._vuln_loci[i*6+1])*100,"%\t",str(self._vuln_loci[i*6+4]))
					print "        Faulting bits:\t" + str(self._vuln_loci[i*6+5])
					if self.verbose == 1:
						for k in range(0,len(self.faults),1):
							if int(self.faults[k][2]) == i:
								print "        "+str(self.faults[k])
					self.l_consistence.append(float(self._vuln_loci[i*6+1])*float(self.LISTL)/float(self._n_of_experiments)*100)
					self.l_crash_r.append(float(self._vuln_loci[i*6+2])/float(self._vuln_loci[i*6+1])*100)
					self.l_freeze_r.append(float(self._vuln_loci[i*6+3])/float(self._vuln_loci[i*6+1])*100)
					self.l_degr_r.append(float(self._vuln_loci[i*6+4])/float(self._vuln_loci[i*6+1])*100)
				i += 1
				
				
		"""
			****************************************************************
			runFIEplotter()
				Create a plot of the results
			****************************************************************
		"""	
		def runFIEplotter(self):
			pyplot.close("all")
			fig = pyplot.figure()
			tt = pyplot.suptitle(str(self.LIST)+" - "+LISTNAMES[int(self.LIST)-1] + " - " + str(self.fn))
			ax0  = pyplot.subplot(321)
			ax1  = pyplot.subplot(322)
			ax2  = pyplot.subplot(323)
			ax3  = pyplot.subplot(324)
			ax4  = pyplot.subplot(325)
			ax5  = pyplot.subplot(326)
			
			if max(self.l_consistence)>5:
				ax0.set_ylim(bottom=0,top=max(self.l_consistence)+5)
			else:
				ax0.set_ylim(bottom=0,top=max(self.l_consistence)+1)
				
			if max(self.l_crash_r)>5:
				ax1.set_ylim(bottom=0,top=max(self.l_crash_r)+5)
			else:
				ax1.set_ylim(bottom=0,top=max(self.l_crash_r)+1)
				
			if max(self.l_freeze_r)>5:
				ax2.set_ylim(bottom=0,top=max(self.l_freeze_r)+5)
			else:
				ax2.set_ylim(bottom=0,top=max(self.l_freeze_r)+1)
				
			if max(self.l_degr_r)>5:
				ax3.set_ylim(bottom=0,top=max(self.l_degr_r)+5)
			else:
				ax3.set_ylim(bottom=0,top=max(self.l_degr_r)+1)
			
			if max(self.l_faultsinbits_r)>5:
				ax4.set_ylim(bottom=0,top=max(self.l_faultsinbits_r)+5)
			else:
				ax4.set_ylim(bottom=0)
			
			if self.NBIT >= 0:
				ax5.set_ylim(bottom=-0.5,top=7.5)
			elif self.NBIT < 0:
				ax5.set_ylim(bottom=-0.5,top=32.5)
				
			# Define layout
			pyplot.tight_layout(rect=[0, 0.03, 1, 0.95],h_pad=2.0)
			pyplot.grid(which='both',axis='both',linewidth=0.5,linestyle='--')	
		
			# Print axis
			
			ax0.set_title("Consistence (#misb/#total_inj)")
			ax0.grid(color='black',linestyle=':',linewidth=0.5)
			ax0.set_xlabel("Faults")
			ax0.set_ylabel("Consistence[%]")
			ax0.set_xticks(range(0,self.LISTL,1))
			ax0.yaxis.set_major_locator(MaxNLocator(nbins=11))
			ax0.bar(range(0,self.LISTL,1),self.l_consistence,edgecolor='black',color='#88AAFF')
			
			ax1.set_title("Crash Ratio (#crash/#misb)")
			ax1.grid(color='black',linestyle=':',linewidth=0.5)
			ax1.set_xlabel("Faults")
			ax1.set_ylabel("Crash Ratio[%]")
			ax1.set_xticks(range(0,self.LISTL,1))
			ax1.yaxis.set_major_locator(MaxNLocator(nbins=11))
			ax1.bar(range(0,self.LISTL,1),self.l_crash_r,edgecolor='black',color='#FFAA66')
			
			ax2.set_title("Freeze Ratio (#freezes/#misb)")
			ax2.grid(color='black',linestyle=':',linewidth=0.5)
			ax2.set_xlabel("Faults")
			ax2.set_ylabel("Freeze Ratio[%]")
			ax2.set_xticks(range(0,self.LISTL,1))
			ax2.yaxis.set_major_locator(MaxNLocator(nbins=11))
			ax2.bar(range(0,self.LISTL,1),self.l_freeze_r,edgecolor='black',color='#FF8888')
			
			ax3.set_title("Degradation Ratio(#degradations/#misb)")
			ax3.grid(color='black',linestyle=':',linewidth=0.5)
			ax3.set_xlabel("Faults")
			ax3.set_ylabel("Degradation Ratio[%]")
			ax3.set_xticks(range(0,self.LISTL,1))
			ax3.yaxis.set_major_locator(MaxNLocator(nbins=11))
			ax3.bar(range(0,self.LISTL,1),self.l_degr_r,edgecolor='black',color='#AAEE88')
			
			ax4.set_title("Faults in Bits (#faults_bit)")
			ax4.grid(color='black',linestyle=':',linewidth=0.5)
			ax4.set_xlabel("Bits")
			ax4.set_ylabel("Faults in Bits[#]")
			if self.NBIT >= 0:
				ax4.set_xticks(range(0,abs(self.NBIT),1))
				ax4.yaxis.set_major_locator(MaxNLocator(nbins=11))
				ax4.bar(range(0,abs(self.NBIT),1),self.l_faultsinbits_r,edgecolor='black',color='#88EEAA')
			elif self.NBIT < 0:
				ax4.set_xticks(range(0,32,1))
				ax4.tick_params('x',labelsize=6)
				ax4.yaxis.set_major_locator(MaxNLocator(nbins=11))
				ax4.bar(range(0,32,1),self.l_faultsinbits_r,edgecolor='black',color='#88EEAA')
			
			ax5.set_title("Faulting bits per fault")
			ax5.grid(color='black',linestyle=':',linewidth=0.5)
			ax5.set_xlabel("Faults")
			ax5.set_ylabel("Faulting Bits")
			ax5.set_xticks(range(0,self.LISTL,1))
			if self.NBIT >= 0:
				ax5.set_yticks(range(0,abs(self.NBIT),1))
			elif self.NBIT < 0:
				ax5.set_yticks(range(0,32,3))
			i = 0
			while i<len(self._vuln_loci)/6:
				bits_tmp = self._vuln_loci[i*6+5]
				for j in range(0,len(bits_tmp),1):
					bits_tmp[j] = float(bits_tmp[j]) - 0.25
				if len(bits_tmp) > 0:
					ax5.bar([i]*len(bits_tmp),[0.5]*len(bits_tmp),bottom=bits_tmp,edgecolor='black',color='#AABBEE')
				else:
					ax5.bar([0],[-10],bottom=[-10])
				i += 1
			
			pyplot.show()
		
			
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
FIEparser()
