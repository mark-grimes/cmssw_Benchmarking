"""
Accumulates all the results for a particule module class into one figure, and
plots that rather than the individual modules. I.e. plot a single module for
each input filename onto the same plot.
"""

import JobInfo, JobInfoPlot
import sys
import ROOT

#
# Check command line options
#
inputFilenames={}
collection=""
nextTitle=""
index=0
while index<len(sys.argv) :
    if sys.argv[index]=="-c" :
        if index+1>=len(sys.argv) : raise Exception("-c option requires an argument")
        if collection!="" : raise Exception("Only one -c option should be specified")
        index+=1
        collection=sys.argv[index]
    elif sys.argv[index]=="-t" :
        if index+1>=len(sys.argv) : raise Exception("-t option requires an argument")
        index+=1
        nextTitle=sys.argv[index]
    else:
        if nextTitle=="" : # Title not specified so make it from the filename
            nextTitle=sys.argv[index].split("/")[-1] # Chop off the path
            if nextTitle[-4:]==".log" : nextTitle=nextTitle[:-4]
            if nextTitle[:7]=="output_" : nextTitle=nextTitle[7:]
        if inputFilenames.keys().count(nextTitle)!=0 : # Add a suffix to distinguish between multiple titles
            suffix=1
            while inputFilenames.keys().count(nextTitle+"_"+str(suffix))!=0 : suffix+=1
            nextTitle+="_"+str(suffix)

        inputFilenames[nextTitle]=sys.argv[index]
        nextTitle=""
    index+=1

if collection=="" : raise Exception( "You need to specify which collection to plot with the '-c' option" )


accumulatedResults=type("AllResults",(),{"modules":{}}) # Need to mimic a JobInfo object so that I can reuse plotting functions
for title in inputFilenames :
    filename=inputFilenames[title]
    try :
        results=JobInfo.JobInfo.load(filename)
        accumulatedResults.modules[title]=results.modules[collection]
        accumulatedResults.modules[title].name=title
        if not hasattr(accumulatedResults, "steps") :
            accumulatedResults.steps=results.steps
        del results
    except Exception as error :
        print "Unable to load "+filename+" because: "+str(error)
      

numberOfModulesToPlot=250

peakPlot=JobInfoPlot.createPlotForMaximumModules( accumulatedResults, lambda step : step.peakMemory, "Peak memory", numberOfModulesToPlot )
productSizePlot=JobInfoPlot.createPlotForMaximumModules( accumulatedResults, lambda step : step.productSize, "Product size", numberOfModulesToPlot )
retainedExcludingProductPlot=JobInfoPlot.createPlotForMaximumModules( accumulatedResults, lambda step : step.heldMemory-step.productSize, "Retained memory excluding product", numberOfModulesToPlot )

