"""
Script that takes the raw output from cmsRun when running either the MemoryCounter or ModuleTimer
and formats it into a comma separated spreadsheet.
Input should be the filename of a dump of the cmsRun output, and output is to standard out so
redirect it into a file.
@author Mark Grimes (mark.grimes@bristol.ac.uk)
"""
import sys


if len( sys.argv ) != 2 : raise Exception( 'The only parameter should be the filename of a dump of the cmsRun output' )

input=open( sys.argv[1] )


class ModuleInfo :
	class MemoryLog :
		def __init__( self, heldMemory, peakMemory, heldAllocation, peakAllocation ) :
			self.heldMemory=float(heldMemory)/1024.0/1024.0
			self.peakMemory=float(peakMemory)/1024.0/1024.0
			self.heldAllocation=heldAllocation
			self.peakAllocation=peakAllocation

	class TimeLog :
		def __init__( self, real, user, sys ) :
			self.real=float(real)/1000000000000.0
			self.user=float(user)/1000000000000.0
			self.sys=float(sys)/1000000000000.0

	def __init__( self, columns ) :
		if len(columns)<3 : raise Exception("Not enough columns")
		
		self.name=columns[1]
		self.type=columns[2]
		self.steps={}
		self.timeSteps={}
		#self.addStep( columns )

	def addStep( self, columns ) :
		if len(columns)<7 : raise Exception("Not enough columns")
		if self.name!=columns[1] : raise Exception("Trying to add step from an incorrect module name")
		if self.type!=columns[2] : raise Exception("Trying to add step from an incorrect module type")
		
		self.steps[columns[0]]=ModuleInfo.MemoryLog(columns[3],columns[4],columns[5],columns[6])
		
	def addTimeStep( self, columns ) :
		if len(columns)<6 : raise Exception("Not enough columns")
		if self.name!=columns[1] : raise Exception("Trying to add step from an incorrect module name")
		if self.type!=columns[2] : raise Exception("Trying to add step from an incorrect module type")
		
		self.timeSteps[columns[0]]=ModuleInfo.TimeLog(columns[3],columns[4],columns[5])

def dumpMemory( allModuleInfo, nameOrder, steps, peakMemory=True ) :
	if peakMemory : print "Peak memory"
	else : print "Retained memory"
	
	lineToOutput="Type,Name,"
	for step in steps :
		lineToOutput+=step+","
	print lineToOutput
	
	for name in nameOrder :
		moduleInfo=allModuleInfo[name]
		if moduleInfo.name=='EVENT' and moduleInfo.type=='EVENT' :
			continue
		lineToOutput=moduleInfo.type+","+moduleInfo.name+","
		for step in steps :
			try :
				if peakMemory : lineToOutput+=str(moduleInfo.steps[step].peakMemory)+","
				else : lineToOutput+=str(moduleInfo.steps[step].heldMemory)+","
			except KeyError :
				lineToOutput+=","
		print lineToOutput

def dumpTime( allModuleInfo, nameOrder, steps, timeType="Real" ) :
	print timeType

	lineToOutput="Type,Name,"
	for step in steps :
		lineToOutput+=step+","
	print lineToOutput
	
	for name in moduleOrder :
		moduleInfo=info[name]
		if moduleInfo.name=='EVENT' and moduleInfo.type=='EVENT' :
			lineToOutput="Event,Total,"
		else :
			lineToOutput=moduleInfo.type+","+moduleInfo.name+","
		for step in steps :
			try :
				if timeType=="Real" : timeValue=moduleInfo.timeSteps[step].real
				elif timeType=="User" : timeValue=moduleInfo.timeSteps[step].user
				elif timeType=="Sys" : timeValue=moduleInfo.timeSteps[step].sys
				else : raise Exception("Unknown value for timeType passed to dumpTime")
				lineToOutput+=str(timeValue)+","
			except KeyError :
				# For the event total, there is no information for construction etcetera so there
				# will be a key error. Just print an empty column.
				lineToOutput+=","
		print lineToOutput
				

moduleOrder=[]
steps=[]
info={}

outputContainsTime=False
outputContainsMemory=False

while True :
	line=input.readline()
	if len(line)==0 : break

	# Check that the line in the log is relevant for me
	if line[:15]==" *MODULETIMER* " :
		line=line[15:]
		inputIsTime=True
		outputContainsTime=True
	elif line[:14]==" *MEMCOUNTER* " :
		line=line[14:]
		inputIsTime=False
		outputContainsMemory=True
	else :
		continue

	columns=line.split(',')
	if len(columns)==1 : break

	moduleName=columns[1]
	step=columns[0]
	
	if steps.count(step)==0 : steps.append(step)
	
	if step[0:5]=="event" :
		if moduleOrder.count(moduleName)==0 : moduleOrder.append(moduleName)

	try :
		currentModule=info[moduleName]
	except KeyError :
		currentModule=ModuleInfo(columns)
		info[moduleName]=currentModule
	
	if inputIsTime :
		currentModule.addTimeStep( columns )
	else :
		currentModule.addStep( columns )

if outputContainsMemory :
	dumpMemory( info, moduleOrder, steps, peakMemory=True )
	print ""
	dumpMemory( info, moduleOrder, steps, peakMemory=False )

if outputContainsTime :
	dumpTime( info, moduleOrder, steps, "Real" )
	print ""
	dumpTime( info, moduleOrder, steps, "User" )
	print ""
	dumpTime( info, moduleOrder, steps, "Sys" )

