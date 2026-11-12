set pagination off
set confirm off
set breakpoint pending on
python
import gdb
import json
from pathlib import Path

phase = "outside"
visits = {"guarded": 0, "restore": 0}
counts = {name: {"solve_preparation": 0, "normalization": 0, "placement_writes": 0, "assembly_placement_writes": 0} for name in ("outside", "guarded", "restore")}

class PhaseBreakpoint(gdb.Breakpoint):
    def __init__(self, location, selected):
        super().__init__(location, internal=True)
        self.selected = selected
    def stop(self):
        global phase
        phase = self.selected
        if phase in visits:
            visits[phase] += 1
        gdb.write("PHASE " + phase + "\n")
        return False

class RestoreFinish(gdb.FinishBreakpoint):
    def __init__(self, frame, previous):
        super().__init__(frame, internal=True)
        self.previous = previous
    def stop(self):
        global phase
        gdb.write("RESTORE_RETURN\n")
        phase = self.previous
        return False

class RestoreBreakpoint(gdb.Breakpoint):
    def stop(self):
        global phase
        RestoreFinish(gdb.newest_frame(), phase)
        phase = "restore"
        visits[phase] += 1
        gdb.write("PHASE restore\n")
        return False

class CountBreakpoint(gdb.Breakpoint):
    def __init__(self, location, key):
        super().__init__(location, internal=True)
        self.key = key
    def stop(self):
        counts[phase][self.key] += 1
        if self.key == "placement_writes":
            frame = gdb.newest_frame()
            while frame:
                name = frame.name() or ""
                if any(writer in name for writer in ("Assembly::AssemblyObject::ensureIdentityPlacementsImpl", "Assembly::AssemblyObject::setNewPlacementsImpl", "Assembly::AssemblyObject::syncActiveRigidGroupPlacements")):
                    counts[phase]["assembly_placement_writes"] += 1
                    break
                frame = frame.older()
        return False

source = Path("tests/src/Mod/Assembly/App/AssemblyObject.cpp")
lines = source.read_text().splitlines()
start = next(i + 1 for i, line in enumerate(lines) if 'assembly->addObject<Assembly::JointGroup>("AdditionalJoints")' in line)
end = next(i + 2 for i, line in enumerate(lines) if "No replay on scope exit" in line)
PhaseBreakpoint(str(source.resolve()) + ":" + str(start), "guarded")
PhaseBreakpoint(str(source.resolve()) + ":" + str(end), "outside")
RestoreBreakpoint("App::DocumentPy::restore", internal=True)
CountBreakpoint("Assembly::AssemblyObject::runSolve", "solve_preparation")
CountBreakpoint("Assembly::AssemblyObject::ensureIdentityPlacementsImpl", "normalization")
CountBreakpoint("App::PropertyPlacement::setValue", "placement_writes")
end
run
python
result = {"counts": counts, "phase_visits": visits, "passed": visits["guarded"] > 0 and visits["restore"] > 0 and counts["guarded"]["solve_preparation"] == 0 and counts["guarded"]["normalization"] == 0 and counts["guarded"]["placement_writes"] == 0 and counts["restore"]["solve_preparation"] == 0 and counts["restore"]["normalization"] == 0 and counts["restore"]["assembly_placement_writes"] == 0 and counts["outside"]["solve_preparation"] > 0 and counts["outside"]["placement_writes"] > 0}
gdb.write("NATIVE_MOTION_RESULT " + json.dumps(result, sort_keys=True) + "\n")
end
