import JobInfo
import sys
import ROOT

if len( sys.argv ) != 2 : raise Exception( 'The only parameter should be the filename of a dump of the cmsRun output' )


results=JobInfo.JobInfo()
try :
    input=open( sys.argv[1] )
    results.parseFile( input )
except :
    print "Exception occured while trying to process file "+filename
finally :
    input.close()
    del input


def createPlot( allResults, moduleNames, valueFunctor, title="" ):
    """
    For every module named in moduleNames, plots for each step whatever the result of
    the valueFunctor is. The valueFunctor takes a JobInfo.MemoryLog object, e.g. using

        lambda step : step.peakMemory

    as the functor will plot the peak memory for each step.
    """
    binLabelEvery=1
    availableColours=[ROOT.kRed,ROOT.kBlue,ROOT.kGreen,ROOT.kMagenta,ROOT.kCyan,ROOT.kYellow]
    # Need to give each plot a unique name because of the stupid root bullshit.
    # Use the title if was given, otherwise make up something random. Also need
    # to replace any spaces in the title.
    if title!="" :
        uniqueName=title.replace(" ","_")
    else :
        import random
        uniqueName="plots"+str(random.random())

    plots=[]
    for moduleIndex in range(0,len(moduleNames)) :
        module=allResults.modules[moduleNames[moduleIndex]]
        plots.append( ROOT.TH1F( uniqueName+"_"+str(len(plots)), module.name, len(module.steps), -0.5, len(module.steps)-0.5 ) )
        for index in range(0,len(allResults.steps)) :
            stepName=allResults.steps[index]
            try :
                plots[-1].SetBinContent( index+1, valueFunctor( module.steps[stepName] ) )
                if index%binLabelEvery==0 : plots[-1].GetXaxis().SetBinLabel( index+1, stepName )
            except KeyError :
                pass # If the file didn't finish properly not all modules will have the final step
        plots[-1].GetYaxis().SetTitle( "Memory/MiB" )
        #plots[-1].GetXaxis().SetRangeUser( -0.5, len(allResults.steps)-0.5 );
        plots[-1].SetLineColor( availableColours[moduleIndex%len(availableColours)]-moduleIndex/len(availableColours) )
        plots[-1].SetFillColor( ROOT.kWhite )
        plots[-1].SetBit( ROOT.TH1.kNoTitle, True );
        plots[-1].SetBit( ROOT.TH1.kNoStats, True );

    canvas=ROOT.TCanvas()
    if title!="" : canvas.SetTitle( title )
    for index in range(0,len(plots)) :
        if index==0 : plots[index].Draw("l")
        else : plots[index].Draw("l;same")
    legend=canvas.BuildLegend();
    legend.SetFillColor( ROOT.kWhite )

    return (canvas,plots)

def orderModulesByMaximumValue( allResults, valueFunctor ) :
    """
    Returns the module names that have the highest result for the valueFunctor, for any step.
    The valueFunctor takes a JobInfo.MemoryLog object, e.g. using

        lambda step : step.peakMemory

    as the functor will return a list of module names, with the one that uses the most memory
    first and the one that uses the least memory last.
    """
    maximaForModules=[]
    for moduleName in allResults.modules :
        module=allResults.modules[moduleName]
        maxStepName=max(module.steps,key=lambda stepName:valueFunctor(module.steps[stepName]) )
        maximaForModules.append( (moduleName,valueFunctor(module.steps[maxStepName]) ) )
    return map(lambda x:x[0], sorted(maximaForModules,lambda a,b:cmp(a[1],b[1]),reverse=True) )

def createPlotForMaximumModules( allResults, valueFunctor, title="", numberToPlot=None ) :
    """
    Just combines createPlot and orderModulesByMaximumValue into one easy call, passing
    the same valueFunctor for both.
    """
    moduleNames=orderModulesByMaximumValue( allResults, valueFunctor );
    if numberToPlot==None or numberToPlot>len(moduleNames) :
        subsetToPlot=moduleNames
    else :
        subsetToPlot=moduleNames[:numberToPlot]
    return createPlot( allResults, subsetToPlot, valueFunctor, title )


numberOfModulesToPlot=25

peakPlot=createPlotForMaximumModules( results, lambda step : step.peakMemory, "Peak memory", numberOfModulesToPlot )
retainedPlot=createPlotForMaximumModules( results, lambda step : step.heldMemory, "Retained memory", numberOfModulesToPlot )
productSizePlot=createPlotForMaximumModules( results, lambda step : step.productSize, "Product size", numberOfModulesToPlot )
retainedExcludingProductPlot=createPlotForMaximumModules( results, lambda step : step.heldMemory-step.productSize, "Retained memory excluding product", numberOfModulesToPlot )


