import JobInfo, JobInfoPlot
import sys
import ROOT

if __name__ == '__main__' :
    import moduleMemoryPlots # I've seen weird things happen with the namespace if a "__main__" conditional doesn't import itself

    if len( sys.argv ) != 2 : raise Exception( 'The only parameter should be the filename of a dump of the cmsRun output' )
    
    results=JobInfo.JobInfo.load( sys.argv[1] )

    numberOfModulesToPlot=25

    peakPlot=JobInfoPlot.createPlotForMaximumModules( results, lambda step : step.peakMemory, "Peak memory", numberOfModulesToPlot )
    retainedPlot=JobInfoPlot.createPlotForMaximumModules( results, lambda step : step.heldMemory, "Retained memory", numberOfModulesToPlot )
    productSizePlot=JobInfoPlot.createPlotForMaximumModules( results, lambda step : step.productSize, "Product size", numberOfModulesToPlot )
    retainedExcludingProductPlot=JobInfoPlot.createPlotForMaximumModules( results, lambda step : step.heldMemory-step.productSize, "Retained memory excluding product", numberOfModulesToPlot )


