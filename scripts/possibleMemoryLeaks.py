import JobInfo
import sys
import ROOT
from scipy import stats


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


#
# Loop over all of the module information and add up anything
# that is from the same class into one MemoryLog object.
#

classResults=type("ClassResults",(),{"modules":{},"steps":results.steps}) # Need to mimic a JobInfo object so that I can reuse plotting functions

for moduleName in results.modules :
    module=results.modules[moduleName]
    #if module.type!="PoolOutputModule" : continue

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

def orderModulesByRetainedSlope( allResults, valueFunctor ) :    
    maximaForModules=[]
    for moduleName in allResults.modules :
        module=allResults.modules[moduleName]
        # I'm only interested in per event changes, so create a list of the events that are present
        eventSteps=[]
        for stepName in module.steps :
            if stepName[:5]=="event" : eventSteps.append(stepName)
        if len(eventSteps)<=1 : continue
        
        yValues=map( lambda stepName :valueFunctor(module.steps[stepName]), eventSteps )
        slope, intercept, r_value, p_value, std_err = stats.linregress(range(0,len(eventSteps)),yValues)
        maxStepName=max(module.steps,key=lambda stepName:valueFunctor(module.steps[stepName]) )
        maximaForModules.append( (moduleName,slope ) )
    return map(lambda x:x[0], sorted(maximaForModules,lambda a,b:cmp(a[1],b[1]),reverse=True) )

def drawTrendLine( plot ):
    yValues=[]
    xValues=[]
    axis=plot.GetXaxis()
    for binNumber in range(1,plot.GetNbinsX()+1) :
        if axis.GetBinLabel(binNumber)[:5]=="event" :
            if int(axis.GetBinLabel(binNumber)[5:])<4 : continue
            xValues.append( axis.GetBinCenter(binNumber) )
            yValues.append( plot.GetBinContent(binNumber) )
    slope, intercept, r_value, p_value, std_err = stats.linregress(xValues,yValues)
    xStart=axis.GetBinCenter(1)#xValues[0]
    xEnd=axis.GetBinCenter( axis.GetNbins() )#xValues[-1]
    print slope, intercept
    print xStart, slope*xStart+intercept, xEnd, slope*xEnd+intercept, plot.GetLineColor()
    plot.trendLine=ROOT.TLine( xStart, slope*xStart+intercept, xEnd, slope*xEnd+intercept )
    plot.trendLine.SetLineColor( plot.GetLineColor() )
    plot.trendLine.SetLineWidth( 2 )
    plot.trendLineLabel=ROOT.TPaveText( 20, 20, 100, 100 )
    plot.trendLineLabel.AddText( "y="+str(slope)+"*x + "+str(intercept) )
    plot.trendLineLabel.Draw()
    plot.trendLineLabel.SetFillColor( ROOT.kWhite )
    plot.trendLine.Draw()
    plot.trendLineLabel.Draw()
    

retainedFunctor=lambda step : 0 if step.productSize==0 else step.heldMemory-step.productSize
modulesToPlot=orderModulesByRetainedSlope( classResults, retainedFunctor )

from moduleMemoryPlots import createPlotForMaximumModules, createPlot
retainedExcludingProductPlot=createPlot( classResults, modulesToPlot[:25], retainedFunctor, "Retained memory excluding product", binLabelEvery=3 )

# drawTrendLine( retainedExcludingProductPlot.plots[0] )
# plot=retainedExcludingProductPlot.plots[0]
# plot.trendLine=ROOT.TLine( 0.0, 67.1565008552, 212.0, 84.4108289899 )
# plot.trendLine.SetLineColor( 632 )
# plot.trendLine.SetLineWidth( 2 )
# plot.trendLine.Draw()
# plot.trendLineLabel=ROOT.TPaveText( 20, 20, 100, 100 )
# plot.trendLineLabel.AddText( "y=0.0922021065625*x + 66.0232347546" )
# plot.trendLineLabel.Draw()
# plot.trendLineLabel.SetFillColor( ROOT.kWhite )