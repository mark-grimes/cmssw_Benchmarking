"""
Accumulates all the results for a particule module class into one figure, and
plots that rather than the individual modules.
"""

import JobInfo, JobInfoPlot
import sys
import ROOT

if len( sys.argv ) != 2 : raise Exception( 'The only parameter should be the filename of a dump of the cmsRun output' )


results=JobInfo.JobInfo.load( sys.argv[1] )

#
# Loop over all of the module information and add up anything
# that is from the same class into one MemoryLog object.
#

classResults=type("ClassResults",(),{"modules":{},"steps":results.steps}) # Need to mimic a JobInfo object so that I can reuse plotting functions

for moduleName in results.modules :
    module=results.modules[moduleName]

    try :
        classResult=classResults.modules[module.type]
    except KeyError :
        classResult=type("ClassInfo",(),{"steps":{},"name":module.type})
        classResults.modules[module.type]=classResult
        

    for step in module.steps :
        try :
            stepResult=classResult.steps[step]
        except KeyError :
            stepResult=JobInfo.MemoryLog(0,0,0,0)
            classResult.steps[step]=stepResult
        
        # Add all the things that are retained
        stepResult.heldMemory+=module.steps[step].heldMemory
        stepResult.productSize+=module.steps[step].productSize
        # The peaks never happen together, so only record the maximum for all of them
        stepResult.peakMemory=max( stepResult.peakMemory, module.steps[step].peakMemory )
        
numberOfModulesToPlot=25
    
peakPlot=JobInfoPlot.createPlotForMaximumModules( classResults, lambda step : step.peakMemory, "Peak memory", numberOfModulesToPlot )
JobInfoPlot.setDefaultPlotPlacement(peakPlot)
productSizePlot=JobInfoPlot.createPlotForMaximumModules( classResults, lambda step : step.productSize, "Product size", numberOfModulesToPlot )
JobInfoPlot.setDefaultPlotPlacement(productSizePlot)
retainedPlot=JobInfoPlot.createPlotForMaximumModules( classResults, lambda step : step.heldMemory, "Retained memory", numberOfModulesToPlot )
JobInfoPlot.setDefaultPlotPlacement(retainedPlot)
retainedExcludingProductPlot=JobInfoPlot.createPlotForMaximumModules( classResults, lambda step : 0 if step.productSize==0 else step.heldMemory-step.productSize, "Retained memory excluding product", numberOfModulesToPlot )
JobInfoPlot.setDefaultPlotPlacement(retainedExcludingProductPlot)
