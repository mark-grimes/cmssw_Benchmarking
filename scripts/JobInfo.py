import cPickle, gzip

class MemoryLog :
    MemScale=1024.0*1024.0
    def __init__( self, heldMemory, peakMemory, heldAllocation, peakAllocation ) :
        self.heldMemory=float(heldMemory)/MemoryLog.MemScale
        self.peakMemory=float(peakMemory)/MemoryLog.MemScale
        self.heldAllocation=heldAllocation
        self.peakAllocation=peakAllocation
        self.productSize=0
    def addProductSize( self, productSize ) :
        self.productSize=float(productSize)/MemoryLog.MemScale

class TimeLog :
    def __init__( self, real, user, sys ) :
        self.real=float(real)/1000000000000.0
        self.user=float(user)/1000000000000.0
        self.sys=float(sys)/1000000000000.0

class ModuleInfo :
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
        
        self.steps[columns[0]]=MemoryLog(columns[3],columns[4],columns[5],columns[6])
        
        # I've recently started recording some extra info about the size of the products
        # This can only be printed in the event after it's relevant though, so this could
        # be tagged on the end here
        if len(columns)>=9 :
            self.steps[columns[7]].addProductSize(columns[8])
        
    def addTimeStep( self, columns ) :
        if len(columns)<6 : raise Exception("Not enough columns")
        if self.name!=columns[1] : raise Exception("Trying to add step from an incorrect module name")
        if self.type!=columns[2] : raise Exception("Trying to add step from an incorrect module type")
        
        self.timeSteps[columns[0]]=TimeLog(columns[3],columns[4],columns[5])

class JobInfo :
    def __init__(self):
        self.modules = {}
        self.runOrder = []
        self.steps = []
        self.containsTime = False
        self.containsMemory = False
        self.rss = []

    @staticmethod
    def load( filename ):
        inputFile=gzip.open( filename )
        newJobInfo=cPickle.load( inputFile )
        inputFile.close()
        return newJobInfo

    def save( self, filename ):
        outputFile=gzip.open( filename, 'w' )
        cPickle.dump( self, outputFile )
        outputFile.close()

    def parseFile( self, inputFile ):
        while True :
            line=inputFile.readline()
            if len(line)==0 : break
            self.addFromLine( line )

    def addFromLine( self, line ) :
        # Check that the line in the log is relevant for me
        if line[:15]==" *MODULETIMER* " :
            self.addStep( line[15:].split(','), True )
        elif line[:14]==" *MEMCOUNTER* " :
            self.addStep( line[14:].split(','), False )
        else :
            # I want to ignore all other lines in the cmsRun output. I might have been
            # fed a log of the RSS use though. I need to see if the line fits what that
            # looks like.
            columns=line.split()
            if len(columns)!=15 and len(columns)!=13 :
                return
            if columns[6]!="mem" or columns[8]!="kb" or columns[9]!="pid" or columns[11]!="node" :
                return
            self.addRSS( columns )

    def addStep( self, columns, inputIsTime ) :
        if( inputIsTime ) : self.containsTime = True
        else : self.containsMemory = True

        moduleName=columns[1]
        step=columns[0]

        if self.steps.count(step)==0 : self.steps.append(step)

        if step[0:5]=="event" :
            if self.runOrder.count(moduleName)==0 : self.runOrder.append(moduleName)

        try :
            currentModule=self.modules[moduleName]
        except KeyError :
            currentModule=ModuleInfo(columns)
            self.modules[moduleName]=currentModule

        if inputIsTime :
            currentModule.addTimeStep( columns )
        else :
            currentModule.addStep( columns )
    def addRSS( self, columns ) :
        self.rss.append( float(columns[7])/1024.0 )

if __name__ == '__main__':
    import sys
    import JobInfo # Need to import itself, otherwise pickling messes up the module name

    results={}
    for filename in sys.argv[1:] :
        try :
            input=open( filename )
            results[filename]=JobInfo()
            results[filename].parseFile( input )
        except :
            print "Exception occured while trying to process file "+filename
        finally :
            input.close()


