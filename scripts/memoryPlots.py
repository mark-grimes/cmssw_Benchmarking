"""
Plots RSS and VmSize from the output of checkMem.py
"""
import ROOT,math

def parseFile( inputFilename ) :
    try :
        inputFile=open( inputFilename )
        results=[]
    
        currentEvent=-999
        while True :
            line=inputFile.readline()
            if len(line)==0 : break
            
            fields=line.split()
            if len(fields)==3 :
                VmRss=int(float(fields[0]))
                VmSize=int(float(fields[1]))
                load=-1
                event=int(float(fields[2]))
            elif len(fields)==4 :
                VmRss=int(float(fields[0]))
                VmSize=int(float(fields[1]))
                load=float(fields[2])
                event=int(float(fields[3]))
            else :
                continue
            
            if event!=currentEvent :
                results.append( { "rss":[], "size":[], "load":[], "event":event } )
                currentEvent=event

            results[-1]["rss"].append( VmRss )
            results[-1]["size"].append( VmSize )
            if load!=-1 : results[-1]["load"].append(load)
    
        return results
        
    finally :
        inputFile.close()

def createXYCoords( results ) :
    rssCoords=[]
    sizeCoords=[]
    loadCoords=[]
    
    for eventResults in results :
        event=float(eventResults["event"])
        if event==-1 : event=0.0

        division=1.0/float(len(eventResults["rss"]))
        yValue=event
        for value in eventResults["rss"] :
            yValue+=division
            rssCoords.append( (yValue,value) )

        division=1.0/float(len(eventResults["size"]))
        yValue=event
        for value in eventResults["size"] :
            yValue+=division
            sizeCoords.append( (yValue,value) )

        division=1.0/float(len(eventResults["load"]))
        yValue=event
        for value in eventResults["load"] :
            yValue+=division
            loadCoords.append( (yValue,value) )
    
    return { "rss":rssCoords, "size":sizeCoords, "load":loadCoords }

def createXYTGraph( dataPoints ) :
    graph=ROOT.TGraphErrors()

    for index in range(0,len(dataPoints)) :
        graph.SetPoint( index, dataPoints[index][0], dataPoints[index][1] )

    return graph

def createTGraph( dataPoints, trimPoints=False ) :
    """
    Turns an array into a root TGraph, but trims the data by only recording
    new points when there is an inflection.
    """
    from scipy import stats
    graph=ROOT.TGraphErrors()
    graphIndex=0
    lastRecordedIndex=0
    lastRecordedValue=0
    lastValue=0
    for index in range(0,len(dataPoints)) :
        addPoint=False
        if index==0 : addPoint=True
        elif index==len(dataPoints)-1 : addPoint=True # always add first and last points
        elif trimPoints==False : addPoint=True
        else :
            # See if the point after this significantly deviates from the straight line
            # joining all of the previous points and this one. I.e. if the trend significantly
            # deflects at this point, record it.
            slope, intercept, r_value, p_value, std_err = stats.linregress(dataPoints[lastRecordedIndex:index],range(lastRecordedIndex,index))
            prediction=(index+1)*slope+intercept # where the next point would be if it follows the trend
            print abs( (prediction-dataPoints[index+1])/(prediction+dataPoints[index+1]) )
            if abs( (prediction-dataPoints[index+1])/(prediction+dataPoints[index+1]) )>5 :
                addPoint=True
            
        #elif lastRecordedIndex+5<index : addPoint=True
        
        if addPoint :
            graph.SetPoint( graphIndex, index, dataPoints[index] )
            graphIndex+=1
            lastRecordedIndex=index
            lastRecordedValue=dataPoints[index]
        lastValue=dataPoints[index]
    return graph

