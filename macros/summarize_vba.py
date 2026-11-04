import os

mod_names = [
    'ThisDocument',
    'SWGlobal',
    'SWBars',
    'SWDialogs',
    'SWConvert',
    'BlockParams',
    'SWShapeFormat',
    'SWModData1',
    'SWModData2',
    'SWModData3',
    'SWModData5'
]

summary = []

for name in mod_names:
    path = os.path.join(r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros', f'{name}.bas')
    if os.path.exists(path):
        with open(path, 'r', encoding='utf-8') as f:
            code = f.read()
        lines = code.splitlines()
        funcs = [l.strip() for l in lines if any(l.strip().startswith(kw) for kw in ['Public Sub', 'Private Sub', 'Sub ', 'Public Function', 'Private Function', 'Function '])]
        globals_list = [l.strip() for l in lines if l.strip().startswith('Public ') and not ('Sub ' in l or 'Function ' in l)]
        summary.append({
            'name': name,
            'lines': len(lines),
            'funcs': funcs,
            'globals': globals_list,
            'head': '\n'.join(lines[:20])
        })

for s in summary:
    print(f"=== MODULE: {s['name']} ({s['lines']} lines) ===")
    if s['globals']:
        print("  Globals:")
        for g in s['globals'][:5]:
            print("   ", g)
    print("  Functions/Subs:")
    for fn in s['funcs']:
        print("   ", fn)
    print()

