import os,sys,json,unittest
from pathlib import Path
sys.path.insert(0,os.environ['SPEC004_ROOT']+'/src/Mod/Sketcher')
import FreeCAD as App
import TestSketcherApp
App.ParamGet('User parameter:BaseApp/Preferences/Mod/Sketcher/SolverAdvanced').SetInt('DefaultSolver',2)
suite=unittest.defaultTestLoader.loadTestsFromModule(TestSketcherApp)
expected=[t.id() for s in suite for t in s]
result=unittest.TextTestRunner(verbosity=2).run(suite)
Path('/tmp/spec004-app-receipt.json').write_text(json.dumps({'expected':expected,'count':result.testsRun,'failures':len(result.failures),'errors':len(result.errors),'skips':[str(t)+': '+r for t,r in result.skipped],'success':result.wasSuccessful()},indent=2))