def createMultiPlot( plots, yAxisTitle ) :
    availableColours=[ROOT.kRed,ROOT.kBlue,ROOT.kGreen,ROOT.kMagenta,ROOT.kCyan,ROOT.kYellow]

    multiPlots=ROOT.TMultiGraph()
    for index in range(0,len(plots)) :
        plots[index].SetLineColor( availableColours[index%len(availableColours)] )
        plots[index].SetFillColor( ROOT.kWhite )
        multiPlots.Add(plots[index])
    
    multiCanvas=ROOT.TCanvas()
    multiPlots.Draw("al")
    multiPlots.GetYaxis().SetTitle(yAxisTitle)
    multiPlots.GetYaxis().SetTitleOffset(1.2)
    multiPlots.GetXaxis().SetTitle("Event number")
    if len(plots)>1 :
        legend=multiCanvas.BuildLegend();
        legend.SetFillColor( ROOT.kWhite )
#         legend.SetX1(0.791782729805014)
#         legend.SetX2(0.9986072423398329)
#         legend.SetY1(0.4386391251518832)
#         legend.SetY2(0.9975698663426489)
# 
#     multiCanvas.SetBottomMargin(0.10206561535596848)
#     multiCanvas.SetTopMargin(0.027946537360548973)
#     multiCanvas.SetLeftMargin(0.07311977446079254)
#     multiCanvas.SetRightMargin(0.1267409473657608)

    return {"canvas":multiCanvas,"multigraph":multiPlots}

recoInputFilenames = {"SLHC25p4":"/afs/cern.ch/user/g/grimes/currentJob/FastTimingMemory/memory_SLHC25p4.out",
                 "SLHC25p4CFWriteInTime":"/afs/cern.ch/user/g/grimes/currentJob/FastTimingMemory/memory_SLHC25p4CFWriteInTime.out",
                 "SLHC25p4PR8891":"/afs/cern.ch/user/g/grimes/currentJob/FastTimingMemory/memory_SLHC25p4PR8891.out",
                 "SLHC25p4 files from DBS":"/afs/cern.ch/user/g/grimes/currentJob/FastTimingMemory/memory_reco_DBS.out"}

digiInputFilenames = {"SLHC25p4":"/afs/cern.ch/user/g/grimes/currentJob/FastTimingMemory/memory_digi_SLHC25p4.out",
                 "SLHC25p4CFWriteInTime":"/afs/cern.ch/user/g/grimes/currentJob/FastTimingMemory/memory_digi_SLHC25p4CFWriteInTime.out",
                 "SLHC25p4PR8891":"/afs/cern.ch/user/g/grimes/currentJob/FastTimingMemory/memory_digi_SLHC25p4PR8891.out"}

inputFilenames=digiInputFilenames

import sys

if len(sys.argv)>1 : # If filenames are given on the command line, use those instead
    #inputFilenames=dict( zip( map(str,range(1,len(sys.argv[1:])+1)), sys.argv[1:] ) )
    inputFilenames={}
    nextArgumentIsTitle=False
    title="1"
    for index in range(1,len(sys.argv)) :
        if nextArgumentIsTitle :
            title=sys.argv[index]
            nextArgumentIsTitle=False
        elif sys.argv[index]=="-t" :
            nextArgumentIsTitle=True
        else:
            inputFilenames[title]=sys.argv[index]
            title=str( len(inputFilenames)+1 )
            
rssPlots=[]
sizePlots=[]
loadPlots=[]
for dataName in inputFilenames :
    results=createXYCoords( parseFile( inputFilenames[dataName] ) )
    rssPlots.append( createXYTGraph( results["rss"] ) )
    sizePlots.append( createXYTGraph( results["size"] ) )
    loadPlots.append( createXYTGraph( results["load"] ) )
    rssPlots[-1].SetTitle(dataName)
    sizePlots[-1].SetTitle(dataName)
    loadPlots[-1].SetTitle(dataName)


canvases=[]
canvases.append( createMultiPlot(rssPlots,"VmRSS/kb") )
canvases.append( createMultiPlot(sizePlots,"VmSize/kb") )
canvases.append( createMultiPlot(loadPlots,"System load") )
