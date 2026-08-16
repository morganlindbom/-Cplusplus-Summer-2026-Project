# validate_project.py
from pathlib import Path
import sqlite3, sys
root=Path(__file__).resolve().parents[1]
errors=[]
sysdb=root/'src/systems/system.sqlite'
if not sysdb.exists(): errors.append('missing system.sqlite')
else:
    con=sqlite3.connect(sysdb)
    comps=con.execute('select component_id,database_path from components').fetchall()
    for cid,rel in comps:
        p=sysdb.parent/rel
        if not p.exists(): errors.append(f'{cid}: missing {p}')
        else:
            try:
                c=sqlite3.connect(p); c.execute('select 1 from metadata limit 1').fetchone(); c.close()
            except Exception as e: errors.append(f'{cid}: {e}')
    con.close()
funcs=list((root/'src/systems/components/pin_functions').glob('*/function.sqlite'))
if len(funcs)<50: errors.append(f'expected >=50 function databases, got {len(funcs)}')
for p in funcs:
    c=sqlite3.connect(p)
    if not c.execute("select value from metadata where key='function_id'").fetchone(): errors.append(f'{p}: missing function_id')
    if not c.execute('select count(*) from pin_mappings').fetchone()[0]: errors.append(f'{p}: no pin mappings')
    c.close()
print(f'UI/system databases checked: {len(comps) if sysdb.exists() else 0}')
print(f'Function databases checked: {len(funcs)}')
if errors:
    print('FAIL')
    for e in errors: print(' -',e)
    sys.exit(1)
print('PASS')
