import os,sys,json,unittest
from pathlib import Path
sys.path.insert(0,str(Path(os.environ['SPEC004_ROOT'])/'src/Mod/Sketcher'))
import FreeCAD as App
from SketcherTests.TestSketchExternalReattachment import TestSketchExternalReattachment
from SketcherTests import TestSketcherSolver
loader=unittest.defaultTestLoader
preferences=App.ParamGet('User parameter:BaseApp/Preferences/Mod/Sketcher/SolverAdvanced')
receipts=[]
for solver in (0,1,2):
    preferences.SetInt('DefaultSolver',solver)
    suite=loader.loadTestsFromTestCase(TestSketchExternalReattachment)
    names=[t.id() for t in suite]
    result=unittest.TextTestRunner(verbosity=2).run(suite)
    receipts.append({'kind':'public','solver':solver,'expected':names,'count':result.testsRun,'failures':len(result.failures),'errors':len(result.errors),'skipped':[str(x[0])+': '+x[1] for x in result.skipped],'success':result.wasSuccessful()})
preferences.SetInt('DefaultSolver',2)
suite=loader.loadTestsFromTestCase(TestSketcherSolver.TestSketcherSolver)
names=[t.id() for t in suite]
result=unittest.TextTestRunner(verbosity=2).run(suite)
receipts.append({'kind':'existing_solver','solver':2,'expected':names,'count':result.testsRun,'failures':len(result.failures),'errors':len(result.errors),'skipped':[str(x[0])+': '+x[1] for x in result.skipped],'success':result.wasSuccessful()})
Path(os.environ['SPEC004_RECEIPT']).write_text(json.dumps({'runs':receipts,'version':App.Version()},indent=2))
